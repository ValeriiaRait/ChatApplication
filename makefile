# Root Makefile for CHAT-SYSTEM project

# Directories
COMMON_DIR := ./common
CLIENT_DIR := ./chat-client
SERVER_DIR := ./chat-server

# Targets
.PHONY: all clean

all:
	$(MAKE) -C $(COMMON_DIR)
	$(MAKE) -C $(CLIENT_DIR)
	$(MAKE) -C $(SERVER_DIR)

# Clean
clean:
	$(MAKE) clean -C $(COMMON_DIR)
	$(MAKE) clean -C $(CLIENT_DIR)
	$(MAKE) clean -C $(SERVER_DIR)
	rm -f ./a.out

