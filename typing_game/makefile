# Compiler
CC = gcc

# Compiler flags
CFLAGS = -lpthread

# Libraries for ncurses
NCURSES_LIB = -lncursesw

# Executable names
SERVER_EXEC = server
CLIENT_EXEC = client
GAME_EXEC = game

# Source files
SERVER_SRC = server.c
CLIENT_SRC = client.c
GAME_SRC = game.c

# Default target
all: $(SERVER_EXEC) $(CLIENT_EXEC) $(GAME_EXEC)

# Build server
$(SERVER_EXEC): $(SERVER_SRC)
	$(CC) $(SERVER_SRC) -o $(SERVER_EXEC) $(CFLAGS)

# Build client
$(CLIENT_EXEC): $(CLIENT_SRC)
	$(CC) $(CLIENT_SRC) -o $(CLIENT_EXEC) $(CFLAGS) $(NCURSES_LIB)

# Build game
$(GAME_EXEC): $(GAME_SRC)
	$(CC) $(GAME_SRC) -o $(GAME_EXEC) $(CFLAGS) $(NCURSES_LIB)

# Clean build artifacts
clean:
	rm -f $(SERVER_EXEC) $(CLIENT_EXEC) $(GAME_EXEC)

# Rebuild all
rebuild: clean all
