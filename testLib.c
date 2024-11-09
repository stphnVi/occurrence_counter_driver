#include <wiringPi.h>
#include <wiringPiSPI.h>
#include <stdio.h>
#include "library/libAnimate.h"

#define SPI_CHANNEL 0
#define SPI_SPEED 500000

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

    max7219_init();

    animate_text("TOLEBRIO");

    return 0;
}
