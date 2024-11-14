#include <wiringPi.h>
#include <wiringPiSPI.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>

#define SPI_CHANNEL 0
#define SPI_SPEED 500000

typedef struct
{
    char letter;
    uint8_t pattern[8];
} Letter;

void max7219_send(uint8_t reg, uint8_t data)
{
    uint8_t buffer[2];
    buffer[0] = reg;
    buffer[1] = data;
    wiringPiSPIDataRW(SPI_CHANNEL, buffer, 2);
}

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
    int length = strlen(text);
    int screen_width = 8;
    int num_leds = 8;

    for (int i = 0; i < length; i++)
    {
        char current_char = text[i];
        int letter_index = -1;

        for (int j = 0; j < sizeof(letters) / sizeof(letters[0]); j++)
        {
            if (letters[j].letter == current_char)
            {
                letter_index = j;
                break;
            }
        }

        if (letter_index != -1)
        {
            for (int offset = -screen_width; offset <= num_leds; offset++)
            {
                uint8_t shifted_letter[8] = {0};
                for (int row = 0; row < 8; row++)
                {
                    shifted_letter[row] = (offset < 0) ? letters[letter_index].pattern[row] << (-offset) : letters[letter_index].pattern[row] >> offset;
                }
                display_letter(shifted_letter);
                delay(100);
            }
            delay(100);
        }
    }
}

void init_display()
{
    wiringPiSetup();
    wiringPiSPISetup(SPI_CHANNEL, SPI_SPEED);
    max7219_send(0x09, 0x00);
    max7219_send(0x0A, 0x03);
    max7219_send(0x0B, 0x07);
    max7219_send(0x0C, 0x01);
    max7219_send(0x0F, 0x00);
}
