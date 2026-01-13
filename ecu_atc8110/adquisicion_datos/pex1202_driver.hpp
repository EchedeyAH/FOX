#pragma once

#include <sys/ioctl.h>
#include <linux/types.h>

// Definiciones extraídas de ixpci.h y _pci1202.h

#define IXPCI_MAGIC_NUM  0x26

// IOCTL Command IDs
enum {
    IXPCI_IOCTL_ID_RESET,
    IXPCI_IOCTL_ID_GET_INFO,
    IXPCI_IOCTL_ID_SET_SIG,
    IXPCI_IOCTL_ID_READ_REG,
    IXPCI_IOCTL_ID_WRITE_REG,
    IXPCI_IOCTL_ID_TIME_SPAN,    // 5
    IXPCI_IOCTL_ID_DI,           // 6
    IXPCI_IOCTL_ID_DO,           // 7
    IXPCI_IOCTL_ID_IRQ_ENABLE,   // 8
    IXPCI_IOCTL_ID_IRQ_DISABLE,  // 9
};

// Estructura para operaciones de registro
struct ixpci_reg {
    unsigned int id;        /* register's id */
    unsigned int value;     /* register's value for read/write */
    unsigned int sram_off;  /* sram offset for sram card */
    unsigned int id_offset; /* register's id and count it's offset */
    int mode;
};

// Macros IOCTL (Basadas en ixpci.h)
// Nota: Usamos void* para que la macro _IOR genere el tamaño de puntero (8 bytes en x64)
#define IXPCI_READ_REG   _IOR(IXPCI_MAGIC_NUM, IXPCI_IOCTL_ID_READ_REG, void *)
#define IXPCI_WRITE_REG  _IOR(IXPCI_MAGIC_NUM, IXPCI_IOCTL_ID_WRITE_REG, void *)
#define IXPCI_IOCTL_DI   _IOR(IXPCI_MAGIC_NUM, IXPCI_IOCTL_ID_DI, void *)
#define IXPCI_IOCTL_DO   _IOW(IXPCI_MAGIC_NUM, IXPCI_IOCTL_ID_DO, void *)

// IDs de Registros (Mapping para PEX-1202L)
// Basado en ixpci.h y _pci1202.h

// Analog Input
#define IXPCI_AI           219 // IXPCI_ANALOG_INPUT_PORT
#define IXPCI_AICR         217 // IXPCI_ANALOG_INPUT_CHANNEL_CONTROL_REG
#define IXPCI_ADST         202 // IXPCI_AD_SOFTWARE_TRIGGER_REG
#define IXPCI_ADGCR        231 // IXPCI_AD_GAIN_CONTROL_AND_MULTIPLEXER_CONTROL_REGISTER
#define IXPCI_ADPR         232 // IXPCI_AD_POLLING_REGISTER

// Analog Output
#define IXPCI_AO           220 // IXPCI_ANALOG_OUTPUT_PORT
#define IXPCI_AO0          222 // IXPCI_ANALOG_OUTPUT_CHANNEL_0
#define IXPCI_AO1          223 // IXPCI_ANALOG_OUTPUT_CHANNEL_1
#define IXPCI_AO2          224 // IXPCI_ANALOG_OUTPUT_CHANNEL_2
// Assuming AO3 follows. Caution: IXPCI_ENABLE_DISABLE_DA_CHANNEL is 225 in ixpci.h
// If PEX-DA16 is 16 channels, the driver method might be different (e.g. Channel Select + Data Write)
// For now, we define up to AO3. If AO3 calls 225, it might toggle Enable.
// Let's verify if we can use IXPCI_AO with channel selection instead.


// Register Operation Modes
enum {
    IXPCI_RM_RAW,
    IXPCI_RM_NORMAL,
    IXPCI_RM_READY,
    IXPCI_RM_TRIGGER,
    IXPCI_RM_LAST_MODE
};
