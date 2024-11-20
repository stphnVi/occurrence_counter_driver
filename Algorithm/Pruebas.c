#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include <string.h>

#include "encryption.h"
//#include "sendRasp.h"

//Alice secret key
unsigned char alicesk[32] = {
 0x77,0x07,0x6d,0x0a,0x73,0x18,0xa5,0x7d
,0x3c,0x16,0xc1,0x72,0x51,0xb2,0x66,0x45
,0xdf,0x4c,0x2f,0x87,0xeb,0xc0,0x99,0x2a
,0xb1,0x77,0xfb,0xa5,0x1d,0xb9,0x2c,0x2a
};
//Alice public key
unsigned char alicepk[32] = {
 0x85,0x20,0xf0,0x09,0x89,0x30,0xa7,0x54
,0x74,0x8b,0x7d,0xdc,0xb4,0x3e,0xf7,0x5a
,0x0d,0xbf,0x3a,0x0d,0x26,0x38,0x1a,0xf4
,0xeb,0xa4,0xa9,0x8e,0xaa,0x9b,0x4e,0x6a
} ;

//Bob secret key
unsigned char bobsk[32] = {
 0x5d,0xab,0x08,0x7e,0x62,0x4a,0x8a,0x4b
,0x79,0xe1,0x7f,0x8b,0x83,0x80,0x0e,0xe6
,0x6f,0x3b,0xb1,0x29,0x26,0x18,0xb6,0xfd
,0x1c,0x2f,0x8b,0x27,0xff,0x88,0xe0,0xeb
} ;
//Bob public key
unsigned char bobpk[32] = {
 0xde,0x9e,0xdb,0x7d,0x7b,0x7d,0xc1,0xb4
,0xd3,0x5b,0x61,0xc2,0xec,0xe4,0x35,0x37
,0x3f,0x83,0x43,0xc8,0x5b,0x78,0x67,0x4d
,0xad,0xfc,0x7e,0x14,0x6f,0x88,0x2b,0x4f
} ;

