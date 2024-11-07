#include <wiringPi.h>
#include <wiringPiSPI.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>

// Definir el canal SPI y la velocidad
#define SPI_CHANNEL 0
#define SPI_SPEED 500000

typedef struct
{
    char letter;        // Letra
    uint8_t pattern[8]; // patrón de la letra en 8 filas
} Letter;

// Función para enviar datos a un registro específico del MAX7219
void max7219_send(uint8_t reg, uint8_t data)
{
    uint8_t buffer[2];
    buffer[0] = reg;  // Dirección del registro
    buffer[1] = data; // Dato a enviar
    wiringPiSPIDataRW(SPI_CHANNEL, buffer, 2);
}

// Mapeo
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

void display_letter(uint8_t *pattern)
{
    for (int row = 0; row < 8; row++)
    {

        max7219_send(row + 1, pattern[row]);
    }
}

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

int main()
{
    // Inicializar WiringPi y SPI
    if (wiringPiSetup() == -1)
    {
        fprintf(stderr, "Error al inicializar WiringPi\n");
        return 1;
    }

    if (wiringPiSPISetup(SPI_CHANNEL, SPI_SPEED) == -1)
    {
        fprintf(stderr, "Error al configurar SPI\n");
        return 1;
    }

    // Inicializar MAX7219
    max7219_send(0x09, 0x00); // Modo de decodificación: ninguno
    max7219_send(0x0A, 0x03); // Intensidad: media
    max7219_send(0x0B, 0x07); // Límite de escaneo: mostrar todas las filas
    max7219_send(0x0C, 0x01); // Salir del modo de apagado
    max7219_send(0x0F, 0x00); // prueba del display: apagada

    animate_text("TOLEBRIO");

    return 0;
}
