CC = gcc
CFLAGS = -Wall -Wextra -I./include -pthread
LDFLAGS = -pthread

# Diretórios
SRC_DIR = src
INC_DIR = include
OBJ_DIR = obj
BIN_DIR = bin

# Criar diretórios
$(shell mkdir -p $(OBJ_DIR) $(BIN_DIR))

# Arquivos objeto
PEER_SERVER_OBJS = $(OBJ_DIR)/peer_server_main.o $(OBJ_DIR)/peer_server.o \
                   $(OBJ_DIR)/network.o
P2PNET_OBJS = $(OBJ_DIR)/p2pnet_main.o $(OBJ_DIR)/peer_client.o \
              $(OBJ_DIR)/network.o $(OBJ_DIR)/protocol.o $(OBJ_DIR)/ui.o

# Targets
all: $(BIN_DIR)/p2pnet_server $(BIN_DIR)/p2pnet

$(BIN_DIR)/p2pnet_server: $(PEER_SERVER_OBJS)
	$(CC) $(LDFLAGS) -o $@ $^

$(BIN_DIR)/p2pnet: $(P2PNET_OBJS)
	$(CC) $(LDFLAGS) -o $@ $^

# Compilação dos objetos
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

# Limpeza
clean:
	rm -rf $(OBJ_DIR) $(BIN_DIR)

# Dependências
$(OBJ_DIR)/peer_server_main.o: $(INC_DIR)/peer_server.h $(INC_DIR)/common.h
$(OBJ_DIR)/peer_server.o: $(INC_DIR)/peer_server.h $(INC_DIR)/network.h $(INC_DIR)/common.h
$(OBJ_DIR)/p2pnet_main.o: $(INC_DIR)/ui.h $(INC_DIR)/network.h $(INC_DIR)/protocol.h $(INC_DIR)/common.h
$(OBJ_DIR)/peer_client.o: $(INC_DIR)/peer_client.h $(INC_DIR)/network.h $(INC_DIR)/common.h
$(OBJ_DIR)/protocol.o: $(INC_DIR)/protocol.h $(INC_DIR)/network.h $(INC_DIR)/peer_client.h $(INC_DIR)/common.h
$(OBJ_DIR)/ui.o: $(INC_DIR)/ui.h $(INC_DIR)/peer_client.h $(INC_DIR)/protocol.h $(INC_DIR)/common.h
$(OBJ_DIR)/network.o: $(INC_DIR)/network.h $(INC_DIR)/common.h

.PHONY: all clean
