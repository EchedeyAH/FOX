/*
 * Copia exacta de read_channels.c, pero compilable como C++
 * para verificar que funciona en el entorno del proyecto.
 */

#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <time.h>
#include <math.h>
#include "adquisicion_datos/ixpci.h"

/* ---------------------------------------------------------------
 * PEX-1202L - Codigos de configuracion (manual pag. 60)
 * -------------------------------------------------------------- */
#define AD_CONFIG_UNIPOLAR_5V    0x09   /* 0 ~ +5 V  */
#define AD_CONFIG_BIPOLAR_5V     0x00   /* -5 ~ +5 V */
#define AD_CHANNEL_DUMMY         15     /* AI_15, pin 16, tied to A.GND */

/* Timeouts */
#define HANDSHAKE_TIMEOUT_NS     100000000L   /* 100 ms */
#define ADC_READY_TIMEOUT_NS     100000000L   /* 100 ms */

/* ---------------------------------------------------------------
 * Definicion de canales
 * -------------------------------------------------------------- */
typedef struct {
    int   channel;       /* canal ADC (0..31)          */
    int   pin;           /* pin fisico DB37             */
    int   config_code;   /* AD_CONFIG_UNIPOLAR/BIPOLAR  */
    float v_min;         /* voltaje minimo del rango    */
    float v_max;         /* voltaje maximo del rango    */
    const char* name;    /* nombre de la señal          */
} channel_def_t;

static channel_def_t channels[] = {
    /*  ch  pin  config                  vmin   vmax   nombre              */
    {  1,   2,  AD_CONFIG_UNIPOLAR_5V,   0.0f,  5.0f, "ACELERADOR"              },
    {  2,   3,  AD_CONFIG_BIPOLAR_5V,   -5.0f,  5.0f, "CORRIENTE_EJE_DELANTERO" },
    {  3,   4,  AD_CONFIG_UNIPOLAR_5V,   0.0f,  5.0f, "FRENO"                   },
    {  4,   5,  AD_CONFIG_UNIPOLAR_5V,   0.0f,  5.0f, "VOLANTE"                 },
    {  5,   6,  AD_CONFIG_BIPOLAR_5V,   -5.0f,  5.0f, "CORRIENTE_EJE_TRASERO"   },
    {  6,   7,  AD_CONFIG_UNIPOLAR_5V,   0.0f,  5.0f, "SUSPENSION_TRASERA_IZQ"  },
    {  8,   9,  AD_CONFIG_UNIPOLAR_5V,   0.0f,  5.0f, "SUSPENSION_TRASERA_DER"  },
    { 10,  11,  AD_CONFIG_UNIPOLAR_5V,   0.0f,  5.0f, "SUSPENSION_DELANTERA_IZQ"},
    { 12,  13,  AD_CONFIG_UNIPOLAR_5V,   0.0f,  5.0f, "SUSPENSION_DELANTERA_DER"},
};

#define NUM_CHANNELS  (int)(sizeof(channels) / sizeof(channels[0]))

/* ---------------------------------------------------------------
 * elapsed_ns
 * -------------------------------------------------------------- */
static long elapsed_ns(struct timespec *start, struct timespec *end)
{
    return (end->tv_sec  - start->tv_sec)  * 1000000000L
         + (end->tv_nsec - start->tv_nsec);
}

/* ---------------------------------------------------------------
 * pic_control - handshake con el controlador MagicScan
 * -------------------------------------------------------------- */
static int pic_control(int fd, int cmd)
{
    ixpci_reg_t reg;
    struct timespec t0, tn;

    /* Recovery si handshake esta bajo */
    reg.id = IXPCI_SR;
    if (ioctl(fd, IXPCI_READ_REG, &reg) < 0) {
        perror("ioctl READ_REG SR (recovery check)");
        return -1;
    }
    if ((reg.value & 0x04) == 0) {
        reg.id    = IXPCI_CR;
        reg.value = 0xFFFF;
        if (ioctl(fd, IXPCI_WRITE_REG, &reg) < 0) {
            perror("ioctl WRITE_REG CR (recovery)");
            return -1;
        }
    }

    /* Esperar handshake alto */
    reg.id = IXPCI_SR;
    clock_gettime(CLOCK_MONOTONIC, &t0);
    do {
        if (ioctl(fd, IXPCI_READ_REG, &reg) < 0) {
            perror("ioctl READ_REG SR (wait high pre-cmd)");
            return -1;
        }
        clock_gettime(CLOCK_MONOTONIC, &tn);
        if (elapsed_ns(&t0, &tn) > HANDSHAKE_TIMEOUT_NS) {
            fprintf(stderr, "Timeout handshake alto (pre-cmd)\n");
            return -1;
        }
    } while ((reg.value & 0x04) == 0);

    /* PIC low */
    int config = cmd & 0xDFFF;
    reg.id    = IXPCI_CR;
    reg.value = config;
    if (ioctl(fd, IXPCI_WRITE_REG, &reg) < 0) {
        perror("ioctl WRITE_REG CR (PIC low)");
        return -1;
    }

    /* Esperar handshake bajo */
    reg.id = IXPCI_SR;
    clock_gettime(CLOCK_MONOTONIC, &t0);
    do {
        if (ioctl(fd, IXPCI_READ_REG, &reg) < 0) {
            perror("ioctl READ_REG SR (wait low)");
            return -1;
        }
        clock_gettime(CLOCK_MONOTONIC, &tn);
        if (elapsed_ns(&t0, &tn) > HANDSHAKE_TIMEOUT_NS) {
            fprintf(stderr, "Timeout handshake bajo\n");
            return -1;
        }
    } while (reg.value & 0x04);

    /* PIC high */
    reg.id    = IXPCI_CR;
    reg.value = config | 0x2000;
    if (ioctl(fd, IXPCI_WRITE_REG, &reg) < 0) {
        perror("ioctl WRITE_REG CR (PIC high)");
        return -1;
    }

    /* Esperar handshake alto */
    reg.id = IXPCI_SR;
    clock_gettime(CLOCK_MONOTONIC, &t0);
    do {
        if (ioctl(fd, IXPCI_READ_REG, &reg) < 0) {
            perror("ioctl READ_REG SR (wait high post-cmd)");
            return -1;
        }
        clock_gettime(CLOCK_MONOTONIC, &tn);
        if (elapsed_ns(&t0, &tn) > HANDSHAKE_TIMEOUT_NS) {
            fprintf(stderr, "Timeout handshake alto (post-cmd)\n");
            return -1;
        }
    } while ((reg.value & 0x04) == 0);

    return 0;
}

