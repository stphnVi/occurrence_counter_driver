#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <mpi.h>
#include "search_algorithm.h"

// Helper function to make a word lowercase
void toLowercase(char *str) {
    for (int i = 0; str[i]; i++) {
        str[i] = tolower(str[i]);
    }
}

WordFreq *hashTable[HASH_SIZE];

// Simple hash function
unsigned int hash(const char *word) {
    unsigned int hash = 0;
    while (*word) {
        hash = (hash << 2) ^ *word++;
    }
    return hash % HASH_SIZE;
}

// Function to add a word to the hash table
void addWord(const char *word) {
    unsigned int idx = hash(word);
    while (hashTable[idx] != NULL) {
        if (strcmp(hashTable[idx]->word, word) == 0) {
            hashTable[idx]->count++;
            return;
        }
        idx = (idx + 1) % HASH_SIZE;
    }
    hashTable[idx] = (WordFreq *)malloc(sizeof(WordFreq));
    strcpy(hashTable[idx]->word, word);
    hashTable[idx]->count = 1;
}

// Function to process each part and count word frequencies
void processPart(const char *text) {
    char word[MAX_WORD_LEN];
    int wordLen = 0;

    for (int i = 0; text[i]; i++) {
        if (isalpha(text[i])) {
            if (wordLen < MAX_WORD_LEN - 1) {
                word[wordLen++] = tolower(text[i]);
            }
        } else {
            if (wordLen > 0) {
                word[wordLen] = '\0';
                addWord(word);
                wordLen = 0;
            }
        }
    }
    if (wordLen > 0) {
        word[wordLen] = '\0';
        addWord(word);
    }
}

