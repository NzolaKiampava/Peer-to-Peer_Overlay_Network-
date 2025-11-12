#include "../include/peer_server.h"
#include "../include/network.h"

/* Cria servidor de peers */
PeerServer* peer_server_create(int port) {
    PeerServer *server = malloc(sizeof(PeerServer));
    if (!server) {
        perror("malloc");
        return NULL;
    }
    
    memset(server, 0, sizeof(PeerServer));
    server->port = port;
    server->next_seqnumber = 1;
    
    server->sockfd = create_udp_socket();
    if (server->sockfd < 0) {
        free(server);
        return NULL;
    }
    
    /* Bind socket */
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);
    
    if (bind(server->sockfd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("bind");
        close(server->sockfd);
        free(server);
        return NULL;
    }
    
    printf("[SERVIDOR] Servidor de peers iniciado na porta %d\n", port);
    return server;
}

/* Inicia servidor */
void peer_server_start(PeerServer *server) {
    char buffer[BUFFER_SIZE];
    char response[BUFFER_SIZE];
    struct sockaddr_in client_addr;
    
    printf("[SERVIDOR] Aguardando requisições...\n");
    
    while (1) {
        memset(buffer, 0, sizeof(buffer));
        memset(response, 0, sizeof(response));
        
        if (udp_receive(server->sockfd, buffer, sizeof(buffer), 
                       &client_addr) < 0) {
            continue;
        }
        
        printf("[RECV] %s\n", buffer);
        
        /* Processa comando */
        if (strncmp(buffer, "REG ", 4) == 0) {
            handle_register(server, buffer, &client_addr, response);
        } else if (strncmp(buffer, "UNR ", 4) == 0) {
            handle_unregister(server, buffer, response);
        } else if (strcmp(buffer, "PEERS") == 0) {
            handle_peers_list(server, response);
        } else {
            strcpy(response, "NOK");
        }
        
        /* Envia resposta */
        sendto(server->sockfd, response, strlen(response), 0,
               (struct sockaddr *)&client_addr, sizeof(client_addr));
        printf("[SEND] %s\n", response);
    }
}

/* Handler REG */
void handle_register(PeerServer *server, const char *message, 
                    struct sockaddr_in *client_addr, char *response) {
    int lnkport;
    
    if (sscanf(message, "REG %d", &lnkport) != 1) {
        strcpy(response, "NOK");
        return;
    }
    
    if (server->count >= MAX_PEERS) {
        strcpy(response, "NOK");
        return;
    }
    
    /* Adiciona peer */
    PeerServerEntry *entry = &server->entries[server->count];
    inet_ntop(AF_INET, &client_addr->sin_addr, entry->ip, sizeof(entry->ip));
    entry->port = lnkport;
    entry->seqnumber = server->next_seqnumber;
    
    server->count++;
    server->next_seqnumber++;
    
    sprintf(response, "SQN %d", entry->seqnumber);
    printf("[REG] Peer %s:%d registrado com seqnumber %d\n", 
           entry->ip, entry->port, entry->seqnumber);
}

/* Handler UNR */
void handle_unregister(PeerServer *server, const char *message, char *response) {
    int seqnumber;
    
    if (sscanf(message, "UNR %d", &seqnumber) != 1) {
        strcpy(response, "NOK");
        return;
    }
    
    /* Procura peer */
    int found = -1;
    for (int i = 0; i < server->count; i++) {
        if (server->entries[i].seqnumber == seqnumber) {
            found = i;
            break;
        }
    }
    
    if (found < 0) {
        strcpy(response, "NOK");
        return;
    }
    
    /* Remove peer */
    for (int i = found; i < server->count - 1; i++) {
        server->entries[i] = server->entries[i + 1];
    }
    server->count--;
    
    strcpy(response, "OK");
    printf("[UNR] Peer com seqnumber %d removido\n", seqnumber);
}

/* Handler PEERS */
void handle_peers_list(PeerServer *server, char *response) {
    strcpy(response, "LST\n");
    
    for (int i = 0; i < server->count; i++) {
        char line[256];
        sprintf(line, "%s:%d#%d\n", 
                server->entries[i].ip,
                server->entries[i].port,
                server->entries[i].seqnumber);
        strcat(response, line);
    }
    
    strcat(response, "\n");
}

/* Destroi servidor */
void peer_server_destroy(PeerServer *server) {
    if (server) {
        close_socket(server->sockfd);
        free(server);
    }
}
