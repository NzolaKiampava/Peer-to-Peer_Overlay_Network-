#ifndef PEER_CLIENT_H
#define PEER_CLIENT_H

#include "common.h"

/* Funções do cliente para servidor de peers */
int peer_client_register(const char *server_ip, int server_port, 
                         int tcp_port, int *seqnumber);
int peer_client_unregister(const char *server_ip, int server_port, 
                           int seqnumber);
int peer_client_get_peers(const char *server_ip, int server_port, 
                          PeerList *peer_list);

#endif /* PEER_CLIENT_H */
