CC = gcc
CFLAGS = -Wall -Wextra -I./src
SRC = src/main.c src/diskio.c src/fat32.c src/commands.c src/shell.c
OBJ = $(SRC:.c=.o)  # Converte os arquivos .c em .o
TARGET = fatshell

# Alvo principal
all: $(TARGET)

# Como gerar o executável
$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) -o $@ $^

# Como compilar os arquivos .c para .o
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Limpeza
clean:
	rm -f $(TARGET) $(OBJ)
