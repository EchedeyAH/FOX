#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <iostream>
#include <cstring>
#include <cerrno>

int main() {
    std::cout << "Testing connection to /dev/ixpio0..." << std::endl;
    // Attempt open with O_RDWR only
    int fd = open("/dev/ixpio0", O_RDWR);
    
    if (fd < 0) {
        std::cerr << "Failed to open /dev/ixpio0: " << strerror(errno) 
                  << " (errno=" << errno << ")" << std::endl;
        
        if (errno == ENODEV) {
             std::cerr << "ENODEV: Device minor number might be incorrect or driver not loaded." << std::endl;
        }
        return 1;
    }
    
    std::cout << "Successfully opened /dev/ixpio0 (FD: " << fd << ")" << std::endl;
    close(fd);
    return 0;
}
