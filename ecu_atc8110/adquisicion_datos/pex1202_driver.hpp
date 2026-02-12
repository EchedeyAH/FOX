#ifndef PEX1202_DRIVER_HPP
#define PEX1202_DRIVER_HPP

#include <sys/ioctl.h>
// Configuración de Gain (PEX-1202 Standard)
// Bipolar +/- 5V suele ser el código 0 o 1 dependiendo del modelo H/L.
// Asumiremos Gain 0 = +/- 5V, Gain 1 = +/- 10V para el modelo L (Low Gain).
#define PEX_GAIN_BIP_5V   0x00
#define PEX_GAIN_BIP_10V  0x01
#define PEX_GAIN_UNI_5V   0x02 // Si soportado
#define PEX_GAIN_UNI_10V  0x03 // Si soportado

// Modo Single-Ended vs Differential
// El modo se configura a menudo en el mismo registro o via jumpers en HW antiguos.
// Si es por SW en ADGCR, suele ser bit 4 o similar, pero no está documentado.
// Sin embargo, el informe dice "se configura en Modo single-ended".
// Asumiremos que el default o la configuración de canal maneja la multiplexación.
// Por ahora solo impondremos el GANANCIA en ADGCR.

// ========================================================================
// MagicScan Protocol - Registros adicionales
// ========================================================================
#define IXPCI_CR  200  // Control Register para PIC handshake
#define IXPCI_SR  201  // Status Register para PIC handshake

// Configuración de canales ADC (PEX-1202L manual pag. 60)
#define AD_CONFIG_UNIPOLAR_5V    0x09   /* 0 ~ +5 V  */
#define AD_CONFIG_BIPOLAR_5V     0x00   /* -5 ~ +5 V */
#define AD_CHANNEL_DUMMY         15     /* AI_15, pin 16, tied to A.GND */

// Timeouts (nanoseconds)
#define HANDSHAKE_TIMEOUT_NS     100000000L   /* 100 ms */
#define ADC_READY_TIMEOUT_NS     100000000L   /* 100 ms */

// ========================================================================
// Funciones de bajo nivel - Código probado portado desde C
// ========================================================================

#include <ctime>
#include <cstring>
#include <unistd.h>

namespace pex1202 {

// Calcular tiempo transcurrido en nanosegundos
inline long elapsed_ns(const struct timespec *start, const struct timespec *end)
{
    return (end->tv_sec  - start->tv_sec)  * 1000000000L
         + (end->tv_nsec - start->tv_nsec);
}

// Handshake con el controlador MagicScan (PIC)
inline int pic_control(int fd, int cmd)
{
    ixpci_reg_t reg;
    memset(&reg, 0, sizeof(reg));
    struct timespec t0, tn;

    // Recovery si handshake esta bajo
    reg.id = IXPCI_SR;
    if (ioctl(fd, IXPCI_READ_REG, &reg) < 0) {
        return -1;
    }
    if ((reg.value & 0x04) == 0) {
        reg.id    = IXPCI_CR;
        reg.value = 0xFFFF;
        if (ioctl(fd, IXPCI_WRITE_REG, &reg) < 0) {
            return -1;
        }
    }

    // Esperar handshake alto
    reg.id = IXPCI_SR;
    clock_gettime(CLOCK_MONOTONIC, &t0);
    do {
        if (ioctl(fd, IXPCI_READ_REG, &reg) < 0) {
            return -1;
        }
        clock_gettime(CLOCK_MONOTONIC, &tn);
        if (elapsed_ns(&t0, &tn) > HANDSHAKE_TIMEOUT_NS) {
            return -1;
        }
    } while ((reg.value & 0x04) == 0);

    // PIC low
    int config = cmd & 0xDFFF;
    reg.id    = IXPCI_CR;
    reg.value = config;
    if (ioctl(fd, IXPCI_WRITE_REG, &reg) < 0) {
        return -1;
    }

    // Esperar handshake bajo
    reg.id = IXPCI_SR;
    clock_gettime(CLOCK_MONOTONIC, &t0);
    do {
        if (ioctl(fd, IXPCI_READ_REG, &reg) < 0) {
            return -1;
        }
        clock_gettime(CLOCK_MONOTONIC, &tn);
        if (elapsed_ns(&t0, &tn) > HANDSHAKE_TIMEOUT_NS) {
            return -1;
        }
    } while (reg.value & 0x04);

    // PIC high
    reg.id    = IXPCI_CR;
    reg.value = config | 0x2000;
    if (ioctl(fd, IXPCI_WRITE_REG, &reg) < 0) {
        return -1;
    }

    // Esperar handshake alto
    reg.id = IXPCI_SR;
    clock_gettime(CLOCK_MONOTONIC, &t0);
    do {
        if (ioctl(fd, IXPCI_READ_REG, &reg) < 0) {
            return -1;
        }
        clock_gettime(CLOCK_MONOTONIC, &tn);
        if (elapsed_ns(&t0, &tn) > HANDSHAKE_TIMEOUT_NS) {
            return -1;
        }
    } while ((reg.value & 0x04) == 0);

    return 0;
}

// Seleccionar canal ADC con configuración
inline int select_channel(int fd, int channel, int config_code)
{
    int cmd = 0x8400 + ((config_code & 0x0F) << 6) + (channel & 0x1F);
    if (pic_control(fd, cmd) < 0)
        return -1;
    usleep(10);
    return 0;
}

// Leer valor ADC (retorna raw 0-4095, o -1 en error)
inline int read_adc(int fd)
{
    ixpci_reg_t reg, rad;
    memset(&reg, 0, sizeof(reg));
    memset(&rad, 0, sizeof(rad));
    struct timespec t0, tn;

    reg.id    = IXPCI_CR;
    reg.value = 0x2000;
    if (ioctl(fd, IXPCI_WRITE_REG, &reg) < 0) {
        return -1;
    }
    reg.value = 0xA000;
    if (ioctl(fd, IXPCI_WRITE_REG, &reg) < 0) {
        return -1;
    }

    reg.id    = IXPCI_ADST;
    reg.value = 0xFFFF;
    if (ioctl(fd, IXPCI_WRITE_REG, &reg) < 0) {
        return -1;
    }

    reg.id = IXPCI_SR;
    clock_gettime(CLOCK_MONOTONIC, &t0);
    do {
        if (ioctl(fd, IXPCI_READ_REG, &reg) < 0) {
            return -1;
        }
        clock_gettime(CLOCK_MONOTONIC, &tn);
        if (elapsed_ns(&t0, &tn) > ADC_READY_TIMEOUT_NS) {
            return -1;
        }
    } while (!(reg.value & 0x20));

    rad.id = IXPCI_AD;
    if (ioctl(fd, IXPCI_READ_REG, &rad) < 0) {
        return -1;
    }

    return static_cast<int>(rad.value & 0x0FFF);
}

// Convertir valor ADC raw a voltaje
inline float compute_voltage(int raw, int config_code, float v_max)
{
    if (config_code == AD_CONFIG_BIPOLAR_5V) {
        return (static_cast<float>(raw) - 2048.0f) / 2048.0f * v_max;
    } else {
        return static_cast<float>(raw) * v_max / 4095.0f;
    }
}

} // namespace pex1202
