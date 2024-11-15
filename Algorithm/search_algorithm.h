#ifndef PRUEBAS_H
#define PRUEBAS_H

#include <stdio.h>
#include <string.h>
#include <mpi.h>
#include <stdlib.h>

#define WORD_LENGTH 24    // Tamaño máximo de palabra
#define NUM_WORDS 100     // Número máximo de palabras
#define MAX_PART_SIZE 1024 // Tamaño máximo de cada parte

// Declaración de la función para contar palabras
void countWords(char *buffer, char words[][WORD_LENGTH], int counts[]);

// Declaración de la función para dividir el archivo en partes
void splitFileToParts(const char *filename, char **buffer, int *buffer_size, int rank);

// Declaración de la función para encontrar la palabra más frecuente
void findMostFrequent(char words[][WORD_LENGTH], int counts[], char *most_frequent, int *max_count);

#endif // PRUEBAS_H
