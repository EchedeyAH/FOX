#include <iostream>
#include <vector>
#include <string>
#include <cstring>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <iomanip>
#include <thread>
#include <chrono>

extern "C" {
    #include "adquisicion_datos/ixpci.h"
}

// IOCTLs definitions if missing (copied from driver logic)
#ifndef IXPCI_GET_INFO
#define IXPCI_GET_INFO _IOR(IXPCI_MAGIC_NUM, IXPCI_IOCTL_ID_GET_INFO, ixpci_card_info *)
#endif

// Correct definitions for DI/DO based on ixpci driver analysis
// The driver expects 'unsigned int' or 'uint32_t' for DI/DO value, not void*
// We use 'unsigned int' to match standard ioctl patterns for 32-bit values
#define PEX_DI_READ  _IOR(IXPCI_MAGIC_NUM, IXPCI_IOCTL_ID_DI, unsigned int)
#define PEX_DO_WRITE _IOW(IXPCI_MAGIC_NUM, IXPCI_IOCTL_ID_DO, unsigned int)

struct CardInfo {
    int id;
    std::string device_path;
    ixpci_card_info info;
    bool valid;
};

void print_card_info(const CardInfo& card) {
    std::cout << "--------------------------------------------------\n";
    std::cout << "Card ID: " << card.id << " (" << card.device_path << ")\n";
    if (card.valid) {
        std::cout << "  Vendor ID: 0x" << std::hex << card.info.vendor_id << "\n";
        std::cout << "  Device ID: 0x" << card.info.device_id << "\n";
        std::cout << "  SubVendor: 0x" << card.info.subvendor_id << "\n";
        std::cout << "  SubDevice: 0x" << card.info.subdevice_id << std::dec << "\n";
        std::cout << "  Interrupt: " << (int)card.info.interrupt_line << "\n";
        std::cout << "  Base Addr: 0x" << std::hex << card.info.io_base_address << std::dec << "\n";
        
        // Try to identify simplified name
        if (card.info.device_id == 0x1234) std::cout << "  Model: Generic PEX/PISO\n";
        // Check for specific PEX-16 signature if known (e.g. subdevice 0x1616)
        if (card.info.subdevice_id == 0x1616) std::cout << "  Model: PEX-16/PISO-16 Likely\n";
    } else {
        std::cout << "  Info: Invalid or Not Accessible\n";
    }
}

int main(int argc, char* argv[]) {
    std::cout << "=== PEX Digital I/O Test Tool ===\n";
    
    std::vector<CardInfo> cards;
    
    // Scan devices /dev/ixpci0 to /dev/ixpci3
    for (int i = 0; i < 4; i++) {
        std::string path = "/dev/ixpci" + std::to_string(i);
        int fd = open(path.c_str(), O_RDWR);
        
        CardInfo card;
        card.id = i;
        card.device_path = path;
        card.valid = false;
        
        if (fd >= 0) {
            memset(&card.info, 0, sizeof(card.info));
            if (ioctl(fd, IXPCI_GET_INFO, &card.info) >= 0) {
                card.valid = true;
            }
            close(fd);
        }
        
        if (card.valid) {
            cards.push_back(card);
            print_card_info(card);
        }
    }
    
    if (cards.empty()) {
        std::cerr << "No PEX cards found! Check drivers and connections.\n";
        return 1;
    }
    
    // Select card
    int selected_idx = -1;
    if (argc > 1) {
        selected_idx = std::stoi(argv[1]);
    } else {
        // Default to first card that is NOT the 1202L (usually 1202L is index 1)
        for (const auto& c : cards) {
            if (c.id != 1) { // Assume 1 is Analog based on previous tests
                selected_idx = c.id;
                break;
            }
        }
        if (selected_idx == -1) selected_idx = cards[0].id;
    }
    
    std::cout << "\nTesting Card: /dev/ixpci" << selected_idx << "\n";
    std::cout << "Press Ctrl+C to exit.\n\n";
    
    int fd = open(("/dev/ixpci" + std::to_string(selected_idx)).c_str(), O_RDWR);
    if (fd < 0) {
        perror("Failed to open selected card");
        return 1;
    }
    
    std::cout << "Monitoring Digital Inputs (Port 0)...\n";
    std::cout << "Values displayed in HEX (LSB = Input 0)\n";
    
    unsigned int last_val = 0xFFFFFFFF;
    
    while (true) {
        unsigned int val = 0;
        int ret = ioctl(fd, PEX_DI_READ, &val);
        
        if (ret < 0) {
            perror("ioctl PEX_DI_READ failed");
            std::this_thread::sleep_for(std::chrono::seconds(1));
            continue;
        }
        
        if (val != last_val) {
            std::cout << "DI Change: 0x" << std::hex << std::setw(4) << std::setfill('0') << val 
                      << " (Bin: ";
            for(int b=15; b>=0; b--) std::cout << ((val >> b) & 1);
            std::cout << ")" << std::dec << "\n";
            last_val = val;
        }
        
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
    
    close(fd);
    return 0;
}
