/***************************************/
// Proyecto: FOX
// Nombre fichero: main.c
// Descripción: Función principal y el flujo de control del programa.
// Autor: Echedey Aguilar Hernández
// email: eaguilar@us.es
// Fecha: 2025-01-08
// ***************************************/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include "can_manager.h"
#include "imu_manager.h"
#include "rt_scheduler.h"
#include "ecu_config.h"
#include "logger.h"

volatile sig_atomic_t running = 1;

void signal_handler(int signum) {
    running = 0;
    log_info("Received signal %d. Shutting down...", signum);
}

void cleanup() {
    cleanup_can_module();
    cleanup_imu_module();
    cleanup_rt_scheduler();
    log_info("Cleanup completed. Exiting.");
}

int initialize_modules() {
    if (init_can_module() != 0) {
        log_error("Failed to initialize CAN module");
        return -1;
    }
    
    if (init_imu_module() != 0) {
        log_error("Failed to initialize IMU module");
        return -1;
    }
    
    if (init_rt_scheduler() != 0) {
        log_error("Failed to initialize RT scheduler");
        return -1;
    }
    
    return 0;
}

int main() {
    log_info("Starting FOX control system");

    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    if (load_configuration("config.json") != 0) {
        log_error("Failed to load configuration");
        return EXIT_FAILURE;
    }

    if (initialize_modules() != 0) {
        cleanup();
        return EXIT_FAILURE;
    }
    
    log_info("All modules initialized. Starting main loop");

    while (running) {
        run_rt_tasks();
        usleep(10000); // 10ms sleep to prevent CPU hogging
    }
    
    cleanup();
    return EXIT_SUCCESS;
}
