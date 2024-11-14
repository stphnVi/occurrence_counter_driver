#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <termios.h>

#define DEVICE "/dev/ttyRaspberryPi" // Puerto serial de la computadora (cambia si es necesario)
#define BAUDRATE B9600               // Velocidad de baudios

int main()
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
    tty.c_cflag |= CS8;                             // 8 bits de datos
                                                    // tty.c_cflag &= ~CRTSCTS; // Deshabilitar control de flujo por hardware
    tty.c_cflag |= CREAD | CLOCAL;                  // Habilitar la lectura y establecer la conexión local
    tty.c_iflag &= ~(IXON | IXOFF | IXANY);         // Sin control de flujo por software
    tty.c_iflag &= ~(ICANON | ECHO | ECHOE | ISIG); // Deshabilitar entrada canónica y retroalimentación
    tty.c_oflag &= ~OPOST;                          // Deshabilitar salida procesada

    if (tcsetattr(serial_port, TCSANOW, &tty) != 0)
    {
        perror("Error al configurar el puerto serial");
        close(serial_port);
        return 1;
    }

    // Enviar datos
    char message[] = "TOLEDO\n";
    int message_len = sizeof(message) - 1;
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
