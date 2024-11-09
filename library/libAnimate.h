#ifndef LIBANIMATE_H
#define LIBANIMATE_H

#include <stdint.h>

typedef struct
{
    char letter;        // Letra
    uint8_t pattern[8]; // patrón de la letra en 8 filas
} Letter;

extern Letter letters[];    // declaración de la letras

void animate_text(const char *text); // decalaración de la función para animar texto

#endif