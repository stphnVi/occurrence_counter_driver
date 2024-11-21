# Proyecto II-Sistemas Operativos: Sistema de Procesamiento de Texto con Visualización

## Resumen

Este proyecto documenta el diseño y desarrollo de un sistema de multiprocesamiento para realizar búsquedas de ocurrencias de palabras en textos en inglés. El algoritmo toma como entrada un texto, calcula la palabra que tenga mas frecuencia en el texto y la muestra en una matriz LED 8x8 conectada a una Raspberry Pi 3 B+ mediante un módulo USB FT232.

El núcleo del sistema incluye un controlador (driver) desarrollado específicamente para Linux, que permite una comunicación eficiente entre el sistema operativo y el hardware. Este driver gestiona la comunicación UART para el envío de datos . Además, se implementó una biblioteca para facilitar la interacción con el hardware y la visualización de resultados.

El proyecto utiliza un enfoque de clúster para aprovechar el multiprocesamiento, dividiendo las tareas entre una máquina principal (master) y dos nodos esclavos (slaves). También integra criptografía de curvas elípticas (ECC) utilizando Curve25519 para garantizar comunicaciones seguras.

---

## Hardware

- Raspberry Pi 3 B+
- Matriz LED 8x8 con módulo MAX7219CNG integrado
- Módulo USB FT232

---

## Software

- Sistema operativo: Ubuntu 22.04.5
- Kernel: 6.8.0-48-generic
- Compilador: gcc 11.3.0
- Biblioteca de control de hardware: WiringPi
- Criptografía: openssl

---

## Daemon en la Raspberry Pi (opcional)

El sistema incluye un demonio que se ejecuta en segundo plano en la Raspberry Pi. Este demonio tiene la siguiente función:

- Gestionar la recepción de datos desde el clúster mediante UART.

### Ubicación del archivo del demonio

El archivo del demonio se encontrara en la siguiente dirección desde el RaspberryPi:  
`/etc/systemd/system/receive_rasp.service`
y tendra el siguiente contenido:

```ini
[Unit]
Description=Receive Master string Service

[Service]
ExecStart=<your path>/occurrence_counter_driver/Library/receiveRasp
TimeoutSec=15s
Restart=always
RestartSec=10s
User=<your user>

[Install]
WantedBy=multi-user.target


```

luego activarlo con:

`sudo systemctl enable receive_rasp.service`

`sudo systemctl start receive_rasp.service`
