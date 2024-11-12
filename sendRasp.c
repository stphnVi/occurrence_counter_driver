#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <errno.h>

int configurar_serial(const char *puerto_serial) {
    int serial_fd = open(puerto_serial, O_RDWR | O_NOCTTY | O_NDELAY);
    if (serial_fd == -1) {
        perror("Error abriendo el puerto serial");
        return -1;
    }

    struct termios opciones;
    tcgetattr(serial_fd, &opciones);
    cfsetispeed(&opciones, B9600);
    cfsetospeed(&opciones, B9600);
    opciones.c_cflag |= (CLOCAL | CREAD);
    opciones.c_cflag &= ~PARENB;
    opciones.c_cflag &= ~CSTOPB;
    opciones.c_cflag &= ~CSIZE;
    opciones.c_cflag |= CS8;
    opciones.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
    opciones.c_iflag &= ~(IXON | IXOFF | IXANY);
    opciones.c_oflag &= ~OPOST;
    tcsetattr(serial_fd, TCSANOW, &opciones);

    return serial_fd;
}

int main() {
    const char *puerto = "/dev/ttyRaspberryPi";  // Cambia el puerto según tu sistema
    int serial_fd = configurar_serial(puerto);
    if (serial_fd == -1) {
        return 1;
    }

    const char *mensaje = "Hola Raspberry Pi";
    int bytes_enviados = write(serial_fd, mensaje, strlen(mensaje));
    if (bytes_enviados < 0) {
        perror("Error enviando mensaje");
        close(serial_fd);
        return 1;
    }
    printf("Mensaje enviado: %s\n", mensaje);

    close(serial_fd);
    return 0;
}
