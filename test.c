#include <wiringPi.h>
#include <wiringPiSPI.h>
#include <stdio.h>
#include <stdint.h>

#define SPI_CHANNEL 0
#define SPI_SPEED 500000

// Function to send data to a specific register of the MAX7219
void max7219_send(uint8_t reg, uint8_t data)
{
    uint8_t buffer[2];
    buffer[0] = reg;  // Register address
    buffer[1] = data; // Data to send to the register
    wiringPiSPIDataRW(SPI_CHANNEL, buffer, 2);
}

// Define the pattern for letter 'A'
uint8_t letterA[8] = {
    0b00111100,
    0b01000010,
    0b10000001,
    0b10000001,
    0b11111111,
    0b10000001,
    0b10000001,
    0b10000001};

void display_letter(uint8_t *pattern)
{
    for (int row = 0; row < 8; row++)
    {
        max7219_send(row + 1, pattern[row]); // Send each row's pattern to the display
    }
}

int main()
{
    // Initialize WiringPi and SPI
    if (wiringPiSetup() == -1)
    {
        fprintf(stderr, "Error initializing WiringPi\n");
        return 1;
    }

    if (wiringPiSPISetup(SPI_CHANNEL, SPI_SPEED) == -1)
    {
        fprintf(stderr, "Error setting up SPI\n");
        return 1;
    }

    // Initialize MAX7219
    max7219_send(0x09, 0x00); // Decode mode: none
    max7219_send(0x0A, 0x03); // Intensity: medium
    max7219_send(0x0B, 0x07); // Scan limit: display all rows
    max7219_send(0x0C, 0x01); // Exit shutdown mode
    max7219_send(0x0F, 0x00); // Display test: off

    // Display the letter "A" on the matrix
    display_letter(letterA);

    return 0;
}
