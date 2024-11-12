#ifndef SEARCH_ALGORITHM_H
#define SEARCH_ALGORITHM_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

// Definición de constantes y macros
#define MAX_WORD_LEN 50
#define MAX_PART_SIZE (1024 * 1024)  // 1 MB máximo por parte
#define FILENAME "el_quijote.txt"    // Nombre del archivo de entrada
#define HASH_SIZE 10000              // Tamaño de la tabla hash

// Estructura para almacenar la frecuencia de las palabras
typedef struct {
    char word[MAX_WORD_LEN];
    int count;
} WordFreq;

// Declaraciones de funciones
void toLowercase(char *str);
unsigned int hash(const char *word);
void addWord(const char *word);
void processPart(const char *text);
void findMostFrequentWord(char *mostFreqWord, int *maxCount);
void cleanupHashTable();
char **splitFileToStrings(const char *filename, int *numParts, int *outPartSizes);

#endif // SEARCH_ALGORITHM_H
