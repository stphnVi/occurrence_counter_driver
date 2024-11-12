#include <stdio.h> 
#include <stdlib.h> 
#include <string.h> 
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
 
int main() { 
    int numParts; 

    int *partSizes = (int *)malloc(MAX_PART_SIZE * sizeof(int));
    if (!partSizes) { 
        perror("Memory allocation for part sizes failed"); 
        return 1; 
    } 
 
    char **textParts = splitFileToStrings(FILENAME, &numParts, partSizes); 
    if (!textParts) { 
        free(partSizes); 
        return 1; 
    } 
 
    // Process each part and count word frequencies 
    for (int i = 0; i < numParts; i++) { 
        processPart(textParts[i]); 
        free(textParts[i]);  // Free each part after processing 
    } 
 
    free(textParts); 
    free(partSizes); 
 
    // Find the most frequent word 
    char mostFreqWord[MAX_WORD_LEN]; 
    int maxCount; 
    findMostFrequentWord(mostFreqWord, &maxCount); 
 
    printf("Most frequent word: '%s' (appears %d times)\n", mostFreqWord, maxCount); 
 
    cleanupHashTable();  // Free the hash table memory 
    return 0; 
}