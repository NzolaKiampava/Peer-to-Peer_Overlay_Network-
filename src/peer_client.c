#include "../include/peer_client.h"
#include "../include/network.h"

/* Registra peer no servidor */
int peer_client_register(const char *server_ip, int server_port, 
                         int tcp_port, int *seqnumber) {
    int sockfd = create_udp_socket();
    if (sockfd < 0) {
        return -1;
    }
    
    char message[256];
    sprintf(message, "REG %d", tcp_port);
    
    if (udp_send(sockfd, server_ip, server_port, message) < 0) {
        close_socket(sockfd);
        return -1;
    }
    
    char response[BUFFER_SIZE];
    struct sockaddr_in from_addr;
    
    if (udp_receive(sockfd, response, sizeof(response), &from_addr) < 0) {
        close_socket(sockfd);
        return -1;
    }
    
    if (sscanf(response, "SQN %d", seqnumber) == 1) {
        close_socket(sockfd);
        return 0;
    }
    
    close_socket(sockfd);
    return -1;
}

/* Remove registro do peer */
int peer_client_unregister(const char *server_ip, int server_port, 
                           int seqnumber) {
    int sockfd = create_udp_socket();
    if (sockfd < 0) {
        return -1;
    }
    
    char message[256];
    sprintf(message, "UNR %d", seqnumber);
    
    if (udp_send(sockfd, server_ip, server_port, message) < 0) {
        close_socket(sockfd);
        return -1;
    }
    
    char response[BUFFER_SIZE];
    struct sockaddr_in from_addr;
    
    if (udp_receive(sockfd, response, sizeof(response), &from_addr) < 0) {
        close_socket(sockfd);
        return -1;
    }
    
    int result = (strcmp(response, "OK") == 0) ? 0 : -1;
    close_socket(sockfd);
    return result;
}

/* Obtém lista de peers */
int peer_client_get_peers(const char *server_ip, int server_port, 
                          PeerList *peer_list) {
    int sockfd = create_udp_socket();
    if (sockfd < 0) {
        return -1;
    }
    
    if (udp_send(sockfd, server_ip, server_port, "PEERS") < 0) {
        close_socket(sockfd);
        return -1;
    }
    
    char response[BUFFER_SIZE];
    struct sockaddr_in from_addr;
    
    if (udp_receive(sockfd, response, sizeof(response), &from_addr) < 0) {
        close_socket(sockfd);
        return -1;
    }
    
    close_socket(sockfd);
    
    /* Parse resposta */
    peer_list->count = 0;
    
    char *line = strtok(response, "\n");
    if (!line || strcmp(line, "LST") != 0) {
        return -1;
    }
    
    while ((line = strtok(NULL, "\n")) != NULL) {
        if (strlen(line) == 0) break;
        
        PeerInfo *peer = &peer_list->peers[peer_list->count];
        int port, seqnum;
        
        if (sscanf(line, "%15[^:]:%d#%d", peer->ip, &port, &seqnum) == 3) {
            peer->port = port;
            peer->seqnumber = seqnum;
            peer_list->count++;
            
            if (peer_list->count >= MAX_PEERS) break;
        }
    }
    
    return 0;
}
