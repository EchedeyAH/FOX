#macros
MAKE = make
DIR1 = proceso_ppal
DIR2 = proceso_can2
DIR3 = proceso_imu

# Compilar:

build:
	$(MAKE) -C $(DIR1)
	$(MAKE) -C $(DIR2)
	$(MAKE) -C $(DIR3)

all:
	$(MAKE) -C $(DIR1) all
	$(MAKE) -C $(DIR2) all
	$(MAKE) -C $(DIR3) all

clean:
	$(MAKE) -C $(DIR1) clean
	$(MAKE) -C $(DIR2) clean
	$(MAKE) -C $(DIR3) clean