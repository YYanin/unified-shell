# ESP32 Shell - Source Component CMakeLists.txt
# ============================================================================
# This CMakeLists.txt tells ESP-IDF how to build the main component.
# All source files in src/ are compiled as part of this component.
# ============================================================================

# Register this as an ESP-IDF component
idf_component_register(
    SRCS 
        "main.c"
        "esp_shell.c"
        "platform_esp32.c"
        "vfs_esp32.c"
        "executor_esp32.c"
        "terminal_esp32.c"
    INCLUDE_DIRS 
        "."
    REQUIRES 
        driver          # For UART driver
        esp_timer       # For timing functions
        spiffs          # For SPIFFS filesystem
        vfs             # For VFS abstraction
)
