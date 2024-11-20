#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include <string.h>

#define WORD_LENGTH 24    // Tamaño máximo de palabra
#define NUM_WORDS 300     // Número máximo de palabras
#define MAX_PART_SIZE 1024 // Tamaño máximo de cada parte

void countWords(char *buffer, char words[NUM_WORDS][WORD_LENGTH], int counts[NUM_WORDS]) {
    int num_words = 0;
    char *token = strtok(buffer, " \n\t.,!?;:\"()[]{}"); // Tokeniza por espacios y puntuación

    while (token != NULL && num_words < NUM_WORDS) {
        // Buscar si la palabra ya está en la lista
        int found = 0;
        for (int i = 0; i < num_words; i++) {
            if (strcmp(words[i], token) == 0) {
                counts[i]++;
                found = 1;
                break;
            }
        }

        // Si no está, añadirla
        if (!found) {
            strncpy(words[num_words], token, WORD_LENGTH - 1);
            words[num_words][WORD_LENGTH - 1] = '\0'; // Garantizar terminación nula
            counts[num_words] = 1;
            num_words++;
        }

        token = strtok(NULL, " \n\t.,!?;:\"()[]{}");
    }
}

void splitFileForProcesses(const char *filename, char ***buffers, int **buffer_sizes, int num_processes) {
    FILE *file = fopen(filename, "r");
    if (!file) {
        perror("Error al abrir el archivo");
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    // Obtener el tamaño total del archivo
    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    if (file_size <= 0) {
        printf("Archivo vacío o no legible.\n");
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    fseek(file, 0, SEEK_SET);

    // Calcular el tamaño de cada parte
    long part_size = file_size / num_processes;
    *buffers = (char **)malloc(num_processes * sizeof(char *));
    *buffer_sizes = (int *)malloc(num_processes * sizeof(int));

    for (int i = 0; i < num_processes; i++) {
        (*buffer_sizes)[i] = (i == num_processes - 1) ? (file_size - part_size * (num_processes - 1)) : part_size;
        (*buffers)[i] = (char *)malloc((*buffer_sizes)[i]);
        if ((*buffers)[i] == NULL) {
            perror("Error al asignar memoria para buffer");
            MPI_Abort(MPI_COMM_WORLD, 1);
        }

        // Leer la parte del archivo correspondiente a cada proceso
        fseek(file, part_size * i, SEEK_SET);
        size_t read_size = fread((*buffers)[i], 1, (*buffer_sizes)[i], file);
        if (read_size == 0) {
            printf("Error al leer el archivo.\n");
            for (int j = 0; j <= i; j++) {
                free((*buffers)[j]);
            }
            free(*buffers);
            free(*buffer_sizes);
            MPI_Abort(MPI_COMM_WORLD, 1);
        }
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

static int mergeCounts_calls = 0; // Contador global de llamadas

void mergeCounts(char global_words[][WORD_LENGTH], int global_counts[], 
                 char local_words[][WORD_LENGTH], int local_counts[]) {
    mergeCounts_calls++;
    printf("mergeCounts llamada #%d\n", mergeCounts_calls);

    for (int i = 0; i < NUM_WORDS; i++) {
        if (strlen(local_words[i]) == 0) continue;

        int found = 0;
        for (int j = 0; j < NUM_WORDS; j++) {
            if (strcmp(global_words[j], local_words[i]) == 0) {
                global_counts[j] += local_counts[i];
                found = 1;
                break;
            }
        }

        if (!found) {
            for (int j = 0; j < NUM_WORDS; j++) {
                if (strlen(global_words[j]) == 0) {
                    strcpy(global_words[j], local_words[i]);
                    global_counts[j] = local_counts[i];
                    break;
                }
            }
        }
    }
}

int main(int argc, char **argv) {
    // Inicializar MPI
    MPI_Init(&argc, &argv);

    int rank, num_processes;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &num_processes);

    // Verificar que se haya proporcionado el nombre del archivo como argumento
    if (argc != 2) {
        if (rank == 0) {
            printf("Uso: %s <nombre_archivo>\n", argv[0]);
        }
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    const char *filename = argv[1];

    // Declarar buffers y buffer_sizes correctamente
    char **buffers = NULL;
    int *buffer_sizes = NULL;

    // Asignar memoria para buffers y buffer_sizes
    buffers = (char **)malloc(num_processes * sizeof(char *));
    buffer_sizes = (int *)malloc(num_processes * sizeof(int));

    if (buffers == NULL || buffer_sizes == NULL) {
        fprintf(stderr, "Error al asignar memoria para buffers o buffer_sizes\n");
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    // El proceso 0 divide el archivo y envía las partes a los demás procesos
    if (rank == 0) {
        splitFileForProcesses(filename, &buffers, &buffer_sizes, num_processes);

        // Enviar las partes del archivo a los procesos 1, 2, ..., num_processes-1
        for (int i = 1; i < num_processes; i++) {
            // Enviar el tamaño del buffer al proceso i
            MPI_Send(&buffer_sizes[i], 1, MPI_INT, i, 0, MPI_COMM_WORLD);
            // Enviar el contenido del buffer al proceso i
            MPI_Send(buffers[i], buffer_sizes[i], MPI_CHAR, i, 0, MPI_COMM_WORLD);
        }

    } else {
        // Recibir el tamaño del buffer
        MPI_Recv(&buffer_sizes[rank], 1, MPI_INT, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

        // Asignar memoria para el buffer del proceso actual
        buffers[rank] = (char *)malloc(buffer_sizes[rank]);
        if (buffers[rank] == NULL) {
            fprintf(stderr, "Error al asignar memoria para el buffer en proceso %d\n", rank);
            MPI_Abort(MPI_COMM_WORLD, 1);
        }

        // Recibir los datos del proceso 0
        MPI_Recv(buffers[rank], buffer_sizes[rank], MPI_CHAR, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

        // Imprimir el contenido de los datos recibidos para depuración
        printf("Proceso %d, Tamaño del buffer: %d\n", rank, buffer_sizes[rank]);
        for (int i = 0; i < buffer_sizes[rank]; i++) {
            printf("%c", buffers[rank][i]);
        }
        printf("\n"); // Imprimir el contenido del buffer
    }

    // Contadores y palabras locales
    char local_words[NUM_WORDS][WORD_LENGTH] = {0};
    int local_counts[NUM_WORDS] = {0};

    // Imprimir qué parte del archivo está manejando cada proceso
    printf("Proceso %d está manejando el rango %d - %d de %s\n", rank, rank * buffer_sizes[rank], (rank + 1) * buffer_sizes[rank], filename);

    // Contar las palabras en el buffer
    countWords(buffers[rank], local_words, local_counts);

    if (rank == 0) {
        char global_words[NUM_WORDS][WORD_LENGTH] = {0};
        int global_counts[NUM_WORDS] = {0};

        // Combinar los resultados locales del proceso 0
        mergeCounts(global_words, global_counts, local_words, local_counts);

        // Recibir y combinar los resultados de los otros procesos
        for (int i = 1; i < num_processes; i++) {
            char received_words[NUM_WORDS][WORD_LENGTH] = {0};
            int received_counts[NUM_WORDS] = {0};

            // Recibir las palabras y los conteos
            MPI_Recv(received_words, NUM_WORDS * WORD_LENGTH, MPI_CHAR, i, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            MPI_Recv(received_counts, NUM_WORDS, MPI_INT, i, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

            // Combinar los resultados globales
            mergeCounts(global_words, global_counts, received_words, received_counts);
        }

        // Encontrar y mostrar la palabra más frecuente
        char most_frequent[WORD_LENGTH];
        int max_count;
        findMostFrequent(global_words, global_counts, most_frequent, &max_count);

        printf("Palabra más frecuente: '%s' (Aparece %d veces)\n", most_frequent, max_count);
    } else {
        // Enviar las palabras y los conteos al proceso 0
        MPI_Send(local_words, NUM_WORDS * WORD_LENGTH, MPI_CHAR, 0, 0, MPI_COMM_WORLD);
        MPI_Send(local_counts, NUM_WORDS, MPI_INT, 0, 0, MPI_COMM_WORLD);
    }

    // Liberar memoria
    if (rank != 0) {
        free(buffers[rank]);  // Liberar solo en los procesos secundarios
    }

    // Liberar buffers y buffer_sizes solo en el proceso 0
    if (rank == 0) {
        for (int i = 0; i < num_processes; i++) {
            free(buffers[i]);
        }
        free(buffers);
        free(buffer_sizes);
    }

    // Finalizar MPI
    MPI_Finalize();

    return 0;
}
