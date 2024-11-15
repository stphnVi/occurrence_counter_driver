#include <stdio.h>
#include <string.h>
#include <mpi.h>
#include <stdlib.h>

#define WORD_LENGTH 24    // Tamaño máximo de palabra
#define NUM_WORDS 100     // Número máximo de palabras
#define MAX_PART_SIZE 1024 // Tamaño máximo de cada parte

// Función para contar palabras
void countWords(char *buffer, char words[][WORD_LENGTH], int counts[]) {
    char word[WORD_LENGTH];
    int index = 0, offset = 0;
    memset(words, 0, sizeof(words[0]) * NUM_WORDS);
    memset(counts, 0, sizeof(counts[0]) * NUM_WORDS);

    while (sscanf(buffer + offset, "%23s%n", word, &index) == 1) {
        int found = 0;
        for (int i = 0; i < NUM_WORDS; i++) {
            if (strcmp(words[i], word) == 0) {
                counts[i]++;
                found = 1;
                break;
            }
            if (strlen(words[i]) == 0) {
                strcpy(words[i], word);
                counts[i] = 1;
                found = 1;
                break;
            }
        }
        if (!found) {
            printf("Advertencia: Número de palabras excede NUM_WORDS. Palabra ignorada: %s\n", word);
        }

        offset += index;
        if (offset >= strlen(buffer)) break;
    }
}

// Función para dividir el archivo en partes
void splitFileToParts(const char *filename, char **buffer, int *buffer_size, int rank) {
    FILE *file = fopen(filename, "r");
    if (!file) {
        if (rank == 0) {
            perror("Error al abrir el archivo");
        }
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    if (file_size <= 0) {
        if (rank == 0) {
            printf("Archivo vacío o no legible.\n");
        }
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    fseek(file, 0, SEEK_SET);

    *buffer_size = file_size / 2 + 1;  // Ajustar tamaño para dividir en partes
    *buffer = (char *)malloc(*buffer_size);
    if (*buffer == NULL) {
        perror("Error al asignar memoria para buffer");
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    size_t read_size = fread(*buffer, 1, *buffer_size, file);
    if (read_size == 0) {
        if (rank == 0) {
            printf("Error al leer el archivo.\n");
        }
        free(*buffer);
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    fclose(file);
}


// Función para encontrar la palabra más frecuente
void findMostFrequent(char words[][WORD_LENGTH], int counts[], char *most_frequent, int *max_count) {
    *max_count = 0;
    for (int i = 0; i < NUM_WORDS; i++) {
        if (counts[i] > *max_count) {
            *max_count = counts[i];
            strcpy(most_frequent, words[i]);
        }
    }
}

int main(int argc, char **argv) {
    MPI_Init(&argc, &argv);

    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    if (argc < 2) {
        if (rank == 0) printf("Uso: mpirun -np <num_proceso    int rank;s> ./programa <archivo_texto>\n");
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    const char *filename = argv[1];  // Archivo de texto pasado como argumento
    int buffer_size = 0;             // Tamaño del buffer para los procesos
    char *buffer = NULL;             // Puntero al buffer de texto

    // Llamada a la función para dividir el archivo
    splitFileToParts(filename, &buffer, &buffer_size, rank);


    char local_words[NUM_WORDS][WORD_LENGTH];
    int local_counts[NUM_WORDS];

    countWords(buffer, local_words, local_counts);

    char global_words[size][NUM_WORDS][WORD_LENGTH];
    int global_counts[size][NUM_WORDS];
    memset(global_counts, 0, sizeof(global_counts)); // Inicializar a cero

    MPI_Gather(local_counts, NUM_WORDS, MPI_INT, global_counts, NUM_WORDS, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Gather(local_words, NUM_WORDS * WORD_LENGTH, MPI_CHAR, global_words, NUM_WORDS * WORD_LENGTH, MPI_CHAR, 0, MPI_COMM_WORLD);

    if (rank == 0) {
        char most_frequent[WORD_LENGTH];
        int max_count = 0;

        for (int i = 0; i < size; i++) {
            for (int j = 0; j < NUM_WORDS; j++) {
                if (strlen(global_words[i][j]) > 0) {
                    for (int k = 0; k < size; k++) {
                        if (strcmp(global_words[i][j], global_words[k][j]) == 0) {
                            global_counts[0][j] += global_counts[k][j];
                        }
                    }
                    if (global_counts[0][j] > max_count) {
                        max_count = global_counts[0][j];
                        strcpy(most_frequent, global_words[i][j]);
                    }
                }
            }
        }

        printf("Palabra más frecuente: '%s' (Aparece %d veces)\n", most_frequent, max_count);
    }

    MPI_Finalize();
    return 0;
}
