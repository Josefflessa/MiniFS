# Nome do executável
TARGET = minifs

# Compilador C a ser usado
CC = gcc

# Opções de compilação
# -Wall: Habilita todos os warnings
# -g: Inclui informações de depuração (útil para gdb, valgrind)
CFLAGS = -Wall -g

# Arquivos fonte C
SRCS = fs.c main.c shell.c utils.c

# Objetos (arquivos .o) que serão gerados a partir dos fontes
OBJS = $(SRCS:.c=.o)

.PHONY: all clean

# Regra padrão: compila tudo
all: $(TARGET)

# Regra para construir o executável
$(TARGET): $(OBJS)
	$(CC) $(OBJS) -o $(TARGET)

# Regra para compilar arquivos .c em .o
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Regra para limpar arquivos compilados e de save/json
clean:
	rm -f $(OBJS) $(TARGET) minifs.dat fs_tree.json
