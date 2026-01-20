#!/bin/bash
# ============================================================================
# ESP32 Shell - Flash and Monitor Script
# ============================================================================
# Convenience script for building, flashing, and monitoring the ESP32 shell.
# Uses PlatformIO with ESP-IDF framework.
#
# Usage:
#   ./flash.sh              # Build, flash, and open monitor
#   ./flash.sh build        # Build only
#   ./flash.sh upload       # Build and upload
#   ./flash.sh monitor      # Open serial monitor only
#   ./flash.sh clean        # Clean build artifacts
#   ./flash.sh uploadfs     # Upload SPIFFS filesystem image
# ============================================================================

# Configuration
PIO_CMD="${HOME}/.platformio/penv/bin/pio"
BAUD_RATE=115200
ENV_NAME="esp32s3dev"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Check if PlatformIO is available
check_pio() {
    if [ ! -f "$PIO_CMD" ]; then
        echo -e "${RED}Error: PlatformIO not found at $PIO_CMD${NC}"
        echo "Install with: pip install platformio"
        exit 1
    fi
}

# Print status message
status() {
    echo -e "${GREEN}==>${NC} $1"
}

# Print warning message
warning() {
    echo -e "${YELLOW}Warning:${NC} $1"
}

# Build the project
do_build() {
    status "Building ESP32 Shell..."
    $PIO_CMD run -e $ENV_NAME
}

# Upload firmware to device
do_upload() {
    status "Uploading firmware to ESP32..."
    $PIO_CMD run -e $ENV_NAME -t upload
}

# Upload SPIFFS filesystem
do_uploadfs() {
    status "Uploading SPIFFS filesystem..."
    $PIO_CMD run -e $ENV_NAME -t uploadfs
}

# Open serial monitor
do_monitor() {
    status "Opening serial monitor at ${BAUD_RATE} baud..."
    echo "Press Ctrl+] to exit monitor"
    $PIO_CMD device monitor -b $BAUD_RATE
}

# Clean build artifacts
do_clean() {
    status "Cleaning build artifacts..."
    $PIO_CMD run -e $ENV_NAME -t clean
    rm -rf .pio/build
    rm -f sdkconfig.esp32s3dev
    status "Clean complete"
}

# Show help
show_help() {
    echo "ESP32 Shell Flash Script"
    echo ""
    echo "Usage: $0 [command]"
    echo ""
    echo "Commands:"
    echo "  (none)     Build, flash, and open monitor (default)"
    echo "  build      Build only"
    echo "  upload     Build and upload to device"
    echo "  monitor    Open serial monitor only"
    echo "  uploadfs   Upload SPIFFS filesystem from 'data' folder"
    echo "  clean      Clean build artifacts"
    echo "  help       Show this help message"
    echo ""
    echo "Examples:"
    echo "  $0                 # Full flash cycle"
    echo "  $0 build           # Just compile"
    echo "  $0 monitor         # Just open monitor"
}

# Main
check_pio

case "${1:-flash}" in
    build)
        do_build
        ;;
    upload)
        do_upload
        ;;
    uploadfs)
        do_uploadfs
        ;;
    monitor)
        do_monitor
        ;;
    clean)
        do_clean
        ;;
    flash)
        do_upload
        do_monitor
        ;;
    help|--help|-h)
        show_help
        ;;
    *)
        echo -e "${RED}Unknown command: $1${NC}"
        show_help
        exit 1
        ;;
esac
