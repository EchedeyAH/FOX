#ifndef PEX1202_DRIVER_HPP
#define PEX1202_DRIVER_HPP

#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>
#include <cstring>
#include <cstdio> // perror

// Include system driver header (C style)
extern "C" {
    #include "ixpci.h"
}

// ========================================================================
// PEX-1202L Specific Configurations (not in ixpci.h)
// ========================================================================
#define AD_CONFIG_UNIPOLAR_5V    0x09   /* 0 ~ +5 V  */
#define AD_CONFIG_BIPOLAR_5V     0x00   /* -5 ~ +5 V */
#define AD_CHANNEL_DUMMY         15     /* AI_15 */

/* Timeouts */
#define HANDSHAKE_TIMEOUT_NS     100000000L   /* 100 ms */
#define ADC_READY_TIMEOUT_NS     100000000L   /* 100 ms */

namespace pex1202 {

// Helper: elapsed time in ns
inline long elapsed_ns(const struct timespec *start, const struct timespec *end)
{
    return (end->tv_sec  - start->tv_sec)  * 1000000000L
         + (end->tv_nsec - start->tv_nsec);
}

// Handshake with MagicScan controller (PIC)
inline int pic_control(int fd, int cmd)
{
    ixpci_reg_t reg;
    memset(&reg, 0, sizeof(reg));
    struct timespec t0, tn;

    // Recovery if handshake is low
    reg.id = IXPCI_SR;
    if (ioctl(fd, IXPCI_READ_REG, &reg) < 0) return -1;
    
    if ((reg.value & 0x04) == 0) {
        reg.id    = IXPCI_CR;
        reg.value = 0xFFFF;
        if (ioctl(fd, IXPCI_WRITE_REG, &reg) < 0) return -1;
    }

    // Wait for handshake high
    reg.id = IXPCI_SR;
    clock_gettime(CLOCK_MONOTONIC, &t0);
    do {
        if (ioctl(fd, IXPCI_READ_REG, &reg) < 0) return -1;
        clock_gettime(CLOCK_MONOTONIC, &tn);
        if (elapsed_ns(&t0, &tn) > HANDSHAKE_TIMEOUT_NS) return -1;
    } while ((reg.value & 0x04) == 0);

    // PIC low
    int config = cmd & 0xDFFF;
    reg.id    = IXPCI_CR;
    reg.value = config;
    if (ioctl(fd, IXPCI_WRITE_REG, &reg) < 0) return -1;

    // Wait for handshake low
    reg.id = IXPCI_SR;
    clock_gettime(CLOCK_MONOTONIC, &t0);
    do {
        if (ioctl(fd, IXPCI_READ_REG, &reg) < 0) return -1;
        clock_gettime(CLOCK_MONOTONIC, &tn);
        if (elapsed_ns(&t0, &tn) > HANDSHAKE_TIMEOUT_NS) return -1;
    } while (reg.value & 0x04);

    // PIC high
    reg.id    = IXPCI_CR;
    reg.value = config | 0x2000;
    if (ioctl(fd, IXPCI_WRITE_REG, &reg) < 0) return -1;

    // Wait for handshake high
    reg.id = IXPCI_SR;
    clock_gettime(CLOCK_MONOTONIC, &t0);
    do {
        if (ioctl(fd, IXPCI_READ_REG, &reg) < 0) return -1;
        clock_gettime(CLOCK_MONOTONIC, &tn);
        if (elapsed_ns(&t0, &tn) > HANDSHAKE_TIMEOUT_NS) return -1;
    } while ((reg.value & 0x04) == 0);

    return 0;
}

// Select ADC channel with configuration
inline int select_channel(int fd, int channel, int config_code)
{
    int cmd = 0x8400 + ((config_code & 0x0F) << 6) + (channel & 0x1F);
    if (pic_control(fd, cmd) < 0)
        return -1;
    usleep(10);
    return 0;
}

// Read ADC value (returns raw 0-4095, or -1 on error)
inline int read_adc(int fd)
{
    ixpci_reg_t reg, rad;
    memset(&reg, 0, sizeof(reg));
    memset(&rad, 0, sizeof(rad));
    struct timespec t0, tn;

    reg.id    = IXPCI_CR;
    reg.value = 0x2000;
    if (ioctl(fd, IXPCI_WRITE_REG, &reg) < 0) return -1;

    reg.value = 0xA000;
    if (ioctl(fd, IXPCI_WRITE_REG, &reg) < 0) return -1;

    reg.id    = IXPCI_ADST;
    reg.value = 0xFFFF;
    if (ioctl(fd, IXPCI_WRITE_REG, &reg) < 0) return -1;

    reg.id = IXPCI_SR;
    clock_gettime(CLOCK_MONOTONIC, &t0);
    do {
        if (ioctl(fd, IXPCI_READ_REG, &reg) < 0) return -1;
        clock_gettime(CLOCK_MONOTONIC, &tn);
        if (elapsed_ns(&t0, &tn) > ADC_READY_TIMEOUT_NS) return -1;
    } while (!(reg.value & 0x20));

    rad.id = IXPCI_AD;
    if (ioctl(fd, IXPCI_READ_REG, &rad) < 0) return -1;

    return static_cast<int>(rad.value & 0x0FFF);
}

// Convert raw ADC to voltage
inline float compute_voltage(int raw, int config_code, float v_max)
{
    if (config_code == AD_CONFIG_BIPOLAR_5V) {
        return (static_cast<float>(raw) - 2048.0f) / 2048.0f * v_max;
    } else {
        return static_cast<float>(raw) * v_max / 4095.0f;
    }
}

} // namespace pex1202

#endif // PEX1202_DRIVER_HPP
