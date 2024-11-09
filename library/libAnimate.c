#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include "libAnimate.h"

#define DEVICE_FILE "/dev/max7219_device" //Hay que generar este archivo desde linux

// Mapeo de las letras al display 8x8
Letter letters[] = {
    {'H', {0xc3, 0xc3, 0xc3, 0xff, 0xff, 0xc3, 0xc3, 0xc3}},
    {'O', {0x3c, 0x7e, 0xc3, 0xc3, 0xc3, 0xc3, 0x7e, 0x3c}},
    {'L', {0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xff, 0x7e}},
    {'A', {0x3c, 0x7e, 0xc3, 0xc3, 0xff, 0xff, 0xc3, 0xc3}},
    {'T', {0xff, 0xff, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18}},
    {'E', {0x7f, 0xff, 0xc0, 0xf8, 0xf8, 0xc0, 0xff, 0x7f}},
    {'D', {0xfc, 0xfe, 0xc7, 0xc3, 0xc3, 0xc7, 0xfe, 0xfc}},
    {'R', {0xf8, 0xfc, 0xc6, 0xfe, 0xfc, 0xce, 0xc7, 0xc3}},
    {'I', {0xff, 0xff, 0x18, 0x18, 0x18, 0x18, 0xff, 0xff}},
    {'B', {0xf8, 0xfc, 0xce, 0xfe, 0xfc, 0xc6, 0xfe, 0xfc}}};

// Función para mostrar las letras en el display
void display_letter(uint8_t *pattern)
{
    for (int row = 0; row < 8; row++)
    {

        max7219_send(row + 1, pattern[row]);
    }
}

// Función para animar el texto
void animate_text(const char *text)
{
    int length = strlen(text); // Longitud del string a animar
    int screen_width = 8;      // MAX7219 tiene 8 columnas
    int num_leds = 8;          // Número de LEDs

    while (1)
    {
        // Recorre cada carácter en el texto dado (por ejemplo, "HOLA")
        for (int i = 0; i < length; i++)
        {
            char current_char = text[i];

            // Buscar el índice del carácter en el mapeo de letras
            int letter_index = -1;
            for (int j = 0; j < sizeof(letters) / sizeof(letters[0]); j++)
            {
                if (letters[j].letter == current_char)
                {
                    letter_index = j;
                    break;
                }
            }

            // mapeon
            if (letter_index != -1)
            {
                // Desplaza izquierda hacia la derecha
                for (int offset = -screen_width; offset <= num_leds; offset++)
                {
                    uint8_t shifted_letter[8] = {0};

                    for (int row = 0; row < 8; row++)
                    {
                        if (offset < 0)
                        {
                            // desde el borde izquierdo
                            shifted_letter[row] = letters[letter_index].pattern[row] << (-offset);
                        }
                        else
                        {
                            // desplaza hacia la derecha
                            shifted_letter[row] = letters[letter_index].pattern[row] >> offset;
                        }
                    }

                    display_letter(shifted_letter);
                    delay(100); // Controla la velocidad de desplazamiento
                }

                // Retardo entre letras
                delay(100);
            }
        }
    }
}