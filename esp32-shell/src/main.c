/**
 * @file main.c
 * @brief ESP32 Shell - Main Entry Point
 * 
 * This is the main entry point for the ESP32 shell port.
 * It initializes the platform-specific components (UART console)
 * and launches the shell main loop.
 * 
 * The shell runs as the main FreeRTOS task and provides an interactive
 * command-line interface over the serial console.
 * 
 * Note: SPIFFS initialization is now handled by the VFS layer in esp_shell_init().
 */

#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_log.h"

#include "platform.h"
#include "esp_shell.h"

/* Tag for ESP-IDF logging system */
static const char *TAG = "esp32_shell";

/**
 * @brief Print startup banner
 * 
 * Displays the shell welcome message and system information
 */
static void print_banner(void) {
    printf("\n");
    printf("=====================================\n");
    printf("   ESP32 Shell (ushell port)\n");
    printf("=====================================\n");
    printf("Type 'help' for available commands\n");
    printf("Type 'info' for system information\n");
    printf("\n");
}

/**
 * @brief Main application entry point
 * 
 * This function is called by the ESP-IDF framework after system
 * initialization is complete. It sets up the console and enters
 * the shell main loop.
 * 
 * Note: SPIFFS is initialized by esp_shell_init() via the VFS layer.
 */
void app_main(void) {
    ESP_LOGI(TAG, "Starting ESP32 Shell...");
    
    /* Initialize platform abstraction layer (UART setup) */
    platform_init();
    
    /* Print welcome banner */
    print_banner();
    
    /* Initialize and run the shell
     * This also initializes the VFS/SPIFFS layer */
    esp_shell_init();
    
    /* Enter the shell main loop
     * This function runs forever, processing user commands */
    esp_shell_run();
    
    /* Should never reach here */
    ESP_LOGI(TAG, "Shell exited");
}
