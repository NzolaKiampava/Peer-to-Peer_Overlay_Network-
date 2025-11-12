#include "../include/ui.h"
#include "../include/peer_client.h"
#include "../include/protocol.h"
#include "../include/network.h"
#include <signal.h>

static volatile int running = 1;

void signal_handler(int sig __attribute__((unused))) {
    running = 0;
}

/* Inicia interface */
void ui_start(PeerState *state) {
    char command[256];
    
    signal(SIGINT, signal_handler);
    
    print_help();
    
    while (running) {
        print_prompt();
        
        if (fgets(command, sizeof(command), stdin) == NULL) {
            break;
        }
        
        command[strcspn(command, "\n")] = 0;
        
        if (strlen(command) == 0) {
            continue;
        }
        
        ui_process_command(state, command);
    }
    
    if (state->joined) {
        cmd_leave(state);
    }
}

/* Processa comando */
void ui_process_command(PeerState *state, const char *command) {
    char arg[256];
    
    if (strcmp(command, "join") == 0) {
        cmd_join(state);
    } else if (strcmp(command, "leave") == 0) {
        cmd_leave(state);
    } else if (strcmp(command, "show neighbors") == 0) {
        cmd_show_neighbors(state);
    } else if (sscanf(command, "release %d", &(int){0}) == 1) {
        int seqnum;
        sscanf(command, "release %d", &seqnum);
        cmd_release(state, seqnum);
    } else if (strcmp(command, "list identifiers") == 0) {
        cmd_list_identifiers(state);
    } else if (sscanf(command, "post %s", arg) == 1) {
        cmd_post(state, arg);
    } else if (sscanf(command, "search %s", arg) == 1) {
        cmd_search(state, arg);
    } else if (sscanf(command, "unpost %s", arg) == 1) {
        cmd_unpost(state, arg);
    } else if (strcmp(command, "exit") == 0) {
        cmd_exit(state);
        running = 0;
    } else if (strcmp(command, "help") == 0) {
        print_help();
    } else {
        printf("Comando desconhecido. Digite 'help' para ajuda.\n");
    }
}

/* Comando join */
void cmd_join(PeerState *state) {
    if (state->joined) {
        printf("Já está na rede.\n");
        return;
    }
    
    /* Regista no servidor */
    if (peer_client_register(state->server_ip, state->server_port,
                            state->tcp_port, &state->seqnumber) < 0) {
        printf("Erro ao registar no servidor de peers.\n");
        return;
    }
    
    printf("Registado com seqnumber %d\n", state->seqnumber);
    
    /* Obtém lista de peers */
    PeerList peer_list;
    if (peer_client_get_peers(state->server_ip, state->server_port, 
                             &peer_list) < 0) {
        printf("Erro ao obter lista de peers.\n");
        peer_client_unregister(state->server_ip, state->server_port, 
                              state->seqnumber);
        return;
    }
    
    printf("Obtidos %d peers da rede.\n", peer_list.count);
    
    /* Estabelece ligações */
    int connected = establish_connections(state, &peer_list);
    
    state->joined = 1;
    
    if (connected > 0) {
        printf("Ligado à rede com %d vizinhos.\n", connected);
    } else if (peer_list.count == 0) {
        printf("És o primeiro peer na rede.\n");
    } else {
        printf("Aviso: Não conseguiu ligar-se a nenhum peer.\n");
    }
}

/* Comando leave */
void cmd_leave(PeerState *state) {
    if (!state->joined) {
        printf("Não está na rede.\n");
        return;
    }
    
    /* Remove registo */
    peer_client_unregister(state->server_ip, state->server_port, 
                          state->seqnumber);
    
    /* Fecha todas ligações */
    for (int i = 0; i < state->neighbors.count; i++) {
        close_socket(state->neighbors.neighbors[i].socket_fd);
    }
    
    state->neighbors.count = 0;
    state->joined = 0;
    
    printf("Abandonou a rede.\n");
}

/* Comando show neighbors */
void cmd_show_neighbors(PeerState *state) {
    if (!state->joined) {
        printf("Não está na rede.\n");
        return;
    }
    
    int external = 0, internal = 0;
    
    printf("\nVizinhos Externos:\n");
    for (int i = 0; i < state->neighbors.count; i++) {
        Neighbor *n = &state->neighbors.neighbors[i];
        if (n->is_external) {
            printf("  %s:%d seqnumber=%d\n", 
                   n->info.ip, n->info.port, n->info.seqnumber);
            external++;
        }
    }
    if (external == 0) printf("  (nenhum)\n");
    
    printf("\nVizinhos Internos:\n");
    for (int i = 0; i < state->neighbors.count; i++) {
        Neighbor *n = &state->neighbors.neighbors[i];
        if (!n->is_external) {
            printf("  %s:%d seqnumber=%d\n", 
                   n->info.ip, n->info.port, n->info.seqnumber);
            internal++;
        }
    }
    if (internal == 0) printf("  (nenhum)\n");
    
    printf("\n");
}

