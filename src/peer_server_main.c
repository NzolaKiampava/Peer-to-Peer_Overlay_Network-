#include "../include/peer_server.h"
#include <signal.h>

static volatile int running = 1;

void signal_handler(int sig __attribute__((unused))) {
    running = 0;
    printf("\n[SERVIDOR] A encerrar...\n");
}

int main(int argc, char *argv[]) {
    int port = DEFAULT_SERVER_PORT;
    
    /* Parse argumentos */
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-p") == 0 && i + 1 < argc) {
            port = atoi(argv[i + 1]);
            i++;
        }
    }
    
    signal(SIGINT, signal_handler);
    
    PeerServer *server = peer_server_create(port);
    if (!server) {
        fprintf(stderr, "Erro ao criar servidor de peers.\n");
        return 1;
    }
    
    peer_server_start(server);
    
    peer_server_destroy(server);
    return 0;
}
