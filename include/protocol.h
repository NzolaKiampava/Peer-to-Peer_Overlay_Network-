#ifndef PROTOCOL_H
#define PROTOCOL_H

#include "common.h"

/* Protocolos de ligação overlay */
int protocol_link_request(int sockfd, int my_seqnumber);
int protocol_force_request(int sockfd, int my_seqnumber);
int protocol_handle_link(PeerState *state, int client_fd, 
                         const char *message, char *client_ip);

/* Protocolos de pesquisa */
int protocol_query_identifier(int sockfd, const char *identifier, int hopcount);
int protocol_handle_query(PeerState *state, int sockfd, const char *message);

/* Gestão de vizinhos */
int add_neighbor(PeerState *state, PeerInfo *peer, int sockfd, int is_external);
int remove_neighbor(PeerState *state, int seqnumber);
int count_external_neighbors(PeerState *state);
int count_internal_neighbors(PeerState *state);
Neighbor* find_neighbor_by_seqnumber(PeerState *state, int seqnumber);
Neighbor* find_internal_with_higher_seqnumber(PeerState *state, int seqnumber);

/* Estabelecimento de ligações */
int establish_connections(PeerState *state, PeerList *peer_list);
int reconnect_after_disconnect(PeerState *state);

#endif /* PROTOCOL_H */
