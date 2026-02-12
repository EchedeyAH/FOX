#include <iostream>
#include <iomanip>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include "adquisicion_datos/pex1202_driver.hpp"

int main()
{
    std::cout << "Test Simple PEX-1202L (sin handshake)" << std::endl;
    std::cout << "======================================" << std::endl;
    
    // Abrir dispositivo
    int fd = open("/dev/ixpci1", O_RDWR);
    if (fd < 0) {
        std::cerr << "ERROR: No se puede abrir /dev/ixpci1" << std::endl;
        std::cerr << "Ejecuta con: sudo ./test_simple_adc" << std::endl;
        return 1;
    }
    
    std::cout << "✓ /dev/ixpci1 abierto (fd=" << fd << ")" << std::endl;
    std::cout << std::endl;
    
    // Canales a leer (según código C funcional)
    int canales[] = {1, 2, 3, 4, 5, 6, 8, 10, 12};
    const char* nombres[] = {
        "AI_1  ACELERADOR",
        "AI_2  CORRIENTE_EJE_D",
        "AI_3  FRENO",
        "AI_4  VOLANTE",
        "AI_5  CORRIENTE_EJE_T",
        "AI_6  SUSPENSION_RL",
        "AI_8  SUSPENSION_RR",
        "AI_10 SUSPENSION_FL",
        "AI_12 SUSPENSION_FR"
    };
    
    std::cout << std::left
              << std::setw(25) << "CANAL"
              << std::setw(8) << "RAW"
              << std::setw(12) << "VOLTAJE"
              << std::endl;
    std::cout << std::string(45, '-') << std::endl;
    
    for (int i = 0; i < 9; i++) {
        int canal = canales[i];
        ixpci_reg reg{};
        
        // Método 1: Simple (sin handshake)
        reg.id = IXPCI_AICR;
        reg.value = canal;
        reg.mode = IXPCI_RM_NORMAL;
        if (ioctl(fd, IXPCI_WRITE_REG, &reg) < 0) {
            std::cerr << "Error WRITE_REG AICR" << std::endl;
            continue;
        }
        
        reg.id = IXPCI_ADST;
        reg.value = 0;
        if (ioctl(fd, IXPCI_WRITE_REG, &reg) < 0) {
            std::cerr << "Error WRITE_REG ADST" << std::endl;
            continue;
        }
        
        reg.id = IXPCI_AI;
        reg.mode = IXPCI_RM_READY;
        if (ioctl(fd, IXPCI_READ_REG, &reg) < 0) {
            std::cerr << "Error READ_REG AI" << std::endl;
            continue;
        }
        
        uint16_t raw = reg.value & 0x0FFF;
        double volts = raw * 5.0 / 4095.0;  // Unipolar 0-5V
        
        std::cout << std::left
                  << std::setw(25) << nombres[i]
                  << std::setw(8) << raw
                  << std::fixed << std::setprecision(3)
                  << std::setw(12) << volts << " V"
                  << std::endl;
    }
    
    std::cout << std::endl;
    std::cout << "Test completado" << std::endl;
    
    close(fd);
    return 0;
}
