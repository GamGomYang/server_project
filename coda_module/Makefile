# Compiler and flags
CC = gcc
CFLAGS = -Wall -g
LDFLAGS = -lpthread -lncurses

# Targets and sources
SERVER_TARGET = server
CLIENT_TARGET = client

SERVER_SOURCES = s.c davinci.c
CLIENT_SOURCES = c.c

# Build server
$(SERVER_TARGET): $(SERVER_SOURCES)
	$(CC) $(CFLAGS) -o $(SERVER_TARGET) $(SERVER_SOURCES) -lpthread

# Build client
$(CLIENT_TARGET): $(CLIENT_SOURCES)
	$(CC) $(CFLAGS) -o $(CLIENT_TARGET) $(CLIENT_SOURCES) -lncurses

# Clean build files
clean:
	rm -f $(SERVER_TARGET) $(CLIENT_TARGET)

# Phony targets
.PHONY: clean
