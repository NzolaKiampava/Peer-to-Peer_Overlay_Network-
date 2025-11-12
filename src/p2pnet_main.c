#include "../include/common.h"
#include "../include/ui.h"
#include "../include/network.h"
#include "../include/protocol.h"
#include <pthread.h>

/* Thread para aceitar conexões TCP */
void* accept_connections_thread(void *arg) {
    PeerState *state = (PeerState *)arg;
    
    while (state->joined) {
        char client_ip[16];
        int client_fd = tcp_accept(state->tcp_server_fd, client_ip);
        
        if (client_fd < 0) {
            continue;
        }
        
        /* Recebe mensagem */
        char buffer[BUFFER_SIZE];
        if (tcp_receive(client_fd, buffer, sizeof(buffer)) > 0) {
            printf("[CONEXÃO] Recebido: %s", buffer);
            protocol_handle_link(state, client_fd, buffer, client_ip);
        } else {
            close_socket(client_fd);
        }
    }
    
    return NULL;
}

int main(int argc, char *argv[]) {
    PeerState state;
    memset(&state, 0, sizeof(state));
    
    /* Valores padrão */
    strcpy(state.server_ip, DEFAULT_SERVER_IP);
    state.server_port = DEFAULT_SERVER_PORT;
    state.tcp_port = 0;
    state.max_neighbors = 3;
    state.max_hopcount = DEFAULT_HOPCOUNT;
    
    /* Parse argumentos */
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-s") == 0 && i + 1 < argc) {
            strcpy(state.server_ip, argv[i + 1]);
            i++;
        } else if (strcmp(argv[i], "-p") == 0 && i + 1 < argc) {
            state.server_port = atoi(argv[i + 1]);
            i++;
        } else if (strcmp(argv[i], "-l") == 0 && i + 1 < argc) {
            state.tcp_port = atoi(argv[i + 1]);
            i++;
        } else if (strcmp(argv[i], "-n") == 0 && i + 1 < argc) {
            state.max_neighbors = atoi(argv[i + 1]);
            i++;
        } else if (strcmp(argv[i], "-h") == 0 && i + 1 < argc) {
            state.max_hopcount = atoi(argv[i + 1]);
            i++;
        }
    }
    
    /* Validações */
    if (state.tcp_port == 0) {
        fprintf(stderr, "Erro: porta TCP (-l) é obrigatória\n");
        return 1;
    }
    
    if (state.max_neighbors <= 0) {
        fprintf(stderr, "Erro: número de vizinhos (-n) deve ser > 0\n");
        return 1;
    }
    
    if (state.max_hopcount <= 0) {
        fprintf(stderr, "Erro: hopcount (-h) deve ser > 0\n");
        return 1;
    }
    
    /* Cria servidor TCP */
    state.tcp_server_fd = create_tcp_server(state.tcp_port);
    if (state.tcp_server_fd < 0) {
        fprintf(stderr, "Erro ao criar servidor TCP na porta %d\n", 
                state.tcp_port);
        return 1;
    }
    
    printf("P2P Overlay Network\n");
    printf("Servidor TCP na porta %d\n", state.tcp_port);
    printf("Servidor de peers: %s:%d\n", state.server_ip, state.server_port);
    printf("Max vizinhos: %d, Max hopcount: %d\n\n", 
           state.max_neighbors, state.max_hopcount);
    
    /* Thread para aceitar conexões */
    pthread_t accept_thread;
    pthread_create(&accept_thread, NULL, accept_connections_thread, &state);
    
    /* Interface de usuário */
    ui_start(&state);
    
    /* Cleanup */
    close_socket(state.tcp_server_fd);
    pthread_cancel(accept_thread);
    pthread_join(accept_thread, NULL);
    
    return 0;
}
