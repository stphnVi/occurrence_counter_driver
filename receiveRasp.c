#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <errno.h>

int configurar_uart(const char *puerto_uart) {
    int uart_fd = open(puerto_uart, O_RDWR | O_NOCTTY | O_NDELAY);
    if (uart_fd == -1) {
        perror("Error abriendo el UART");
        return -1;
    }

    struct termios opciones;
    tcgetattr(uart_fd, &opciones);
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
    tcsetattr(uart_fd, TCSANOW, &opciones);

    return uart_fd;
}

int main() {
    const char *puerto = "/dev/serial0";  // Puerto UART en Raspberry Pi
    int uart_fd = configurar_uart(puerto);
    if (uart_fd == -1) {
        return 1;
    }

    char buffer[256];
    memset(buffer, 0, sizeof(buffer));
    int bytes_recibidos = read(uart_fd, buffer, sizeof(buffer) - 1);
    if (bytes_recibidos < 0) {
        perror("Error recibiendo mensaje");
        close(uart_fd);
        return 1;
    } else {
        printf("Mensaje recibido: %s\n", buffer);
    }

    close(uart_fd);
    return 0;
}