// Function to gather word frequencies from each process and find the most frequent word globally
void gatherAndFindMostFrequentWord(int rank, int size) {
    if (rank == 0) {
        // Master receives word frequencies from each process
        WordFreq globalHashTable[HASH_SIZE] = {0};
        for (int src = 1; src < size; src++) {
            WordFreq recvTable[HASH_SIZE];
            MPI_Recv(recvTable, HASH_SIZE * sizeof(WordFreq), MPI_BYTE, src, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            for (int i = 0; i < HASH_SIZE; i++) {
                if (recvTable[i].count > 0) {
                    addWordToGlobalHash(globalHashTable, recvTable[i].word, recvTable[i].count);
                }
            }
        }

        // Find the most frequent word in the global hash table
        char mostFreqWord[MAX_WORD_LEN] = "";
        int maxCount = 0;
        for (int i = 0; i < HASH_SIZE; i++) {
            if (globalHashTable[i].count > maxCount) {
                maxCount = globalHashTable[i].count;
                strcpy(mostFreqWord, globalHashTable[i].word);
            }
        }
        printf("Most frequent word: '%s' (appears %d times)\n", mostFreqWord, maxCount);

    } else {
        // Each worker sends its local hash table to the master
        MPI_Send(hashTable, HASH_SIZE * sizeof(WordFreq), MPI_BYTE, 0, 0, MPI_COMM_WORLD);
    }
}

// Utility function to add word frequencies to the global hash table
void addWordToGlobalHash(WordFreq *globalHashTable, const char *word, int count) {
    unsigned int idx = hash(word);
    while (globalHashTable[idx].count > 0) {
        if (strcmp(globalHashTable[idx].word, word) == 0) {
            globalHashTable[idx].count += count;
            return;
        }
        idx = (idx + 1) % HASH_SIZE;
    }
    strcpy(globalHashTable[idx].word, word);
    globalHashTable[idx].count = count;
}

// Function to split a file into a specific number of parts based on 'size' parameter
char **splitFileToStrings(const char *filename, int size, int *outPartSizes) {
    FILE *file = fopen(filename, "rb");
    if (!file) {
        perror("Error opening file");
        return NULL;
    }

    fseek(file, 0, SEEK_END);
    long fileSize = ftell(file);
    fseek(file, 0, SEEK_SET);

    int numParts = size;
    int basePartSize = fileSize / numParts;
    int remainingBytes = fileSize % numParts;

    char **partsArray = (char **)malloc(numParts * sizeof(char *));
    if (!partsArray) {
        perror("Memory allocation for parts array failed");
        fclose(file);
        return NULL;
    }

    for (int i = 0; i < numParts; i++) {
        int currentPartSize = basePartSize + (i < remainingBytes ? 1 : 0);
        outPartSizes[i] = currentPartSize;

        partsArray[i] = (char *)malloc((currentPartSize + 1) * sizeof(char));
        if (!partsArray[i]) {
            perror("Memory allocation for part failed");
            for (int j = 0; j < i; j++) free(partsArray[j]);
            free(partsArray);
            fclose(file);
            return NULL;
        }

        size_t bytesRead = fread(partsArray[i], 1, currentPartSize, file);
        partsArray[i][bytesRead] = '\0'; // Null-terminate the string
    }

    fclose(file);
    return partsArray;
}

// Limpia la tabla hash liberando la memoria de cada entrada
void cleanupHashTable() {
    for (int i = 0; i < HASH_SIZE; i++) {
        if (hashTable[i] != NULL) {
            free(hashTable[i]);
            hashTable[i] = NULL;
        }
    }
}


int main(int argc, char *argv[]) {
    int rank, size;

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    int numParts = size;
    int *partSizes = malloc(numParts * sizeof(int));
    if (!partSizes) {
        perror("Memory allocation for part sizes failed");
        MPI_Finalize();
        return 1;
    }

    char **textParts = NULL;
    if (rank == 0) {
        textParts = splitFileToStrings(FILENAME, numParts, partSizes);
        if (!textParts) {
            free(partSizes);
            MPI_Finalize();
            return 1;
        }
    }

    // Distribuir partes de texto
    int localPartSize;
    char *localText = NULL;

    if (rank == 0) {
        localPartSize = partSizes[rank];
        localText = textParts[rank];

        for (int i = 1; i < size; i++) {
            //Encriptar
            MPI_Send(&partSizes[i], 1, MPI_INT, i, 0, MPI_COMM_WORLD);
            MPI_Send(textParts[i], partSizes[i], MPI_CHAR, i, 0, MPI_COMM_WORLD);
        }
    } else {
        MPI_Recv(&localPartSize, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        localText = malloc(localPartSize + 1);
        MPI_Recv(localText, localPartSize, MPI_CHAR, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        //Traducir
        localText[localPartSize] = '\0';
    }

    // Procesar la parte de texto localmente
    processPart(localText);

    // Enviar resultados locales al proceso maestro
    if (rank != 0) {
        for (int i = 0; i < HASH_SIZE; i++) {
            if (hashTable[i] != NULL) {
                MPI_Send(hashTable[i]->word, MAX_WORD_LEN, MPI_CHAR, 0, 0, MPI_COMM_WORLD);
                MPI_Send(&hashTable[i]->count, 1, MPI_INT, 0, 0, MPI_COMM_WORLD);
            }
        }
    } else {
        WordFreq globalHashTable[HASH_SIZE] = {0};

        // Recibir datos de otros procesos y combinarlos
        for (int i = 1; i < size; i++) {
            char word[MAX_WORD_LEN];
            int count;

            while (1) {
                MPI_Recv(word, MAX_WORD_LEN, MPI_CHAR, i, MPI_ANY_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                if (strcmp(word, "END") == 0) break;

                MPI_Recv(&count, 1, MPI_INT, i, MPI_ANY_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                addWordToGlobalHash(globalHashTable, word, count);
            }
        }

        // Encontrar la palabra más frecuente globalmente
        char mostFreqWord[MAX_WORD_LEN];
        int maxCount = 0;
        for (int i = 0; i < HASH_SIZE; i++) {
            if (globalHashTable[i].count > maxCount) {
                maxCount = globalHashTable[i].count;
                strcpy(mostFreqWord, globalHashTable[i].word);
            }
        }
        printf("Most frequent word: '%s' (appears %d times)\n", mostFreqWord, maxCount);
    }

    cleanupHashTable();
    free(localText);
    free(partSizes);
    if (rank == 0) free(textParts);

    MPI_Finalize();
    return 0;
}