/* ---------------------------------------------------------------
 * select_channel
 * -------------------------------------------------------------- */
static int select_channel(int fd, int channel, int config_code)
{
    int cmd = 0x8400 + ((config_code & 0x0F) << 6) + (channel & 0x1F);
    if (pic_control(fd, cmd) < 0)
        return -1;
    usleep(10);
    return 0;
}

/* ---------------------------------------------------------------
 * read_adc
 * -------------------------------------------------------------- */
static int read_adc(int fd)
{
    ixpci_reg_t reg, rad;
    struct timespec t0, tn;

    reg.id    = IXPCI_CR;
    reg.value = 0x2000;
    if (ioctl(fd, IXPCI_WRITE_REG, &reg) < 0) {
        perror("ioctl WRITE_REG CR (clear FIFO 1)");
        return -1;
    }
    reg.value = 0xA000;
    if (ioctl(fd, IXPCI_WRITE_REG, &reg) < 0) {
        perror("ioctl WRITE_REG CR (clear FIFO 2)");
        return -1;
    }

    reg.id    = IXPCI_ADST;
    reg.value = 0xFFFF;
    if (ioctl(fd, IXPCI_WRITE_REG, &reg) < 0) {
        perror("ioctl WRITE_REG ADST (SW trigger)");
        return -1;
    }

    reg.id = IXPCI_SR;
    clock_gettime(CLOCK_MONOTONIC, &t0);
    do {
        if (ioctl(fd, IXPCI_READ_REG, &reg) < 0) {
            perror("ioctl READ_REG SR (wait ready)");
            return -1;
        }
        clock_gettime(CLOCK_MONOTONIC, &tn);
        if (elapsed_ns(&t0, &tn) > ADC_READY_TIMEOUT_NS) {
            fprintf(stderr, "Timeout ADC ready\n");
            return -1;
        }
    } while (!(reg.value & 0x20));

    rad.id = 219; // IXPCI_AD alias IXPCI_AI hardcoded 
    if (ioctl(fd, IXPCI_READ_REG, &rad) < 0) {
        perror("ioctl READ_REG AD");
        return -1;
    }

    return (int)(rad.value & 0x0FFF);
}

/* ---------------------------------------------------------------
 * compute_voltage
 * -------------------------------------------------------------- */
static float compute_voltage(int raw, int config_code, float v_max)
{
    if (config_code == AD_CONFIG_BIPOLAR_5V)
        return ((float)raw - 2048.0f) / 2048.0f * v_max;
    else
        return (float)raw * v_max / 4095.0f;
}

/* ---------------------------------------------------------------
 * main
 * -------------------------------------------------------------- */
int main(void)
{
    int fd, raw, i;
    float voltage;

    printf("Test C++ Directo (Copia read_channels.c)\n");
    printf("========================================\n");

    fd = open("/dev/ixpci1", O_RDWR);
    if (fd < 0) {
        perror("open /dev/ixpci1");
        return 1;
    }

    /* Dummy read en AI_15 para estabilizar MUX */
    if (select_channel(fd, AD_CHANNEL_DUMMY, AD_CONFIG_UNIPOLAR_5V) < 0) {
        fprintf(stderr, "Error canal dummy\n");
        close(fd);
        return 1;
    }
    if (read_adc(fd) < 0) {
        fprintf(stderr, "Error lectura dummy\n");
        close(fd);
        return 1;
    }

    /* Lectura de los 9 canales */
    printf("%-5s %-4s %-26s %-6s  %-10s  %s\n",
           "CANAL", "PIN", "SEÑAL", "ADC", "VOLTAJE", "RANGO");
    printf("-----------------------------------------------------------------------\n");

    for (i = 0; i < NUM_CHANNELS; i++) {

        if (select_channel(fd, channels[i].channel, channels[i].config_code) < 0) {
            fprintf(stderr, "Error seleccionando %s\n", channels[i].name);
            close(fd);
            return 1;
        }

        raw = read_adc(fd);
        if (raw < 0) {
            fprintf(stderr, "Error leyendo %s\n", channels[i].name);
            close(fd);
            return 1;
        }

        voltage = compute_voltage(raw, channels[i].config_code, channels[i].v_max);

        printf("AI_%-2d  %-4d  %-26s %4d    %+7.3f V   %.0f~%.0fV\n",
               channels[i].channel,
               channels[i].pin,
               channels[i].name,
               raw,
               voltage,
               channels[i].v_min,
               channels[i].v_max);
    }

    close(fd);
    printf("\nTest completado exitosamente.\n");
    return 0;
}
