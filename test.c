#include <wiringPi.h>
#include <wiringPiSPI.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>

// Definir el canal SPI y la velocidad
#define SPI_CHANNEL 0
#define SPI_SPEED 500000

// Estructura para almacenar el patrón de una letra
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
    {'H', {0x82, 0x82, 0xFE, 0x82, 0x82, 0x82, 0x82, 0x82}},
    {'O', {0xFC, 0x82, 0x82, 0x82, 0x82, 0x82, 0x82, 0xFC}},
    {'L', {0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0xFC}},
    {'A', {0x3C, 0x22, 0x41, 0x41, 0x7F, 0x41, 0x41, 0x41}}};

void display_letter(uint8_t *pattern)
{
    for (int row = 0; row < 8; row++)
    {
        max7219_send(row + 1, pattern[row]);
    }
}

// Función para animar la palabra "HOLA" con letras desplazándose
void animate_text()
{
    int length = sizeof(letters) / sizeof(letters[0]); // Número de letras en el array

    int offset;
    int screen_width = 8; // MAX7219 tiene 8 columnas
    int num_leds = 8;     // Número de LEDs

    while (1)
    {
        // Desplazar cada letra de "HOLA"
        for (int i = 0; i < length; i++)
        {

            for (offset = -num_leds; offset < num_leds + screen_width; offset++)
            {
                uint8_t shifted_letter[8] = {0};

                for (int row = 0; row < 8; row++)
                {
                    if (offset >= 0 && offset < num_leds)
                    {
                        shifted_letter[row] = letters[i].pattern[row] >> offset;
                    }
                    else if (offset < 0)
                    {
                        shifted_letter[row] = 0;
                    }
                    else
                    {
                        shifted_letter[row] = 0;
                    }
                }

                display_letter(shifted_letter);
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

    animate_text();

    return 0;
}
