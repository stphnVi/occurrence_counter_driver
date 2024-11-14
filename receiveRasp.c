#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <termios.h>
#include "display.h"

#define DEVICE "/dev/ttyAMA0"
#define BAUDRATE B9600
#define BUFFER_SIZE 256

int main()
{
    int serial_port = open(DEVICE, O_RDONLY);
    if (serial_port < 0)
    {
        perror("Error al abrir el puerto serial");
        return 1;
    }

    struct termios tty;
    if (tcgetattr(serial_port, &tty) != 0)
    {
        perror("Error al obtener los atributos del puerto serial");
        close(serial_port);
        return 1;
    }

    cfsetospeed(&tty, BAUDRATE);
    cfsetispeed(&tty, BAUDRATE);
    tty.c_cflag &= ~PARENB;
    tty.c_cflag &= ~CSTOPB;
    tty.c_cflag &= ~CSIZE;
    tty.c_cflag |= CS8;
    tty.c_cflag |= CREAD | CLOCAL;
    tty.c_iflag &= ~(IXON | IXOFF | IXANY);
    tty.c_iflag &= ~(ICANON | ECHO | ECHOE | ISIG);
    tty.c_oflag &= ~OPOST;

    if (tcsetattr(serial_port, TCSANOW, &tty) != 0)
    {
        perror("Error al configurar el puerto serial");
        close(serial_port);
        return 1;
    }

    init_display();

    char buffer[BUFFER_SIZE];
    int bytes_read;
    while (1)
    {
        bytes_read = read(serial_port, buffer, sizeof(buffer) - 1);
        if (bytes_read > 0)
        {
            buffer[bytes_read] = '\0';
            printf("Mensaje recibido: %s\n", buffer);
            animate_text(buffer); // Llama a animate_text
        }
    }

    close(serial_port);
    return 0;
}
// gcc -o receiveRasp receiveRasp.c test.c -lwiringPi