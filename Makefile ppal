# MAKEFILE PPAL ECU FOX

# Macros:

.SUFFIXES: .o .c
.c.o:
	$(CC) $(CFLAGS) $(MATH_FLAG) -c $<
CC = gcc
CFLAGS = -Wall
MATH_FLAG = -lm
SRC = ecu_fox.c err_fox.c can1_fox.c tad_fox.c tadAO_fox.c canRx_fox.c can_superv_fox.c gestionpot_fox.c ctrl_trac_estab_fox.c imu_fox.c
LIB_CAN = ./include/can.a
LIB_TAD = ./include/libUD-6.02.a
OBJ = ecu_fox.o err_fox.o can1_fox.o tad_fox.o tadAO_fox.o canRx_fox.o can_superv_fox.o gestionpot_fox.o ctrl_trac_estab_fox.o imu_fox.o
NOMBRE = ecu_fox
NOMBRE_G = ecu_fox_g

# Reglas implicitas:
ecu_fox: $(OBJ) $(LIB_CAN) $(LIB_TAD)

# reglas explicitas:
all: $(OBJ)
	$(CC) $(CFLAGS) -o $(NOMBRE) $(SRC) $(LIB_CAN) $(LIB_TAD) $(MATH_FLAG)
	$(CC) $(CFLAGS) -g -o $(NOMBRE_G) $(SRC) $(LIB_CAN) $(LIB_TAD) $(MATH_FLAG)
clean:
	$(RM) $(OBJ) $(NOMBRE) $(NOMBRE_G)
debug:
	$(CC) $(CFLAGS) -g -o $(NOMBRE_G) $(SRC) $(LIB_CAN) $(LIB_TAD) $(MATH_FLAG)



