# Compiler
CC := cc

# Directories
SRC_DIR := ./src
INC_DIR := ./inc
OBJ_DIR := ./obj
BIN_DIR := ./bin
COMMON_DIR := ../common

# Source files
SRC_FILES := $(wildcard $(SRC_DIR)/*.c)
OBJ_FILES := $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%.o,$(SRC_FILES))

# Compiler flags
CFLAGS := -Wall -Werror -I$(INC_DIR)

# Targets
all: $(BIN_DIR)/chat-server

# Compile source files into object files
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c | $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@
	
# Link object files and create executable
$(BIN_DIR)/chat-server: $(OBJ_FILES) | $(BIN_DIR)
	$(CC) $(CFLAGS) $^ $(COMMON_DIR)/obj/*.o -o $@

# Clean up object files
clean:
	rm -f $(OBJ_DIR)/*.o
	rm -f $(BIN_DIR)/chat-server
