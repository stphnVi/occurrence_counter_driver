#ifndef MPI_WORD_COUNT_H 
#define MPI_WORD_COUNT_H 
 
#include <stdio.h> 
 
#define HASH_SIZE 10000 
#define WORD_SIZE 50 
#define CHUNK_SIZE 1024  // Size of each text chunk to be processed by each process 
 
// Structure for each entry in the hash table 
typedef struct Entry { 
    char word[WORD_SIZE]; 
    int count; 
    struct Entry *next; 
} Entry; 
 
// Hash table declaration 
extern Entry *hashTable[HASH_SIZE]; 
 
// Function prototypes 
 
/** 
 * Computes the hash value for a given word. 
 *  
 * @param word The word to hash. 
 * @return The computed hash value. 
 */ 
unsigned int hash(const char *word); 
 
/** 
 * Inserts a word into the hash table, or increments its count if it already exists. 
 *  
 * @param word The word to insert or update. 
 */ 
void insertWord(const char *word); 
 
/** 
 * Clears the hash table and frees allocated memory. 
 */ 
void clearHashTable(); 
 
/** 
 * Processes a text chunk and counts the frequency of each word within it. 
 *  
 * @param chunk The chunk of text to process. 
 */ 
void processChunk(char *chunk); 
 
/** 
 * Finds the most frequent word in the local hash table of a process. 
 *  
 * @param mostFrequentWord Output parameter to store the most frequent word. 
 * @param highestCount Output parameter to store the count of the most frequent word. 
 */ 
void findMostFrequentWordLocal(char *mostFrequentWord, int *highestCount); 
 
#endif  // MPI_WORD_COUNT_H