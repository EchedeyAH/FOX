# Variables
CC := gcc
CFLAGS := -Wall -Wextra -g
SRC_DIR := src
BUILD_DIR := build
TEST_DIR := tests
EXECUTABLE := my_program

# Archivos fuente y objeto
SRC_FILES := $(wildcard $(SRC_DIR)/*.c)
OBJ_FILES := $(patsubst $(SRC_DIR)/%.c, $(BUILD_DIR)/%.o, $(SRC_FILES))

# Regla principal
all: $(EXECUTABLE)

# Regla para crear el ejecutable
$(EXECUTABLE): $(OBJ_FILES)
	$(CC) -o $@ $^

# Regla para compilar archivos fuente en archivos objeto
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Crear el directorio de construcción si no existe
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

# Regla para ejecutar pruebas unitarias
test: $(EXECUTABLE)
	@echo "Ejecutando pruebas..."
	$(TEST_DIR)/run_tests.sh

# Regla para limpiar archivos generados
clean:
	rm -rf $(BUILD_DIR)/*.o $(EXECUTABLE)

# Regla para instalar el ejecutable
install: $(EXECUTABLE)
	install -m 755 $(EXECUTABLE) /usr/local/bin/

# Regla para ejecutar el programa
run: $(EXECUTABLE)
	./$(EXECUTABLE)

# Declarar reglas como .PHONY
.PHONY: all clean install test run