#include "ao_output.h"

#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <string.h>
#include <errno.h>
#include <stdint.h>
#include <stdio.h>

// Driver PEX-DA16 (ixpio)
#include "ixpci.h"

// Registros ixpio/ixpci usados por el driver (ver tools/pex_da16_auto_mapper)
#define AO_RANGE_REG   291  // IXPCI_AO_CONFIGURATION
#define AO_ENABLE_REG  225  // IXPCI_ENABLE_DISABLE_DA_CHANNEL
#define AO_CH0_REG     222  // IXPCI_AO0
#define AO_CH1_REG     223  // IXPCI_AO1
#define AO_CH2_REG     224  // IXPCI_AO2
#define AO_FALLBACK    220  // Fallback: (ch<<12 | dac)

// Suposicion (segun requisitos): 4 canales AO, 0-10V, 14 bits.
#define AO_HW_CHANNELS 4
#define AO_DAC_MAX     16383
#define AO_VOLT_MAX    10.0f
#define AO_VOLT_MIN   -10.0f

static int g_ao_fd = -1;
static int g_ao_ready = 0;

static int ao_open_device(void)
{
    const char *paths[] = {"/dev/ixpio1", "/dev/ixpio0"};
    for (int i = 0; i < 2; ++i) {
        int fd = open(paths[i], O_RDWR);
        if (fd >= 0) return fd;
    }
    return -1;
}

static inline uint16_t voltageToDAC(float voltage)
{
    if (voltage > AO_VOLT_MAX) voltage = AO_VOLT_MAX;
    if (voltage < AO_VOLT_MIN) voltage = AO_VOLT_MIN;
    return (uint16_t)((voltage + AO_VOLT_MAX) * ((float)AO_DAC_MAX / 20.0f));
}

static void ao_write_raw(int ch, uint32_t code)
{
    ixpci_reg_t reg;
    memset(&reg, 0, sizeof(reg));

    if (ch == 0) reg.id = AO_CH0_REG;
    else if (ch == 1) reg.id = AO_CH1_REG;
    else if (ch == 2) reg.id = AO_CH2_REG;
    else {
        reg.id = AO_FALLBACK;
        reg.value = ((uint32_t)ch << 12) | (code & 0x3FFF);
        reg.mode = IXPCI_RM_NORMAL;
        (void)ioctl(g_ao_fd, IXPCI_WRITE_REG, &reg);
        return;
    }

    reg.value = code;
    reg.mode = IXPCI_RM_NORMAL;
    (void)ioctl(g_ao_fd, IXPCI_WRITE_REG, &reg);
}

void ao_init(void)
{
    if (g_ao_ready) return;

    g_ao_fd = ao_open_device();
    if (g_ao_fd < 0) {
        fprintf(stderr, "[AO] No se pudo abrir /dev/ixpio0/1 (PEX-DA16).\n");
        g_ao_ready = 0;
        return;
    }

    ixpci_reg_t reg;
    memset(&reg, 0, sizeof(reg));

    // Configurar rango AO (0 = 0-10V segun driver)
    reg.id = AO_RANGE_REG;
    reg.value = 0;
    reg.mode = IXPCI_RM_NORMAL;
    if (ioctl(g_ao_fd, IXPCI_WRITE_REG, &reg) < 0) {
        fprintf(stderr, "[AO] Fallo configurando rango AO: errno=%d\n", errno);
    }

    // Habilitar AO0..AO3
    reg.id = AO_ENABLE_REG;
    reg.value = 0x0F;
    reg.mode = IXPCI_RM_NORMAL;
    if (ioctl(g_ao_fd, IXPCI_WRITE_REG, &reg) < 0) {
        fprintf(stderr, "[AO] Fallo habilitando canales AO: errno=%d\n", errno);
    }

    // Poner AO0..AO3 a 0V
    for (int i = 0; i < AO_HW_CHANNELS; ++i) {
        ao_write_raw(i, 0);
    }

    g_ao_ready = 1;
}

void ao_set_channel(int channel, float voltage)
{
    if (!g_ao_ready) return;
    if (channel < 0 || channel >= AO_HW_CHANNELS) return;

    uint16_t code = voltageToDAC(voltage);
    ao_write_raw(channel, code);
}

void ao_set_all(const float *values, int count)
{
    if (!g_ao_ready || values == NULL) return;
    if (count < 0) return;

    int n = count;
    if (n > AO_HW_CHANNELS) n = AO_HW_CHANNELS;

    for (int i = 0; i < n; ++i) {
        ao_set_channel(i, values[i]);
    }
}
