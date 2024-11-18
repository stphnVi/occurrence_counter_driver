#include <stdint.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/spi/spi.h>

#define DRIVER_NAME "max7219_driver"
#define DEVICE_NAME "max7219_device"

// Definición del bus SPI y del dispositivo
static struct spi_device *max7219_spi_device;

// Definir el canal SPI y la velocidad
#define SPI_CHANNEL 0
#define SPI_SPEED 500000

// Función para enviar datos a un registro específico del MAX7219
void max7219_send(uint8_t reg, uint8_t data)
{
    uint8_t buffer[2];
    buffer[0] = reg;  // Dirección del registro
    buffer[1] = data; // Dato a enviar

    // Usar el driver SPI del kernel para enviar datos
    spi_write(max7219_spi_device, buffer, 2);
}

// Función de inicialización del display (MAX7219)
void max7219_init()
{
    max7219_send(0x09, 0x00); // Modo de decodificación: ninguno
    max7219_send(0x0A, 0x03); // Intensidad: media
    max7219_send(0x0B, 0x07); // Límite de escaneo: mostrar todas las filas
    max7219_send(0x0C, 0x01); // Salir del modo de apagado
    max7219_send(0x0F, 0x00); // Prueba del display: apagada
}

// Función del driver para configurar el dispositivo SPI
static int max7219_probe(struct spi_device *spi)
{
    pr_info("Max7219 driver probed: dispositivo encontrado\n");

    // Guardamos la referencia al dispositivo SPI
    max7219_spi_device = spi;

    // Inicializamos el MAX7219
    max7219_init();

    return 0;
}

// Función para eliminar el driver
static int max7219_remove(struct spi_device *spi)
{
    pr_info("Max7219 driver removed: dispositivo desconectado\n");
    return 0;
}

// Estructura para registrar el driver SPI
static struct spi_driver max7219_driver = {
    .driver = {
        .name = DRIVER_NAME,
        .owner = THIS_MODULE,
    },
    .probe = max7219_probe,
    .remove = max7219_remove,
};

// Registro del driver en el sistema
module_spi_driver(max7219_driver);