#define WORD_LENGTH 24    // Tamaño máximo de palabra
#define NUM_WORDS 500     // Número máximo de palabras
#define MAX_PART_SIZE 5,120 // Tamaño máximo de cada parte

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

    // Calcular el tamaño base de cada parte
    long base_part_size = file_size / num_processes;
    *buffers = (char **)malloc(num_processes * sizeof(char *));
    *buffer_sizes = (int *)malloc(num_processes * sizeof(int));

    long current_offset = 0;
    for (int i = 0; i < num_processes; i++) {
        // Estimar el tamaño inicial de la parte
        long part_size = (i == num_processes - 1) ? (file_size - current_offset) : base_part_size;

        // Ajustar para no partir palabras
        fseek(file, current_offset + part_size - 1, SEEK_SET);
        int c = fgetc(file);
        while (c != EOF && !isspace(c)) {
            part_size++;
            c = fgetc(file);
        }

        // Leer el buffer ajustado
        (*buffer_sizes)[i] = part_size;
        (*buffers)[i] = (char *)malloc(part_size + 1);
        if ((*buffers)[i] == NULL) {
            perror("Error al asignar memoria para buffer");
            MPI_Abort(MPI_COMM_WORLD, 1);
        }

        fseek(file, current_offset, SEEK_SET);
        size_t read_size = fread((*buffers)[i], 1, part_size, file);
        if (read_size < part_size) {
            printf("Error al leer el archivo.\n");
            for (int j = 0; j <= i; j++) {
                free((*buffers)[j]);
            }
            free(*buffers);
            free(*buffer_sizes);
            MPI_Abort(MPI_COMM_WORLD, 1);
        }

        // Asegurarse de que el buffer esté terminado como cadena
        (*buffers)[i][read_size] = '\0';

        // Actualizar el offset para la siguiente parte
        current_offset += part_size;
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

    //Generation of shared secret and symmetric key
    unsigned char alice_shared_secret[32], bob_shared_secret[32], symmetric_key[32];

    generate_shared_secret(alice_shared_secret, alicesk, bobpk);
    generate_shared_secret(bob_shared_secret, alicesk, bobpk);

    //Derive symmetric keys
    unsigned char alice_key[32], bob_key[32];

    derive_symmetric_key(alice_key, alice_shared_secret);
    derive_symmetric_key(bob_key, bob_shared_secret);

    if (buffers == NULL || buffer_sizes == NULL) {
        fprintf(stderr, "Error al asignar memoria para buffers o buffer_sizes\n");
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    // El proceso 0 divide el archivo y envía las partes a los demás procesos
    if (rank == 0) {
        splitFileForProcesses(filename, &buffers, &buffer_sizes, num_processes);

        for (int i = 1; i < num_processes; i++) {
            int max_ciphertext_len = buffer_sizes[i] + 64;
            unsigned char *ciphertext = malloc(max_ciphertext_len);
            unsigned char iv[12], tag[16];
            int ciphertext_len = 0;

            if (!ciphertext) {
                fprintf(stderr, "Memory allocation failed for ciphertext\n");
                MPI_Abort(MPI_COMM_WORLD, 1);
            }

            encrypt_message(alice_key, (unsigned char*)buffers[i], buffer_sizes[i], 
                            iv, tag, ciphertext, &ciphertext_len);

            MPI_Send(&ciphertext_len, 1, MPI_INT, i, 0, MPI_COMM_WORLD);
            MPI_Send(&buffer_sizes[i], 1, MPI_INT, i, 4, MPI_COMM_WORLD); // Enviar tamaño original
            MPI_Send(iv, 12, MPI_UNSIGNED_CHAR, i, 1, MPI_COMM_WORLD);
            MPI_Send(tag, 16, MPI_UNSIGNED_CHAR, i, 2, MPI_COMM_WORLD);
            MPI_Send(ciphertext, ciphertext_len, MPI_CHAR, i, 3, MPI_COMM_WORLD);

            free(ciphertext);
        }
    } else {
        int ciphertext_len;
        MPI_Recv(&ciphertext_len, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        MPI_Recv(&buffer_sizes[rank], 1, MPI_INT, 0, 4, MPI_COMM_WORLD, MPI_STATUS_IGNORE); // Recibir tamaño original

        unsigned char iv[12], tag[16];
        unsigned char *ciphertext = malloc(ciphertext_len);

        MPI_Recv(iv, 12, MPI_UNSIGNED_CHAR, 0, 1, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        MPI_Recv(tag, 16, MPI_UNSIGNED_CHAR, 0, 2, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        MPI_Recv(ciphertext, ciphertext_len, MPI_CHAR, 0, 3, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

        buffers[rank] = malloc(buffer_sizes[rank]);

        unsigned char *decrypted = malloc(buffer_sizes[rank]);
        decrypt_message(bob_key, iv, tag, ciphertext, ciphertext_len, decrypted);

        memcpy(buffers[rank], decrypted, buffer_sizes[rank]);

        free(ciphertext);
        free(decrypted);
    }

    // Contadores y palabras locales
    char local_words[NUM_WORDS][WORD_LENGTH] = {0};
    int local_counts[NUM_WORDS] = {0};

    // Imprimir qué parte del archivo está manejando cada proceso
    printf("Proceso %d está manejando el rango %d - %d de %s\n", rank, rank * buffer_sizes[rank], (rank + 1) * buffer_sizes[rank], filename);

    // Contar las palabras en el buffer
    countWords(buffers[rank], local_words, local_counts);
    printf("Proceso %d: Conteo de palabras locales:\n", rank);
    for (int i = 0; i < NUM_WORDS; i++) {
        if (local_counts[i] > 0) {
            printf("Palabra: '%s', Conteo: %d\n", local_words[i], local_counts[i]);
        }
    }

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
