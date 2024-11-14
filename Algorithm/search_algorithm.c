#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <mpi.h>
#include "search_algorithm.h"

#define HASH_SIZE 10000

// Helper function to make a word lowercase
void toLowercase(char *str) {
    for (int i = 0; str[i]; i++) {
        str[i] = tolower(str[i]);
    }
}

// Structure for word frequency
typedef struct {
    char word[MAX_WORD_LEN];
    int count;
} WordFreq;

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

int main(int argc, char *argv[]) {
    int rank, size;
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    int numParts;
    int *partSizes = (int *)malloc(size * sizeof(int));
    char **textParts;

    if (rank == 0) {
        textParts = splitFileToStrings(FILENAME, size, partSizes);
        for (int dest = 1; dest < size; dest++) {
            MPI_Send(&partSizes[dest], 1, MPI_INT, dest, 0, MPI_COMM_WORLD);
            MPI_Send(textParts[dest], partSizes[dest], MPI_CHAR, dest, 0, MPI_COMM_WORLD);
        }
    } else {
        MPI_Recv(&partSizes[rank], 1, MPI_INT, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        textParts = (char **)malloc(sizeof(char *));
        textParts[0] = (char *)malloc((partSizes[rank] + 1) * sizeof(char));
        MPI_Recv(textParts[0], partSizes[rank], MPI_CHAR, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        textParts[0][partSizes[rank]] = '\0';
    }

    processPart(textParts[rank == 0 ? 0 : 0]);
    gatherAndFindMostFrequentWord(rank, size);

    // Cleanup omitted for brevity

    MPI_Finalize();
    return 0;
}