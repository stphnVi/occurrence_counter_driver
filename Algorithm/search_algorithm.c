#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include <string.h>

#define WORD_LENGTH 24    // Tamaño máximo de palabra
#define NUM_WORDS 300     // Número máximo de palabras
#define MAX_PART_SIZE 1024 // Tamaño máximo de cada parte

// Función para contar palabras en una parte del texto
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
        if (!found) {;
        }

        offset += index;
        if (offset >= strlen(buffer)) break;
    }
}

// Función para dividir el archivo y devolver las partes
void splitFileToParts(const char *filename, char **buffer, int *buffer_size, int rank, int num_processes) {
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

    // Dividir el archivo en partes
    long part_size = file_size / num_processes;
    *buffer_size = (rank == num_processes - 1) ? (file_size - part_size * (num_processes - 1)) : part_size;

    *buffer = (char *)malloc(*buffer_size);
    if (*buffer == NULL) {
        perror("Error al asignar memoria para buffer");
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    fseek(file, part_size * rank, SEEK_SET);
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
        if (rank == 0) printf("Uso: mpirun -np <num_procesos> ./programa <archivo_texto>\n");
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    const char *filename = argv[1];  // Archivo de texto pasado como argumento
    int buffer_size = 0;             // Tamaño del buffer para los procesos
    char *buffer = NULL;             // Puntero al buffer de texto

    // El proceso maestro lee su parte del archivo
    if (rank == 0) {
        splitFileToParts(filename, &buffer, &buffer_size, rank, size);

        // Luego, el maestro envía su parte a todos los esclavos
        for (int i = 1; i < size; i++) {
            MPI_Send(&buffer_size, 1, MPI_INT, i, 0, MPI_COMM_WORLD);
            MPI_Send(buffer, buffer_size, MPI_CHAR, i, 0, MPI_COMM_WORLD);
        }
    } else {

        printf("Prueba1");
        // Los esclavos reciben su parte del archivo
        MPI_Recv(&buffer_size, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        buffer = (char *)malloc(buffer_size);
        MPI_Recv(buffer, buffer_size, MPI_CHAR, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

        countWords(buffer, local_words, local_counts);

        MPI_Send(local_words, NUM_WORDS * WORD_LENGTH, MPI_CHAR, 0, 0, MPI_COMM_WORLD);
        MPI_Send(local_counts, NUM_WORDS, MPI_INT, 0, 0, MPI_COMM_WORLD);
    }

    // Contadores y palabras locales
    char local_words[NUM_WORDS][WORD_LENGTH];
    int local_counts[NUM_WORDS];

    // Imprimir qué parte del archivo está manejando cada proceso
    printf("Proceso %d está manejando el rango %d - %d de %s\n", rank, rank * buffer_size, (rank + 1) * buffer_size, filename);

    countWords(buffer, local_words, local_counts);

    // El proceso maestro recogerá los resultados de todos los procesos
    if (rank == 0) {
        // Matrices para almacenar las palabras y los contadores de todos los procesos
        char global_words[size][NUM_WORDS][WORD_LENGTH];
        int global_counts[size][NUM_WORDS];
        memset(global_counts, 0, sizeof(global_counts)); // Inicializar a cero

        // Copiar los resultados del proceso 0 a su propia matriz
        memcpy(global_words[0], local_words, sizeof(local_words));
        memcpy(global_counts[0], local_counts, sizeof(local_counts));

        // Recibir los resultados de los otros procesos
        for (int i = 1; i < size; i++) {
            MPI_Recv(global_words[i], NUM_WORDS * WORD_LENGTH, MPI_CHAR, i, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            MPI_Recv(global_counts[i], NUM_WORDS, MPI_INT, i, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        }

        // Encontrar la palabra más frecuente utilizando la función findMostFrequent
        char most_frequent[WORD_LENGTH];
        int max_count = 0;

        // Procesamos las palabras recibidas
        for (int i = 0; i < size; i++) {
            findMostFrequent(global_words[i], global_counts[i], most_frequent, &max_count);
        }

        // Mostrar la palabra más frecuente
        printf("Palabra más frecuente: '%s' (Aparece %d veces)\n", most_frequent, max_count);
    } else {
        // Enviar los resultados del proceso no maestro al maestro
        MPI_Send(local_words, NUM_WORDS * WORD_LENGTH, MPI_CHAR, 0, 0, MPI_COMM_WORLD);
        MPI_Send(local_counts, NUM_WORDS, MPI_INT, 0, 0, MPI_COMM_WORLD);
    }

    // Liberar memoria del buffer
    free(buffer);

    MPI_Finalize();
    return 0;
}
