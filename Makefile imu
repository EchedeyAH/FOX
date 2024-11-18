# MAKEFILE IMU FOX

# Macros:

.SUFFIXES: .o .c
.c.o:
	$(CC) $(CFLAGS) -c $<
CC = gcc
CFLAGS = -Wall
SRC = imu_datalog_fox.c
OBJ = imu_datalog_fox.o
NOMBRE = imu_datalog_fox
NOMBRE_G = imu_datalog_g

# Reglas implicitas:
imu_datalog_fox: $(OBJ)

# reglas explicitas:
all: $(OBJ)
	$(CC) $(CFLAGS) -o $(NOMBRE) $(SRC)
	$(CC) $(CFLAGS) -g -o $(NOMBRE_G) $(SRC)
clean:
	$(RM) $(OBJ) $(NOMBRE) $(NOMBRE_G)
debug:
	$(CC) $(CFLAGS) -g -o $(NOMBRE_G) $(SRC)
