#include "../include/protocol.h"
#include "../include/network.h"
#include "../include/peer_client.h"

/* Envia pedido LNK */
int protocol_link_request(int sockfd, int my_seqnumber) {
    char message[256];
    sprintf(message, "LNK %d\n", my_seqnumber);
    return tcp_send(sockfd, message);
}

/* Envia pedido FRC */
int protocol_force_request(int sockfd, int my_seqnumber) {
    char message[256];
    sprintf(message, "FRC %d\n", my_seqnumber);
    return tcp_send(sockfd, message);
}

/* Trata pedido de ligação */
int protocol_handle_link(PeerState *state, int client_fd, 
                         const char *message, char *client_ip) {
    int client_seqnumber;
    int is_force = 0;
    
    if (sscanf(message, "LNK %d", &client_seqnumber) == 1) {
        is_force = 0;
    } else if (sscanf(message, "FRC %d", &client_seqnumber) == 1) {
        is_force = 1;
    } else {
        return -1;
    }
    
    int internal_count = count_internal_neighbors(state);
    
    /* Pode aceitar a ligação */
    if (internal_count < state->max_neighbors) {
        tcp_send(client_fd, "CNF\n");
        
        PeerInfo peer_info;
        strcpy(peer_info.ip, client_ip);
        peer_info.port = 0; /* Não sabemos porta do peer cliente */
        peer_info.seqnumber = client_seqnumber;
        
        add_neighbor(state, &peer_info, client_fd, 0);
        printf("[PROTOCOLO] Ligação aceite de peer %d\n", client_seqnumber);
        return 0;
    }
    
    /* Se FRC e tem vizinho com seqnumber maior */
    if (is_force) {
        Neighbor *higher = find_internal_with_higher_seqnumber(state, 
                                                               client_seqnumber);
        if (higher) {
            tcp_send(client_fd, "CNF\n");
            
            PeerInfo peer_info;
            strcpy(peer_info.ip, client_ip);
            peer_info.port = 0;
            peer_info.seqnumber = client_seqnumber;
            
            /* Remove vizinho com seqnumber maior */
            int removed_seq = higher->info.seqnumber;
            remove_neighbor(state, removed_seq);
            
            /* Adiciona novo vizinho */
            add_neighbor(state, &peer_info, client_fd, 0);
            
            printf("[PROTOCOLO] Ligação FRC aceite de peer %d, removido peer %d\n",
                   client_seqnumber, removed_seq);
            return 0;
        }
    }
    
    /* Não pode aceitar */
    close_socket(client_fd);
    return -1;
}

/* Envia query de identificador */
int protocol_query_identifier(int sockfd, const char *identifier, int hopcount) {
    char message[256];
    sprintf(message, "QRY %s %d\n", identifier, hopcount);
    return tcp_send(sockfd, message);
}

/* Trata query de identificador */
int protocol_handle_query(PeerState *state, int sockfd, const char *message) {
    char identifier[MAX_IDENTIFIER_LEN];
    int hopcount;
    
    if (sscanf(message, "QRY %s %d", identifier, &hopcount) != 2) {
        return -1;
    }
    
    /* Verifica se tem identificador */
    for (int i = 0; i < state->identifiers.count; i++) {
        if (strcmp(state->identifiers.identifiers[i], identifier) == 0) {
            char response[256];
            sprintf(response, "FND %s\n", identifier);
            tcp_send(sockfd, response);
            return 1;
        }
    }
    
    /* Se hopcount > 1, pergunta aos vizinhos */
    if (hopcount > 1) {
        for (int i = 0; i < state->neighbors.count; i++) {
            Neighbor *neighbor = &state->neighbors.neighbors[i];
            protocol_query_identifier(neighbor->socket_fd, identifier, 
                                    hopcount - 1);
            
            char response[256];
            if (tcp_receive(neighbor->socket_fd, response, 
                          sizeof(response)) > 0) {
                if (strncmp(response, "FND", 3) == 0) {
                    char fwd[256];
                    sprintf(fwd, "FND %s\n", identifier);
                    tcp_send(sockfd, fwd);
                    return 1;
                }
            }
        }
    }
    
    char response[256];
    sprintf(response, "NOTFND %s\n", identifier);
    tcp_send(sockfd, response);
    return 0;
}

/* Adiciona vizinho */
int add_neighbor(PeerState *state, PeerInfo *peer, int sockfd, int is_external) {
    if (state->neighbors.count >= MAX_NEIGHBORS * 2) {
        return -1;
    }
    
    Neighbor *neighbor = &state->neighbors.neighbors[state->neighbors.count];
    neighbor->info = *peer;
    neighbor->socket_fd = sockfd;
    neighbor->is_external = is_external;
    
    state->neighbors.count++;
    return 0;
}

