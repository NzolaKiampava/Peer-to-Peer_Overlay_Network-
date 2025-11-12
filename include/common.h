#ifndef COMMON_H
#define COMMON_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/select.h>
#include <errno.h>

/* Constantes */
#define MAX_NEIGHBORS 10
#define MAX_IDENTIFIERS 100
#define MAX_PEERS 100
#define BUFFER_SIZE 4096
#define MAX_IDENTIFIER_LEN 64
#define DEFAULT_SERVER_IP "192.168.56.21"
#define DEFAULT_SERVER_PORT 58000
#define DEFAULT_HOPCOUNT 5

/* Estruturas de Dados */

/* Informação de um peer */
typedef struct {
    char ip[16];
    int port;
    int seqnumber;
} PeerInfo;

/* Informação de um vizinho */
typedef struct {
    PeerInfo info;
    int socket_fd;
    int is_external;  /* 1 = vizinho externo, 0 = vizinho interno */
} Neighbor;

/* Lista de vizinhos */
typedef struct {
    Neighbor neighbors[MAX_NEIGHBORS * 2];
    int count;
} NeighborList;

/* Lista de identificadores */
typedef struct {
    char identifiers[MAX_IDENTIFIERS][MAX_IDENTIFIER_LEN];
    int count;
} IdentifierList;

/* Lista de peers do servidor */
typedef struct {
    PeerInfo peers[MAX_PEERS];
    int count;
} PeerList;

/* Estado do peer */
typedef struct {
    int seqnumber;
    int tcp_port;
    int tcp_server_fd;
    int joined;
    NeighborList neighbors;
    IdentifierList identifiers;
    
    /* Configuração */
    char server_ip[16];
    int server_port;
    int max_neighbors;
    int max_hopcount;
} PeerState;

/* Registro no servidor de peers */
typedef struct {
    char ip[16];
    int port;
    int seqnumber;
} PeerServerEntry;

/* Funções auxiliares */
void error_exit(const char *msg);
void log_message(const char *format, ...);

#endif /* COMMON_H */
