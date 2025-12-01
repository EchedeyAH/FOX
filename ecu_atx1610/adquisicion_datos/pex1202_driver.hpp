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
    IXPCI_IOCTL_ID_TIME_SPAN,
    // ... otros no necesarios por ahora
};

// Estructura para operaciones de registro
struct ixpci_reg {
    unsigned int id;        /* register's id */
    unsigned int value;     /* register's value for read/write */
    unsigned int sram_off;  /* sram offset for sram card */
    unsigned int id_offset; /* register's id and count it's offset */
    int mode;
};

// Macros IOCTL
#define IXPCI_READ_REG   _IOR(IXPCI_MAGIC_NUM, IXPCI_IOCTL_ID_READ_REG, ixpci_reg *)
#define IXPCI_WRITE_REG  _IOR(IXPCI_MAGIC_NUM, IXPCI_IOCTL_ID_WRITE_REG, ixpci_reg *)

// IDs de Registros (Mapping para PEX-1202L)
// Basado en ixpci.h y _pci1202.h

// Analog Input
#define IXPCI_AI           219 // IXPCI_ANALOG_INPUT_PORT
#define IXPCI_AICR         217 // IXPCI_ANALOG_INPUT_CHANNEL_CONTROL_REG
#define IXPCI_ADST         202 // IXPCI_AD_SOFTWARE_TRIGGER_REG
#define IXPCI_ADGCR        231 // IXPCI_AD_GAIN_CONTROL_AND_MULTIPLEXER_CONTROL_REGISTER
#define IXPCI_ADPR         232 // IXPCI_AD_POLLING_REGISTER

// Register Operation Modes
enum {
    IXPCI_RM_RAW,
    IXPCI_RM_NORMAL,
    IXPCI_RM_READY,
    IXPCI_RM_TRIGGER,
    IXPCI_RM_LAST_MODE
};
