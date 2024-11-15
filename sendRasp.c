#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <termios.h>
#include <string.h>
#include "sendRasp.h"

#define DEVICE "/dev/ttyRaspberryPi" // Puerto serial de la computadora
#define BAUDRATE B9600               // baudios

int draw(const char *message)
{
    int serial_port = open(DEVICE, O_RDWR);

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

    // Configuración de los parámetros del puerto serial
    cfsetospeed(&tty, BAUDRATE);
    cfsetispeed(&tty, BAUDRATE);
    tty.c_cflag &= ~PARENB; // Sin paridad
    tty.c_cflag &= ~CSTOPB; // 1 bit de stop
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

    // Enviar datos
    int message_len = strlen(message);
    int bytes_written = write(serial_port, message, message_len);

    if (bytes_written < 0)
    {
        perror("Error al escribir en el puerto serial");
        close(serial_port);
        return 1;
    }

    printf("Mensaje enviado: %s", message);

    close(serial_port);
    return 0;
}