/* Remove vizinho por seqnumber */
int remove_neighbor(PeerState *state, int seqnumber) {
    for (int i = 0; i < state->neighbors.count; i++) {
        if (state->neighbors.neighbors[i].info.seqnumber == seqnumber) {
            close_socket(state->neighbors.neighbors[i].socket_fd);
            
            for (int j = i; j < state->neighbors.count - 1; j++) {
                state->neighbors.neighbors[j] = state->neighbors.neighbors[j + 1];
            }
            
            state->neighbors.count--;
            return 0;
        }
    }
    return -1;
}

/* Conta vizinhos externos */
int count_external_neighbors(PeerState *state) {
    int count = 0;
    for (int i = 0; i < state->neighbors.count; i++) {
        if (state->neighbors.neighbors[i].is_external) {
            count++;
        }
    }
    return count;
}

/* Conta vizinhos internos */
int count_internal_neighbors(PeerState *state) {
    int count = 0;
    for (int i = 0; i < state->neighbors.count; i++) {
        if (!state->neighbors.neighbors[i].is_external) {
            count++;
        }
    }
    return count;
}

/* Encontra vizinho por seqnumber */
Neighbor* find_neighbor_by_seqnumber(PeerState *state, int seqnumber) {
    for (int i = 0; i < state->neighbors.count; i++) {
        if (state->neighbors.neighbors[i].info.seqnumber == seqnumber) {
            return &state->neighbors.neighbors[i];
        }
    }
    return NULL;
}

/* Encontra vizinho interno com seqnumber maior */
Neighbor* find_internal_with_higher_seqnumber(PeerState *state, int seqnumber) {
    for (int i = 0; i < state->neighbors.count; i++) {
        Neighbor *n = &state->neighbors.neighbors[i];
        if (!n->is_external && n->info.seqnumber > seqnumber) {
            return n;
        }
    }
    return NULL;
}

/* Estabelece ligações iniciais */
int establish_connections(PeerState *state, PeerList *peer_list) {
    int connected = 0;
    
    for (int i = 0; i < peer_list->count && connected < state->max_neighbors; i++) {
        PeerInfo *peer = &peer_list->peers[i];
        
        /* Apenas conecta com peers de seqnumber menor */
        if (peer->seqnumber >= state->seqnumber) {
            continue;
        }
        
        int sockfd = tcp_connect(peer->ip, peer->port);
        if (sockfd < 0) {
            continue;
        }
        
        if (protocol_link_request(sockfd, state->seqnumber) < 0) {
            close_socket(sockfd);
            continue;
        }
        
        char response[256];
        if (tcp_receive(sockfd, response, sizeof(response)) < 0) {
            close_socket(sockfd);
            continue;
        }
        
        if (strncmp(response, "CNF", 3) == 0) {
            add_neighbor(state, peer, sockfd, 1);
            connected++;
            printf("[CONEXÃO] Ligado ao peer %d\n", peer->seqnumber);
        } else {
            close_socket(sockfd);
        }
    }
    
    return connected;
}

/* Reconecta após desconexão */
int reconnect_after_disconnect(PeerState *state) {
    if (count_external_neighbors(state) > 0) {
        return 0;
    }
    
    PeerList peer_list;
    if (peer_client_get_peers(state->server_ip, state->server_port, 
                             &peer_list) < 0) {
        return -1;
    }
    
    int connected = establish_connections(state, &peer_list);
    
    /* Se ainda não conseguiu, tenta FRC */
    if (connected == 0) {
        for (int i = 0; i < peer_list.count; i++) {
            PeerInfo *peer = &peer_list.peers[i];
            
            if (peer->seqnumber >= state->seqnumber) {
                continue;
            }
            
            int sockfd = tcp_connect(peer->ip, peer->port);
            if (sockfd < 0) {
                continue;
            }
            
            if (protocol_force_request(sockfd, state->seqnumber) < 0) {
                close_socket(sockfd);
                continue;
            }
            
            char response[256];
            if (tcp_receive(sockfd, response, sizeof(response)) > 0 &&
                strncmp(response, "CNF", 3) == 0) {
                add_neighbor(state, peer, sockfd, 1);
                connected++;
                printf("[RECONEXÃO] Ligado ao peer %d com FRC\n", 
                       peer->seqnumber);
                break;
            }
            
            close_socket(sockfd);
        }
    }
    
    return connected;
}
