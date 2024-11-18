# Variables
CC = gcc
CFLAGS = -Wall -Wextra -Iinclude -Idrivers/include -g
SRC_DIR = src
BUILD_DIR = build
TEST_DIR = tests
EXECUTABLE = $(BUILD_DIR)/project_executable

# Listas de archivos fuente
SRC_FILES = $(wildcard $(SRC_DIR)/*.c) \
            $(wildcard $(SRC_DIR)/can_module/*.c) \
            $(wildcard $(SRC_DIR)/usb_module/*.c) \
            $(wildcard $(SRC_DIR)/scheduler_module/*.c) \
            $(wildcard $(SRC_DIR)/rt_tasks/*.c)

# Archivos objeto
OBJ_FILES = $(patsubst $(SRC_DIR)/%.c,$(BUILD_DIR)/%.o,$(SRC_FILES))

# Reglas
all: $(EXECUTABLE)

$(EXECUTABLE): $(OBJ_FILES)
	$(CC) -o $@ $^

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Reglas para pruebas
test: $(TEST_DIR)/test_can_module.o $(TEST_DIR)/test_usb_module.o $(TEST_DIR)/test_rt_scheduler.o \
      $(TEST_DIR)/test_task_comm.o $(TEST_DIR)/test_task_sensor.o
	$(CC) -o $(BUILD_DIR)/test_executable $^

$(TEST_DIR)/%.o: $(TEST_DIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

# Limpieza
clean:
	rm -rf $(BUILD_DIR)/*.o $(EXECUTABLE) $(BUILD_DIR)/test_executable

# Instalación (opcional)
install: all
	@echo "Instalando el ejecutable en /usr/local/bin"
	cp $(EXECUTABLE) /usr/local/bin/

# Ejecución del programa
run: all
	./$(EXECUTABLE)

.PHONY: all clean install test run