#!/usr/bin/env bash
# Advanced Examples for Unified Shell (ushell)
# Demonstrates complex use cases and feature combinations
# Run with: ./ushell < advanced_examples.sh

echo "========================================="
echo "Unified Shell - Advanced Examples"
echo "========================================="
echo

# ============================================
# EXAMPLE 1: Data Processing Pipeline
# ============================================

echo "=== Example 1: Data Processing Pipeline ==="
echo "Creating sample data files..."

mytouch data1.txt data2.txt data3.txt
echo "Dataset A: 100 records" > data1.txt
echo "Dataset B: 250 records" > data2.txt
echo "Dataset C: 175 records" > data3.txt

echo "Processing all data files..."
myls *.txt | myfd data

echo "Combining data..."
mycat data1.txt data2.txt data3.txt > combined.txt
mycat combined.txt

echo "Cleaning up..."
myrm data*.txt combined.txt
echo "Example 1 complete!"
echo

# ============================================
# EXAMPLE 2: Conditional File Management
# ============================================

echo "=== Example 2: Conditional File Management ==="
echo "Creating backup system..."

mymkdir backup
mytouch important.txt
echo "Critical data" > important.txt

if myls important.txt then
    echo File found, creating backup...
    mycp important.txt backup/important_backup.txt
    echo Backup created successfully
fi

if myls backup/important_backup.txt then
    echo Verifying backup...
    mycat backup/important_backup.txt
    echo Backup verified!
fi

echo "Cleaning up..."
myrm important.txt backup/important_backup.txt
myrmdir backup
echo "Example 2 complete!"
echo

# ============================================
# EXAMPLE 3: Complex Arithmetic Calculations
# ============================================

echo "=== Example 3: Complex Arithmetic Calculations ==="
echo "Financial calculations..."

set PRICE=1500
set QUANTITY=12
set TAX_RATE=8

set SUBTOTAL=$((PRICE * QUANTITY))
set TAX=$((SUBTOTAL * TAX_RATE / 100))
set TOTAL=$((SUBTOTAL + TAX))

echo "Price per unit: \$$PRICE"
echo "Quantity: $QUANTITY"
echo "Tax rate: $TAX_RATE%"
echo "Subtotal: \$$SUBTOTAL"
echo "Tax: \$$TAX"
echo "Total: \$$TOTAL"
echo

echo "Geometric calculations..."
set RADIUS=10
set PI=314
set AREA=$((PI * RADIUS * RADIUS / 100))
set CIRCUMFERENCE=$((2 * PI * RADIUS / 100))

echo "Circle with radius $RADIUS units:"
echo "Area: $AREA square units"
echo "Circumference: $CIRCUMFERENCE units"
echo "Example 3 complete!"
echo

# ============================================
# EXAMPLE 4: Multi-Level Directory Structure
# ============================================

echo "=== Example 4: Multi-Level Directory Structure ==="
echo "Creating project structure..."

mymkdir project
cd project
mymkdir src include tests docs

echo "Creating source files..."
mytouch src/main.c src/utils.c
mytouch include/header.h
mytouch tests/test_main.c
mytouch docs/README.txt

echo "Project structure:"
myls
echo
echo "Source files:"
myls src
echo
echo "Include files:"
myls include

echo "Navigating structure..."
cd src
pwd
cd ../tests
pwd
cd ../..

echo "Cleaning up..."
myrm project/src/main.c project/src/utils.c
myrm project/include/header.h
myrm project/tests/test_main.c
myrm project/docs/README.txt
myrmdir project/src project/include project/tests project/docs
myrmdir project
echo "Example 4 complete!"
echo

# ============================================
# EXAMPLE 5: Variable Interpolation & Expansion
# ============================================

echo "=== Example 5: Variable Interpolation & Expansion ==="
echo "Building file paths with variables..."

set PROJECT_NAME=MyApp
set VERSION=1.0
set BUILD_NUM=42

set RELEASE_NAME=${PROJECT_NAME}_v${VERSION}_build$BUILD_NUM
echo "Release name: $RELEASE_NAME"

set BASE_DIR=/tmp/build
set SRC_DIR=${BASE_DIR}/src
set BIN_DIR=${BASE_DIR}/bin

echo "Build directories:"
echo "Base: $BASE_DIR"
echo "Source: $SRC_DIR"
echo "Binary: $BIN_DIR"

echo "Creating release package..."
mytouch ${RELEASE_NAME}.tar.gz
myls ${RELEASE_NAME}.tar.gz
myrm ${RELEASE_NAME}.tar.gz
echo "Example 5 complete!"
echo

# ============================================
# EXAMPLE 6: Pattern Matching & Filtering
# ============================================

echo "=== Example 6: Pattern Matching & Filtering ==="
echo "Creating various file types..."

mytouch app.c app.h app.o
mytouch utils.c utils.h utils.o
mytouch main.c main.o
mytouch test.c test.o

echo "All files:"
myls

echo "Source files only (*.c):"
myls *.c

echo "Header files only (*.h):"
myls *.h

echo "Object files only (*.o):"
myls *.o

echo "Files starting with 'app':"
myls app*

echo "Files starting with 'a' or 'u':"
myls [au]*

echo "Cleaning up all object files..."
myrm *.o

echo "Remaining files:"
myls *.c *.h

echo "Final cleanup..."
myrm *.c *.h
echo "Example 6 complete!"
echo

# ============================================
# EXAMPLE 7: Nested Conditionals & Logic
# ============================================

echo "=== Example 7: Nested Conditionals & Logic ==="
echo "File validation system..."

mytouch config.txt
echo "config_value=123" > config.txt

if myls config.txt then
    echo Configuration file found
    if mycat config.txt then
        echo Configuration file readable
        echo Contents verified
    fi
