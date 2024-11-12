# Nombre del ejecutable
TARGET = search_algorithm

# Compilador
CC = g++

# Flags de compilación
CFLAGS = -Wall -g

# Directorio de salida y archivos fuente
OUTPUT_DIR = /home/josue/Desktop/occurrence_counter_driver/Algorithm
SRC = $(OUTPUT_DIR)/search_algorithm.c

# Comando de construcción
$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $(OUTPUT_DIR)/$(TARGET) $(SRC)

# Limpieza
clean:
	rm -f $(OUTPUT_DIR)/$(TARGET)

# Ejecución
run: $(TARGET)
	./$(OUTPUT_DIR)/$(TARGET)

