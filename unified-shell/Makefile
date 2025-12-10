# Unified Shell - Makefile

# Compiler and flags
CC = gcc
CFLAGS = -Wall -Wextra -g -Iinclude
LDFLAGS = -pthread -lm 

# Directories
SRC_DIR = src
INCLUDE_DIR = include
BUILD_DIR = build
TEST_DIR = tests

# Target executable
TARGET = ushell

# Source files
SRCS = src/main.c \
       src/evaluator/environment.c \
       src/evaluator/executor.c \
       src/evaluator/conditional.c \
       src/evaluator/arithmetic.c \
       src/builtins/builtins.c \
       src/builtins/builtin_edi.c \
       src/utils/expansion.c \
       src/utils/arg_parser.c \
       src/utils/history.c \
       src/utils/completion.c \
       src/utils/terminal.c \
       src/parser/Absyn.c \
       src/parser/Buffer.c \
       src/parser/Lexer.c \
       src/parser/Parser.c \
       src/parser/Printer.c \
       src/parser/Skeleton.c \
       src/tools/tool_dispatch.c \
       src/tools/myls.c \
       src/tools/mycat.c \
       src/tools/mycp.c \
       src/tools/mymv.c \
       src/tools/myrm.c \
       src/tools/mymkdir.c \
       src/tools/myrmdir.c \
       src/tools/mytouch.c \
       src/tools/mystat.c \
       src/tools/myfd.c \
       src/glob/glob.c \
       src/apt/repo.c \
       src/apt/apt_builtin.c \
       src/apt/install.c \
       src/apt/remove.c \
       src/apt/depends.c \
       src/jobs/jobs.c \
       src/jobs/signals.c \
       src/threading/threading.c \
       src/help/help.c \
       src/argtable3/arg_cmd.c \
       src/argtable3/arg_date.c \
       src/argtable3/arg_dbl.c \
       src/argtable3/arg_dstr.c \
       src/argtable3/arg_end.c \
       src/argtable3/arg_file.c \
       src/argtable3/arg_getopt_long.c \
       src/argtable3/arg_hashtable.c \
       src/argtable3/arg_int.c \
       src/argtable3/arg_lit.c \
       src/argtable3/arg_rem.c \
       src/argtable3/arg_rex.c \
       src/argtable3/arg_str.c \
       src/argtable3/arg_utils.c \
       src/argtable3/argtable3.c

# Object files
OBJS = $(SRCS:.c=.o)

# Default target
all: $(TARGET)

# Build executable
$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

# Compile source files
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Clean build artifacts
clean:
	rm -f $(OBJS) $(TARGET)
	rm -f $(SRC_DIR)/*/*.o $(SRC_DIR)/*/*/*.o
	rm -f *.o

# Run tests
test:
	@echo "Running tests..."
	@cd $(TEST_DIR) && ./test_runner.sh

# Valgrind memory check
valgrind: $(TARGET)
	valgrind --leak-check=full --show-leak-kinds=all ./$(TARGET)

# Generate parser (if using BNFC)
parser:
	@echo "Generating parser from grammar..."
	cd $(SRC_DIR)/parser && bnfc -c --makefile Grammar.cf

# Help
help:
	@echo "Available targets:"
	@echo "  all      - Build the shell (default)"
	@echo "  clean    - Remove build artifacts"
	@echo "  test     - Run test suite"
	@echo "  valgrind - Run with memory checker"
	@echo "  parser   - Generate parser from Grammar.cf"
	@echo "  help     - Show this message"

.PHONY: all clean test valgrind parser help