fi

echo "Creating log file..."
mytouch app.log
echo "Application started" > app.log

if myls app.log then
    echo Log file exists
    mycat app.log
fi

myrm config.txt app.log
echo "Example 7 complete!"
echo

# ============================================
# EXAMPLE 8: Batch File Operations
# ============================================

echo "=== Example 8: Batch File Operations ==="
echo "Processing multiple files..."

echo "Creating test dataset..."
mytouch test_001.dat test_002.dat test_003.dat
mytouch test_004.dat test_005.dat
echo "Data chunk 1" > test_001.dat
echo "Data chunk 2" > test_002.dat
echo "Data chunk 3" > test_003.dat
echo "Data chunk 4" > test_004.dat
echo "Data chunk 5" > test_005.dat

echo "All data files:"
myls test_*.dat

echo "Processing files with pattern matching..."
myls test_00[1-3].dat

echo "Creating backup copies..."
mycp test_001.dat backup_001.dat
mycp test_002.dat backup_002.dat
mycp test_003.dat backup_003.dat

echo "Verifying backups..."
myls backup_*.dat

echo "Cleaning up..."
myrm test_*.dat backup_*.dat
echo "Example 8 complete!"
echo

# ============================================
# EXAMPLE 9: Report Generation
# ============================================

echo "=== Example 9: Report Generation ==="
echo "Building system report..."

mytouch report.txt
echo "System Report" > report.txt
echo "=============" >> report.txt
echo >> report.txt

set REPORT_DATE=2025-11-19
echo "Date: $REPORT_DATE" >> report.txt
echo "User: $USER" >> report.txt
echo "Home: $HOME" >> report.txt
echo >> report.txt

echo "Statistics:" >> report.txt
set FILES_PROCESSED=156
set ERRORS_FOUND=3
set SUCCESS_RATE=$((((FILES_PROCESSED - ERRORS_FOUND) * 100) / FILES_PROCESSED))
echo "Files processed: $FILES_PROCESSED" >> report.txt
echo "Errors found: $ERRORS_FOUND" >> report.txt
echo "Success rate: $SUCCESS_RATE%" >> report.txt
echo >> report.txt

echo "Report generated!" >> report.txt

echo "Final report:"
mycat report.txt

myrm report.txt
echo "Example 9 complete!"
echo

# ============================================
# EXAMPLE 10: Complex Pipeline Workflow
# ============================================

echo "=== Example 10: Complex Pipeline Workflow ==="
echo "Setting up data processing pipeline..."

mytouch raw_data.txt processed_data.txt
echo "raw_001.dat" > raw_data.txt
echo "raw_002.dat" >> raw_data.txt
echo "raw_003.dat" >> raw_data.txt

echo "Pipeline stage 1: List and filter"
mycat raw_data.txt | myfd raw

echo "Pipeline stage 2: Create processed data"
echo "Processed output" > processed_data.txt
mycat processed_data.txt

echo "Cleaning up pipeline..."
myrm raw_data.txt processed_data.txt
echo "Example 10 complete!"
echo

# ============================================
# EXAMPLE 11: Environment Configuration
# ============================================

echo "=== Example 11: Environment Configuration ==="
echo "Setting up environment..."

export APP_NAME=AdvancedShell
export APP_VERSION=1.0.0
export APP_HOME=/usr/local/app
export DEBUG_MODE=enabled

echo "Environment variables:"
echo "APP_NAME=$APP_NAME"
echo "APP_VERSION=$APP_VERSION"
echo "APP_HOME=$APP_HOME"
echo "DEBUG_MODE=$DEBUG_MODE"

set CONFIG_FILE=${APP_HOME}/config.ini
echo "Config file path: $CONFIG_FILE"

echo "Cleaning up environment..."
unset APP_NAME
unset APP_VERSION
unset APP_HOME
unset DEBUG_MODE
echo "Example 11 complete!"
echo

# ============================================
# EXAMPLE 12: State Machine Simulation
# ============================================

echo "=== Example 12: State Machine Simulation ==="
echo "Simulating application states..."

set STATE=INIT
echo "Current state: $STATE"

set STATE=LOADING
echo "State transition: $STATE"

set STATE=PROCESSING
echo "State transition: $STATE"
set PROGRESS=0

set PROGRESS=$((PROGRESS + 25))
echo "Progress: $PROGRESS%"

set PROGRESS=$((PROGRESS + 25))
echo "Progress: $PROGRESS%"

set PROGRESS=$((PROGRESS + 25))
echo "Progress: $PROGRESS%"

set PROGRESS=$((PROGRESS + 25))
echo "Progress: $PROGRESS%"

set STATE=COMPLETE
echo "Final state: $STATE"
echo "Example 12 complete!"
echo

# ============================================
# COMPLETION
# ============================================

echo "========================================="
echo "All Advanced Examples Completed!"
echo "========================================="
echo
echo "You've explored:"
echo "  ✓ Data processing pipelines"
echo "  ✓ Conditional file management"
echo "  ✓ Complex arithmetic"
echo "  ✓ Directory structure management"
echo "  ✓ Variable interpolation"
echo "  ✓ Pattern matching & filtering"
echo "  ✓ Nested conditionals"
echo "  ✓ Batch file operations"
echo "  ✓ Report generation"
echo "  ✓ Pipeline workflows"
echo "  ✓ Environment configuration"
echo "  ✓ State machine simulation"
echo
echo "For more information:"
echo "  - User Guide: docs/USER_GUIDE.md"
echo "  - Developer Guide: docs/DEVELOPER_GUIDE.md"
echo "  - Basic Examples: examples/example_scripts.sh"
echo "  - Tutorial: examples/tutorial.txt"
echo
echo "Happy shell scripting!"
