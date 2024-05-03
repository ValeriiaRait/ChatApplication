
# Compiler
CC := cc

# Directories
SRC_DIR := ./src
INC_DIR := ./inc
OBJ_DIR := ./obj

# Source files
SRC_FILES := $(wildcard $(SRC_DIR)/*.c)
OBJ_FILES := $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%.o,$(SRC_FILES))

# Compiler flags
CFLAGS := -Wall -Werror

# Targets
all: $(OBJ_FILES)

# Compile source files into object files
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	$(CC) $(CFLAGS) -I$(INC_DIR) -c $< -o $@

# Clean up object files
clean:
	rm -f $(OBJ_DIR)/*.o