# Makefile for server and client programs

# Compiler
CC = gcc

# Compiler flags
CFLAGS = -Wall -g

# Targets
TARGETS = server client

# Build all targets (default rule)
all: $(TARGETS)

# Rule to build server executable
server: server.c
	$(CC) $(CFLAGS) server.c -o server

# Rule to build client executable
client: client.c
	$(CC) $(CFLAGS) client.c -o client

# Clean up the compiled files
clean:
	rm -f $(TARGETS)

# Phony targets to avoid conflicts with files named 'all' or 'clean'
.PHONY: all clean
