#include "sendRasp.h"
#include <stdio.h>

int main()
{
    draw("TOLEBRIO\n");
    return 0;
}

/**
 * Consideraciones para utilizar draw():
 * 1. El string debe ser en letras mayusculas: se puede usar la función de la libreria parseString(string)
 * 2. Debe finalizar con un salto de linea \n
 * 
 * Compilar un archivo que utiliza la libreria: gcc program.c -L. -lsendRasp -o program
 * Esto vincula la libreria (.a) al archivo ejecutable
 * Hacer include de sendRasp.h
 *
 * Si hay cambios en la libreria:
 * 1. Compilar: gcc -c sendRasp.c -o sendRasp.o
 * 2. Crear biblioteca estatica: ar rcs libSendRasp.a sendRasp.o
 * 
 */