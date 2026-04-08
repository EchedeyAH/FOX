#include "ao_output.h"

#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <string.h>
#include <errno.h>
#include <stdint.h>
#include <stdio.h>

// Driver PEX-DA16 (ixpio)
#include "ixpio.h"

// Suposicion (segun requisitos): 4 canales AO, 0-10V, 14 bits.
#define AO_HW_CHANNELS 4
#define AO_DAC_MAX     16383
#define AO_VOLT_MAX    10.0f
#define AO_VOLT_MIN   -10.0f

static int g_ao_fd = -1;
static int g_ao_ready = 0;
static const char *g_ao_path = NULL;

static int ao_open_device(void)
{
    const char *paths[] = {"/dev/ixpio1", "/dev/ixpio0"};
    for (int i = 0; i < 2; ++i) {
        int fd = open(paths[i], O_RDWR);
        if (fd >= 0) {
            g_ao_path = paths[i];
            printf("[AO] Using device: %s\n", g_ao_path);
            return fd;
        }
        printf("ERROR opening %s errno=%d (%s)\n", paths[i], errno, strerror(errno));
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
    printf("AO DEBUG -> ch=%d, raw=%u\n", ch, (unsigned)code);
    ixpio_analog_t ao;
    memset(&ao, 0, sizeof(ao));
    ao.channel = ch;
    ao.data.u16 = (uint16_t)(code & 0x3FFF);
    if (ioctl(g_ao_fd, IXPIO_ANALOG_OUT, &ao) != 0) {
        perror("PEX-DA16 ioctl failed");
    }
}

void ao_init(void)
{
    if (g_ao_ready) return;

    g_ao_fd = ao_open_device();
    if (g_ao_fd < 0) {
        fprintf(stderr, "[AO] No se pudo abrir /dev/ixpio0 o /dev/ixpio1 (PEX-DA16).\n");
        g_ao_ready = 0;
        return;
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
    printf("AO DEBUG -> ch=%d, V=%.3f, DAC=%u\n", channel, voltage, (unsigned)code);
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
