#ifndef PEER_SERVER_H
#define PEER_SERVER_H

#include "common.h"

/* Estrutura do servidor de peers */
typedef struct {
    PeerServerEntry entries[MAX_PEERS];
    int count;
    int next_seqnumber;
    int sockfd;
    int port;
} PeerServer;

/* Funções do servidor de peers */
PeerServer* peer_server_create(int port);
void peer_server_start(PeerServer *server);
void peer_server_destroy(PeerServer *server);

/* Handlers de comandos */
void handle_register(PeerServer *server, const char *message, 
                    struct sockaddr_in *client_addr, char *response);
void handle_unregister(PeerServer *server, const char *message, char *response);
void handle_peers_list(PeerServer *server, char *response);

#endif /* PEER_SERVER_H */
