// search_algorithm.h

#ifndef SEARCH_ALGORITHM_H
#define SEARCH_ALGORITHM_H

#include <mpi.h>

// Define the maximum word length
#define MAX_WORD_LEN 256
// Define the hash table size for word frequencies
#define HASH_SIZE 10000
//Define the location of the txt
#define FILENAME "el_quijote.txt" 

// Structure to store each word and its frequency
typedef struct {
    char word[MAX_WORD_LEN];
    int count;
} WordFreq;

// Functions for word frequency processing

/**
 * Converts a given string to lowercase.
 * @param str The string to convert to lowercase.
 */
void toLowercase(char *str);

/**
 * Hash function to compute an index for a word in the hash table.
 * @param word The word to hash.
 * @return The computed hash index.
 */
unsigned int hash(const char *word);

/**
 * Adds a word to the local hash table or increments its count if it already exists.
 * @param word The word to add or update in the hash table.
 */
void addWord(const char *word);

/**
 * Processes a given text part to count the frequencies of each word.
 * @param text The text part to process.
 */
void processPart(const char *text);

/**
 * Combines word frequencies from each process, gathers them at the root process,
 * and finds the most frequent word globally.
 * @param rank The rank of the current MPI process.
 * @param size The total number of MPI processes.
 */
void gatherAndFindMostFrequentWord(int rank, int size);

/**
 * Adds a word and its count to the global hash table for consolidation at the root.
 * @param globalHashTable The global hash table at the root process.
 * @param word The word to add to the hash table.
 * @param count The frequency count of the word.
 */
void addWordToGlobalHash(WordFreq *globalHashTable, const char *word, int count);

/**
 * Splits a file into a specified number of parts.
 * @param filename The name of the file to split.
 * @param numParts The number of parts to split the file into.
 * @param outPartSizes Array to store the size of each part.
 * @return An array of strings where each entry is a part of the file.
 */
char **splitFileToStrings(const char *filename, int numParts, int *outPartSizes);

void cleanupHashTable();

#endif // SEARCH_ALGORITHM_H
