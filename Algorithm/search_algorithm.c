#include <stdio.h> 
#include <stdlib.h> 
#include <string.h>
#include <mpi.h> 
#include <ctype.h> 
#include "search_algorithm.h"

 
// Helper function to make a word lowercase 
void toLowercase(char *str) { 
    for (int i = 0; str[i]; i++) { 
        str[i] = tolower(str[i]); 
    } 
} 
 
// Hash table to store word frequencies for each part 
#define HASH_SIZE 10000 
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
    if (wordLen > 0) {  // Add the last word if text ends with a word 
        word[wordLen] = '\0'; 
        addWord(word); 
    } 
} 
 
// Function to find the most frequent word across all parts 
void findMostFrequentWord(char *mostFreqWord, int *maxCount) { 
    *maxCount = 0; 
    mostFreqWord[0] = '\0'; 
 
    for (int i = 0; i < HASH_SIZE; i++) { 
        if (hashTable[i] != NULL && hashTable[i]->count > *maxCount) { 
            *maxCount = hashTable[i]->count; 
            strcpy(mostFreqWord, hashTable[i]->word); 
        } 
    } 
} 
 
// Cleanup function to free the hash table 
void cleanupHashTable() { 
    for (int i = 0; i < HASH_SIZE; i++) { 
        if (hashTable[i] != NULL) { 
            free(hashTable[i]); 
            hashTable[i] = NULL; 
        } 
    } 
} 
 
// Function to split a file into parts of up to MAX_PART_SIZE bytes 
char **splitFileToStrings(const char *filename, int *numParts, int *outPartSizes) { 
    FILE *file = fopen(filename, "rb"); 
    if (!file) { 
        perror("Error opening file"); 
        return NULL; 
    } 
 
    fseek(file, 0, SEEK_END); 
    long fileSize = ftell(file); 
    fseek(file, 0, SEEK_SET); 
 
    *numParts = (fileSize + MAX_PART_SIZE - 1) / MAX_PART_SIZE; 
 
    char **partsArray = (char **)malloc(*numParts * sizeof(char *));
    if (!partsArray) {
        perror("Memory allocation for parts array failed");
        fclose(file);
        return NULL;
    }
 
    for (int i = 0; i < *numParts; i++) { 
        long currentPartSize = (i < *numParts - 1) ? MAX_PART_SIZE : (fileSize - i * MAX_PART_SIZE); 
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
        partsArray[i][bytesRead] = '\0';  // Null-terminate the string 
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
    int *partSizes = (int *)malloc(MAX_PART_SIZE * sizeof(int));
    if (!partSizes) {
        perror("Memory allocation for part sizes failed");
        MPI_Finalize();
        return 1;
    }

    char **textParts = NULL;
    if (rank == 0) {
        textParts = splitFileToStrings(FILENAME, &numParts, partSizes);
        if (!textParts) {
            free(partSizes);
            MPI_Finalize();
            return 1;
        }
    }

    // Broadcast the number of parts to all processes
    MPI_Bcast(&numParts, 1, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Bcast(partSizes, numParts, MPI_INT, 0, MPI_COMM_WORLD);

    int chunk_size = numParts / size;
    int start = rank * chunk_size;
    int end = (rank == size - 1) ? numParts : start + chunk_size;

    // Each process processes its chunk of text
    for (int i = start; i < end; i++) {
        char *part;
        if (rank == 0) {
            // Master process already has the text parts
            part = textParts[i];
        } else {
            // Receive part size from master
            int partSize = partSizes[i];
            part = (char *)malloc((partSize + 1) * sizeof(char));
            MPI_Recv(part, partSize + 1, MPI_CHAR, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        }

        // Process the part and free the memory
        processPart(part);
        if (rank != 0) {
            free(part);
        }
    }

    if (rank == 0) {
        for (int i = 0; i < numParts; i++) {
            free(textParts[i]);
        }
        free(textParts);
    }
    free(partSizes);

    // Reduction to find the most frequent word across all processes
    char localMostFreqWord[MAX_WORD_LEN];
    int localMaxCount;
    findMostFrequentWord(localMostFreqWord, &localMaxCount);

    char globalMostFreqWord[MAX_WORD_LEN];
    int globalMaxCount;
    MPI_Reduce(&localMaxCount, &globalMaxCount, 1, MPI_INT, MPI_MAX, 0, MPI_COMM_WORLD);

    if (rank == 0) {
        printf("Most frequent word: '%s' (appears %d times)\n", globalMostFreqWord, globalMaxCount);
    }

    cleanupHashTable();
    MPI_Finalize();
    return 0;
}
