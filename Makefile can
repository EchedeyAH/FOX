# MAKEFILE CAN 2 FOX

# Macros:

.SUFFIXES: .o .c
.c.o:
	$(CC) $(CFLAGS) -c $<
CC = gcc
CFLAGS = -Wall
SRC = can2_fox.c
LIB_CAN = ./include/can.a
OBJ = can2_fox.o
NOMBRE = can2_fox
NOMBRE_G = can2_fox_g


# Reglas implicitas:
can2_fox: $(OBJ) $(LIB_CAN)


# reglas explicitas:
all: $(OBJ)
	$(CC) $(CFLAGS) -o $(NOMBRE) $(SRC) $(LIB_CAN)
	$(CC) $(CFLAGS) -g -o $(NOMBRE_G) $(SRC) $(LIB_CAN)
clean:
	$(RM) $(OBJ) $(NOMBRE) $(NOMBRE_G)
debug:
	$(CC) $(CFLAGS) -g -o $(NOMBRE_G) $(SRC) $(LIB_CAN)