/* Comando release */
void cmd_release(PeerState *state, int seqnumber) {
    if (!state->joined) {
        printf("Não está na rede.\n");
        return;
    }
    
    Neighbor *neighbor = find_neighbor_by_seqnumber(state, seqnumber);
    if (!neighbor) {
        printf("Vizinho com seqnumber %d não encontrado.\n", seqnumber);
        return;
    }
    
    if (neighbor->is_external) {
        printf("Apenas pode remover vizinhos internos.\n");
        return;
    }
    
    if (remove_neighbor(state, seqnumber) == 0) {
        printf("Ligação com peer %d removida.\n", seqnumber);
    }
}

/* Comando list identifiers */
void cmd_list_identifiers(PeerState *state) {
    printf("\nIdentificadores conhecidos:\n");
    
    if (state->identifiers.count == 0) {
        printf("  (nenhum)\n");
    } else {
        for (int i = 0; i < state->identifiers.count; i++) {
            printf("  %s\n", state->identifiers.identifiers[i]);
        }
    }
    
    printf("\n");
}

/* Comando post */
void cmd_post(PeerState *state, const char *identifier) {
    if (state->identifiers.count >= MAX_IDENTIFIERS) {
        printf("Lista de identificadores cheia.\n");
        return;
    }
    
    /* Verifica se já existe */
    for (int i = 0; i < state->identifiers.count; i++) {
        if (strcmp(state->identifiers.identifiers[i], identifier) == 0) {
            printf("Identificador já existe.\n");
            return;
        }
    }
    
    strcpy(state->identifiers.identifiers[state->identifiers.count], identifier);
    state->identifiers.count++;
    
    printf("Identificador '%s' adicionado.\n", identifier);
}

/* Comando search */
void cmd_search(PeerState *state, const char *identifier) {
    if (!state->joined) {
        printf("Não está na rede.\n");
        return;
    }
    
    /* Verifica se já tem */
    for (int i = 0; i < state->identifiers.count; i++) {
        if (strcmp(state->identifiers.identifiers[i], identifier) == 0) {
            printf("Identificador '%s' já conhecido.\n", identifier);
            return;
        }
    }
    
    /* Pergunta aos vizinhos */
    int found = 0;
    for (int i = 0; i < state->neighbors.count; i++) {
        Neighbor *n = &state->neighbors.neighbors[i];
        
        protocol_query_identifier(n->socket_fd, identifier, 
                                 state->max_hopcount);
        
        char response[256];
        if (tcp_receive(n->socket_fd, response, sizeof(response)) > 0) {
            if (strncmp(response, "FND", 3) == 0) {
                found = 1;
                break;
            }
        }
    }
    
    if (found) {
        strcpy(state->identifiers.identifiers[state->identifiers.count], 
               identifier);
        state->identifiers.count++;
        printf("Identificador '%s' encontrado e adicionado.\n", identifier);
    } else {
        printf("Identificador '%s' não encontrado na rede.\n", identifier);
    }
}

/* Comando unpost */
void cmd_unpost(PeerState *state, const char *identifier) {
    for (int i = 0; i < state->identifiers.count; i++) {
        if (strcmp(state->identifiers.identifiers[i], identifier) == 0) {
            for (int j = i; j < state->identifiers.count - 1; j++) {
                strcpy(state->identifiers.identifiers[j],
                       state->identifiers.identifiers[j + 1]);
            }
            state->identifiers.count--;
            printf("Identificador '%s' removido.\n", identifier);
            return;
        }
    }
    
    printf("Identificador '%s' não encontrado.\n", identifier);
}

/* Comando exit */
void cmd_exit(PeerState *state) {
    printf("A sair...\n");
    cmd_leave(state);
}

/* Imprime prompt */
void print_prompt() {
    printf("p2pnet> ");
    fflush(stdout);
}

/* Imprime ajuda */
void print_help() {
    printf("\n=== P2P Overlay Network ===\n");
    printf("Comandos disponíveis:\n");
    printf("  join                    - Entrar na rede\n");
    printf("  leave                   - Sair da rede\n");
    printf("  show neighbors          - Mostrar vizinhos\n");
    printf("  release <seqnumber>     - Remover vizinho interno\n");
    printf("  list identifiers        - Listar identificadores\n");
    printf("  post <id>               - Adicionar identificador\n");
    printf("  search <id>             - Pesquisar identificador\n");
    printf("  unpost <id>             - Remover identificador\n");
    printf("  exit                    - Sair da aplicação\n");
    printf("  help                    - Mostrar esta ajuda\n");
    printf("\n");
}
