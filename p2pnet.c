/* p2pnet.c
 * Peer application for the p2pnet project (Linux/Windows)
 *
 * Uso:
 * p2pnet [-s addr] [-p prport] [-l lnkport] [-n neigh] [-h hc]
 *
 * Implementação funcional básica do protocolo descrito no enunciado.
 */

#ifdef _WIN32
#define _CRT_SECURE_NO_WARNINGS
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#pragma comment(lib, "ws2_32.lib")
typedef int socklen_t;
#else
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <fcntl.h>
#endif

#define BUFSIZE 2048
#define MAX_NEIGH 10
#define MAX_PEER_SLOTS 200
#define MAX_IDENTIFIERS 200
#define MAX_TCP_SOCKS 64

typedef struct {
    int active;
    char ip[INET_ADDRSTRLEN];
    int port;
    int seq;
} RemotePeer;

typedef struct {
    int sock;
    int seq; // remote seq
    int is_external; // 1 if we initiated (external to remote), 0 if remote initiated (internal)
    char ip[INET_ADDRSTRLEN];
    int port;
} TcpNeighbor;

static RemotePeer known_peers[MAX_PEER_SLOTS];
static int known_count = 0;

static TcpNeighbor tcp_neighbors[MAX_TCP_SOCKS];
static int neighbor_count = 0;

static char identifiers[MAX_IDENTIFIERS][128];
static int identifiers_count = 0;

static int seqnumber = -1;
static int neigh_limit = 2;
static int hopcount_limit = 3;
static char server_addr_str[64] = "192.168.56.21";
static int server_port = 58000;
static int lnkport = 60000;

#ifdef _WIN32
static int set_nonblock(SOCKET s) {
    unsigned long mode = 1;
    return ioctlsocket(s, FIONBIO, &mode);
}
#else
static int set_nonblock(int s) {
    int flags = fcntl(s, F_GETFL, 0);
    if (flags == -1) return -1;
    return fcntl(s, F_SETFL, flags | O_NONBLOCK);
}
#endif

/* Utility: add identifier */
void add_identifier(const char *id) {
    for (int i=0;i<identifiers_count;i++) if (strcmp(identifiers[i], id)==0) return;
    if (identifiers_count < MAX_IDENTIFIERS) {
        strncpy(identifiers[identifiers_count++], id, sizeof(identifiers[0])-1);
    }
}

/* Utility: remove identifier */
void remove_identifier(const char *id) {
    for (int i=0;i<identifiers_count;i++){
        if (strcmp(identifiers[i], id)==0) {
            for (int j=i;j<identifiers_count-1;j++) strcpy(identifiers[j], identifiers[j+1]);
            identifiers_count--;
            return;
        }
    }
}

/* send UDP to server and get response (blocking) */
int send_udp_and_recv(const char *msg, char *resp, int resp_size) {
    int sock;
    struct sockaddr_in srv;
    socklen_t slen = sizeof(srv);
#ifdef _WIN32
    sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sock == INVALID_SOCKET) return -1;
#else
    sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) return -1;
#endif
    memset(&srv,0,sizeof(srv));
    srv.sin_family = AF_INET;
    srv.sin_port = htons(server_port);
    inet_pton(AF_INET, server_addr_str, &srv.sin_addr);

    int sent = 
#ifdef _WIN32
    sendto(sock, msg, (int)strlen(msg), 0, (struct sockaddr*)&srv, slen);
#else
    sendto(sock, msg, strlen(msg), 0, (struct sockaddr*)&srv, slen);
#endif

    if (sent < 0) {
#ifdef _WIN32
        closesocket(sock);
#else
        close(sock);
#endif
        return -1;
    }

#ifdef _WIN32
    int n = recvfrom(sock, resp, resp_size-1, 0, (struct sockaddr*)&srv, &slen);
    if (n == SOCKET_ERROR) n = 0;
#else
    int n = recvfrom(sock, resp, resp_size-1, 0, (struct sockaddr*)&srv, &slen);
    if (n < 0) n = 0;
#endif
    resp[n] = '\0';
#ifdef _WIN32
    closesocket(sock);
#else
    close(sock);
#endif
    return n;
}

/* parse PEERS response and populate known_peers[] */
void populate_known_peers_from_list(const char *lst) {
    known_count = 0;
    const char *p = lst;
    char line[256];
    while (*p) {
        if (*p == '\n') { p++; continue; }
        int i = 0;
        while (*p && *p != '\n' && i < (int)sizeof(line)-1) line[i++] = *p++;
        line[i] = '\0';
        if (strlen(line) == 0) continue;
        char ip[INET_ADDRSTRLEN]; int port, seq;
        if (sscanf(line, "%15[^:]:%d#%d", ip, &port, &seq) == 3) {
            if (known_count < MAX_PEER_SLOTS) {
                known_peers[known_count].active = 1;
                strncpy(known_peers[known_count].ip, ip, INET_ADDRSTRLEN-1);
                known_peers[known_count].port = port;
                known_peers[known_count].seq = seq;
                known_count++;
            }
        }
    }
}

/* attempt TCP connection to remote peer; return socket or -1 */
int tcp_connect_to_peer(const char *ip, int port) {
    int s;
    struct sockaddr_in addr;
#ifdef _WIN32
    s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (s == INVALID_SOCKET) return -1;
#else
    s = socket(AF_INET, SOCK_STREAM, 0);
    if (s < 0) return -1;
#endif
    memset(&addr,0,sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    inet_pton(AF_INET, ip, &addr.sin_addr);

    if (connect(s, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
#ifdef _WIN32
        closesocket(s);
#else
        close(s);
#endif
        return -1;
    }
    set_nonblock(s);
    return s;
}

/* accept a new tcp neighbor (server side) */
void add_tcp_neighbor(int s, int remote_seq, int is_external, const char *ip, int port) {
    for (int i=0;i<MAX_TCP_SOCKS;i++){
        if (tcp_neighbors[i].sock == 0) {
            tcp_neighbors[i].sock = s;
            tcp_neighbors[i].seq = remote_seq;
            tcp_neighbors[i].is_external = is_external;
            strncpy(tcp_neighbors[i].ip, ip, INET_ADDRSTRLEN-1);
            tcp_neighbors[i].port = port;
            neighbor_count++;
            return;
        }
    }
}

/* remove neighbor by socket */
void remove_tcp_neighbor_by_sock(int s) {
    for (int i=0;i<MAX_TCP_SOCKS;i++){
        if (tcp_neighbors[i].sock == s) {
#ifdef _WIN32
            closesocket(tcp_neighbors[i].sock);
#else
            close(tcp_neighbors[i].sock);
#endif
            tcp_neighbors[i].sock = 0;
            tcp_neighbors[i].seq = 0;
            tcp_neighbors[i].is_external = 0;
            tcp_neighbors[i].ip[0] = '\0';
            tcp_neighbors[i].port = 0;
            neighbor_count--;
            return;
        }
    }
}

/* find neighbor index by seq */
int find_neighbor_index_by_seq(int seq) {
    for (int i=0;i<MAX_TCP_SOCKS;i++) if (tcp_neighbors[i].sock && tcp_neighbors[i].seq == seq) return i;
    return -1;
}

/* attempt to establish up to neigh_limit outgoing connections to peers with seq < my seq */
void try_establish_outgoing(int udp_sock) {
    if (seqnumber <= 0) return;
    int externals = 0;
    for (int i=0;i<MAX_TCP_SOCKS;i++) if (tcp_neighbors[i].sock && tcp_neighbors[i].is_external) externals++;
    for (int i=0;i<known_count && externals < neigh_limit;i++){
        RemotePeer *rp = &known_peers[i];
        if (!rp->active) continue;
        if (rp->seq >= seqnumber) continue;
        /* check if we already connected */
        int exists = 0;
        for (int j=0;j<MAX_TCP_SOCKS;j++) if (tcp_neighbors[j].sock && tcp_neighbors[j].seq == rp->seq) { exists = 1; break; }
        if (exists) continue;
        int s = tcp_connect_to_peer(rp->ip, rp->port);
        if (s < 0) continue;
        /* send LNK <myseq> */
        char buf[256];
        snprintf(buf, sizeof(buf), "LNK %d\n", seqnumber);
#ifdef _WIN32
        send(s, buf, (int)strlen(buf), 0);
#else
        write(s, buf, strlen(buf));
#endif
        add_tcp_neighbor(s, rp->seq, 1, rp->ip, rp->port);
        externals++;
        printf("Tentou LNK %s:%d (seq=%d)\n", rp->ip, rp->port, rp->seq);
    }
}

/* handle incoming LNK/FRC messages on socket s */
void handle_tcp_message(int s, char *line) {
    /* expected formats:
       LNK <seq>
       FRC <seq>
       CNF
       QRY <identifier> <hop>
       FND <identifier>
       NOTFND <identifier>
    */
    char cmd[16];
    if (sscanf(line, "%15s", cmd) != 1) return;

    if (strcmp(cmd, "LNK") == 0 || strcmp(cmd, "FRC") == 0) {
        int remote_seq = -1;
        if (sscanf(line, "%*s %d", &remote_seq) == 1) {
            /* find available internal slots */
            int internals = 0;
            for (int i=0;i<MAX_TCP_SOCKS;i++) if (tcp_neighbors[i].sock && !tcp_neighbors[i].is_external) internals++;
            if (internals < neigh_limit) {
                /* accept: send CNF */
#ifdef _WIN32
                send(s, "CNF\n", 4, 0);
#else
                write(s, "CNF\n", 4);
#endif
                /* mark neighbor's seq in our table */
                /* find slot for this socket */
                for (int i=0;i<MAX_TCP_SOCKS;i++){
                    if (tcp_neighbors[i].sock == s) {
                        tcp_neighbors[i].seq = remote_seq;
                        tcp_neighbors[i].is_external = 0; // they initiated, so they are internal to us
                        break;
                    }
                }
                printf("Aceitou LNK/FRC de seq %d (socket %d)\n", remote_seq, s);
            } else if (strcmp(cmd, "FRC") == 0) {
                /* FRC: find an internal neighbor with seq > remote_seq and replace it */
                int idx_replace = -1;
                for (int i=0;i<MAX_TCP_SOCKS;i++){
                    if (tcp_neighbors[i].sock && !tcp_neighbors[i].is_external && tcp_neighbors[i].seq > remote_seq) {
                        idx_replace = i; break;
                    }
                }
                if (idx_replace >= 0) {
                    /* close that connection and accept this one */
#ifdef _WIN32
                    closesocket(tcp_neighbors[idx_replace].sock);
#else
                    close(tcp_neighbors[idx_replace].sock);
#endif
                    tcp_neighbors[idx_replace].sock = s;
                    tcp_neighbors[idx_replace].seq = remote_seq;
                    tcp_neighbors[idx_replace].is_external = 0;
#ifdef _WIN32
                    send(s, "CNF\n", 4, 0);
#else
                    write(s, "CNF\n", 4);
#endif
                    printf("Troca de ligação: removeu seq antigo -> aceitou seq %d\n", remote_seq);
                } else {
                    /* cannot accept: simply close */
#ifdef _WIN32
                    closesocket(s);
#else
                    close(s);
#endif
                    remove_tcp_neighbor_by_sock(s);
                    printf("FRC rejeitado (sem substituivel)\n");
                }
            } else {
                /* LNK but no space: close */
#ifdef _WIN32
                closesocket(s);
#else
                close(s);
#endif
                remove_tcp_neighbor_by_sock(s);
                printf("LNK rejeitado (cheio)\n");
            }
        }
    } else if (strcmp(cmd, "CNF") == 0) {
        /* remote confirmed our connection */
        printf("Recebeu CNF em socket %d\n", s);
    } else if (strcmp(cmd, "QRY") == 0) {
        char identifier[128]; int hop;
        if (sscanf(line, "QRY %127s %d", identifier, &hop) == 2) {
            /* if we know it, send FND back; else if hop>1 forward to our neighbors */
            int we_have = 0;
            for (int i=0;i<identifiers_count;i++) if (strcmp(identifiers[i], identifier)==0) { we_have = 1; break; }
            char resp[256];
            if (we_have) {
                snprintf(resp, sizeof(resp), "FND %s\n", identifier);
#ifdef _WIN32
                send(s, resp, (int)strlen(resp), 0);
#else
                write(s, resp, strlen(resp));
#endif
            } else {
                if (hop > 1) {
                    /* forward to each neighbor except the one we received from */
                    for (int i=0;i<MAX_TCP_SOCKS;i++){
                        int nsock = tcp_neighbors[i].sock;
                        if (!nsock || nsock == s) continue;
                        char fwd[256];
                        snprintf(fwd, sizeof(fwd), "QRY %s %d\n", identifier, hop-1);
#ifdef _WIN32
                        send(nsock, fwd, (int)strlen(fwd), 0);
#else
                        write(nsock, fwd, strlen(fwd));
#endif
                    }
                }
                /* after forwarding (or not) send NOTFND back to the requester */
                snprintf(resp, sizeof(resp), "NOTFND %s\n", identifier);
#ifdef _WIN32
                send(s, resp, (int)strlen(resp), 0);
#else
                write(s, resp, strlen(resp));
#endif
            }
        }
    } else if (strcmp(cmd, "FND") == 0) {
        char identifier[128];
        if (sscanf(line, "FND %127s", identifier) == 1) {
            add_identifier(identifier);
            printf("Recebeu FND: %s -> agora sei deste identificador\n", identifier);
        }
    } else if (strcmp(cmd, "NOTFND") == 0) {
        char identifier[128];
        if (sscanf(line, "NOTFND %127s", identifier) == 1) {
            printf("Recebeu NOTFND %s\n", identifier);
        }
    } else {
        printf("Mensagem TCP desconhecida: %s\n", line);
    }
}

/* print neighbors */
void show_neighbors() {
    printf("Vizinho(s) TCP (%d):\n", neighbor_count);
    for (int i=0;i<MAX_TCP_SOCKS;i++){
        if (tcp_neighbors[i].sock) {
            printf(" - seq=%d %s:%d %s\n", tcp_neighbors[i].seq, tcp_neighbors[i].ip, tcp_neighbors[i].port,
                tcp_neighbors[i].is_external ? "(externo)" : "(interno)");
        }
    }
}

/* release seqnumber: close the internal neighbor with that seq if exists */
void release_seqnumber(int seq) {
    int idx = find_neighbor_index_by_seq(seq);
    if (idx >= 0) {
#ifdef _WIN32
        closesocket(tcp_neighbors[idx].sock);
#else
        close(tcp_neighbors[idx].sock);
#endif
        printf("Liberou ligação com seq %d\n", seq);
        tcp_neighbors[idx].sock = 0;
        tcp_neighbors[idx].seq = 0;
        tcp_neighbors[idx].is_external = 0;
        neighbor_count--;
    } else {
        printf("Seq %d não encontrado entre vizinhos\n", seq);
    }
}

int main(int argc, char *argv[]) {
#ifdef _WIN32
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2,2), &wsa) != 0) {
        fprintf(stderr, "WSAStartup falhou\n");
        return 1;
    }
#endif

    /* parse args (simple) */
    for (int i=1;i<argc;i++){
        if (strcmp(argv[i], "-s")==0 && i+1<argc) { strncpy(server_addr_str, argv[++i], sizeof(server_addr_str)-1); }
        else if (strcmp(argv[i], "-p")==0 && i+1<argc) { server_port = atoi(argv[++i]); }
        else if (strcmp(argv[i], "-l")==0 && i+1<argc) { lnkport = atoi(argv[++i]); }
        else if (strcmp(argv[i], "-n")==0 && i+1<argc) { neigh_limit = atoi(argv[++i]); if (neigh_limit<1) neigh_limit=1; }
        else if (strcmp(argv[i], "-h")==0 && i+1<argc) { hopcount_limit = atoi(argv[++i]); if (hopcount_limit<1) hopcount_limit=1; }
    }

    printf("Servidor peers %s:%d  | lnkport=%d neigh=%d hop=%d\n", server_addr_str, server_port, lnkport, neigh_limit, hopcount_limit);

    /* UDP socket to server for REG/PEERS/UNR */
    int udp_sock;
    struct sockaddr_in udp_local;
#ifdef _WIN32
    udp_sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (udp_sock == INVALID_SOCKET) { fprintf(stderr,"udp socket erro\n"); return 1; }
#else
    udp_sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (udp_sock < 0) { perror("udp socket"); return 1; }
#endif
    memset(&udp_local,0,sizeof(udp_local));
    udp_local.sin_family = AF_INET;
    udp_local.sin_port = htons(0); /* ephemeral */
    udp_local.sin_addr.s_addr = INADDR_ANY;

    if (
#ifdef _WIN32
        bind(udp_sock, (struct sockaddr*)&udp_local, sizeof(udp_local)) == SOCKET_ERROR
#else
        bind(udp_sock, (struct sockaddr*)&udp_local, sizeof(udp_local)) < 0
#endif
    ) {
#ifdef _WIN32
        fprintf(stderr,"bind udp falhou\n");
#else
        perror("bind udp");
#endif
        return 1;
    }

    /* TCP listening socket for peer connections */
    int listen_sock;
    struct sockaddr_in listen_addr;
#ifdef _WIN32
    listen_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listen_sock == INVALID_SOCKET) { fprintf(stderr,"tcp listen socket erro\n"); return 1; }
#else
    listen_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_sock < 0) { perror("tcp listen socket"); return 1; }
#endif
    memset(&listen_addr,0,sizeof(listen_addr));
    listen_addr.sin_family = AF_INET;
    listen_addr.sin_port = htons(lnkport);
    listen_addr.sin_addr.s_addr = INADDR_ANY;

    if (
#ifdef _WIN32
        bind(listen_sock, (struct sockaddr*)&listen_addr, sizeof(listen_addr)) == SOCKET_ERROR
#else
        bind(listen_sock, (struct sockaddr*)&listen_addr, sizeof(listen_addr)) < 0
#endif
    ) {
#ifdef _WIN32
        fprintf(stderr,"bind listen falhou\n");
#else
        perror("bind listen");
#endif
        return 1;
    }
    if (
#ifdef _WIN32
        listen(listen_sock, 10) == SOCKET_ERROR
#else
        listen(listen_sock, 10) < 0
#endif
    ) {
#ifdef _WIN32
        fprintf(stderr,"listen falhou\n");
#else
        perror("listen");
#endif
        return 1;
    }
    set_nonblock(listen_sock);

    /* initialize neighbor table */
    for (int i=0;i<MAX_TCP_SOCKS;i++) tcp_neighbors[i].sock = 0;

    /* main loop: select on stdin, udp_sock (for server replies if we bind to receive), listen_sock, and tcp neighbors */
    fd_set readfds;
    int maxfd = listen_sock;
    char linebuf[BUFSIZE];

    while (1) {
        FD_ZERO(&readfds);
        FD_SET(0, &readfds); /* stdin */
        FD_SET(listen_sock, &readfds);
#ifdef _WIN32
        FD_SET(udp_sock, &readfds);
#else
        FD_SET(udp_sock, &readfds);
#endif
        int local_max = listen_sock;
        if (udp_sock > local_max) local_max = udp_sock;

        for (int i=0;i<MAX_TCP_SOCKS;i++){
            if (tcp_neighbors[i].sock) {
                FD_SET(tcp_neighbors[i].sock, &readfds);
                if (tcp_neighbors[i].sock > local_max) local_max = tcp_neighbors[i].sock;
            }
        }

#ifdef _WIN32
        int sel = select(local_max+1, &readfds, NULL, NULL, NULL);
#else
        int sel = select(local_max+1, &readfds, NULL, NULL, NULL);
#endif
        if (sel < 0) {
#ifdef _WIN32
            fprintf(stderr,"select erro\n");
#else
            perror("select");
#endif
            break;
        }

        /* stdin */
        if (FD_ISSET(0, &readfds)) {
            if (!fgets(linebuf, sizeof(linebuf), stdin)) break;
            /* remove trailing newline */
            char *pnl = strchr(linebuf, '\n'); if (pnl) *pnl = '\0';
            if (strncmp(linebuf, "join", 4) == 0) {
                /* send REG lnkport */
                char msg[128], resp[BUFSIZE];
                snprintf(msg, sizeof(msg), "REG %d", lnkport);
                if (send_udp_and_recv(msg, resp, sizeof(resp)) > 0) {
                    if (sscanf(resp, "SQN %d", &seqnumber) == 1) {
                        printf("REG OK -> seq=%d\n", seqnumber);
                        /* request PEERS */
                        send_udp_and_recv("PEERS", resp, sizeof(resp));
                        if (strncmp(resp, "LST", 3) == 0) {
                            /* skip "LST\n" */
                            char *p = strstr(resp, "\n");
                            if (p) p++;
                            populate_known_peers_from_list(p ? p : "");
                            printf("Recebeu lista de %d peers\n", known_count);
                            /* try to connect to some peers */
                            try_establish_outgoing(udp_sock);
                        } else {
                            printf("PEERS falhou / resposta: %s\n", resp);
                        }
                    } else {
                        printf("REG NOK / resposta: %s\n", resp);
                    }
                } else {
                    printf("Erro a contatar servidor de peers\n");
                }
            }
            else if (strncmp(linebuf, "leave", 5) == 0) {
                if (seqnumber > 0) {
                    char msg[64], resp[BUFSIZE];
                    snprintf(msg, sizeof(msg), "UNR %d", seqnumber);
                    send_udp_and_recv(msg, resp, sizeof(resp));
                    printf("UNR resposta: %s\n", resp);
                    /* close all tcp neighbors */
                    for (int i=0;i<MAX_TCP_SOCKS;i++) if (tcp_neighbors[i].sock) remove_tcp_neighbor_by_sock(tcp_neighbors[i].sock);
                    seqnumber = -1;
                } else {
                    printf("Não estás registado.\n");
                }
            }
            else if (strncmp(linebuf, "show neighbors", 14) == 0) {
                show_neighbors();
            }
            else if (strncmp(linebuf, "post ", 5) == 0) {
                char id[128];
                if (sscanf(linebuf+5, "%127s", id) == 1) {
                    add_identifier(id);
                    printf("Post: %s\n", id);
                }
            }
            else if (strncmp(linebuf, "unpost ", 7) == 0) {
                char id[128];
                if (sscanf(linebuf+7, "%127s", id) == 1) {
                    remove_identifier(id);
                    printf("Unpost: %s\n", id);
                }
            }
            else if (strncmp(linebuf, "list identifiers", 16) == 0) {
                printf("Identifiers (%d):\n", identifiers_count);
                for (int i=0;i<identifiers_count;i++) printf(" - %s\n", identifiers[i]);
            }
            else if (strncmp(linebuf, "search ", 7) == 0) {
                char id[128];
                if (sscanf(linebuf+7, "%127s", id) == 1) {
                    /* if we already know, done */
                    int has = 0; for (int i=0;i<identifiers_count;i++) if (strcmp(identifiers[i], id)==0) { has = 1; break; }
                    if (has) { printf("Já conheces %s\n", id); continue; }
                    /* query each neighbor in turn */
                    for (int i=0;i<MAX_TCP_SOCKS;i++){
                        int nsock = tcp_neighbors[i].sock;
                        if (!nsock) continue;
                        char q[256];
                        snprintf(q, sizeof(q), "QRY %s %d\n", id, hopcount_limit);
#ifdef _WIN32
                        send(nsock, q, (int)strlen(q), 0);
#else
                        write(nsock, q, strlen(q));
#endif
                    }
                    printf("QRY enviada para vizinhos (hop=%d). Aguarda respostas.\n", hopcount_limit);
                }
            }
            else if (strncmp(linebuf, "release ", 8) == 0) {
                int seq;
                if (sscanf(linebuf+8, "%d", &seq) == 1) release_seqnumber(seq);
            }
            else if (strncmp(linebuf, "exit", 4) == 0) {
                /* perform leave then exit */
                if (seqnumber > 0) {
                    char msg[64], resp[BUFSIZE];
                    snprintf(msg, sizeof(msg), "UNR %d", seqnumber);
                    send_udp_and_recv(msg, resp, sizeof(resp));
                }
                printf("A sair...\n");
                break;
            }
            else {
                printf("Comando desconhecido: %s\n", linebuf);
            }
        }

        /* UDP socket ready? (rare: server responses when using same socket) */
        if (FD_ISSET(udp_sock, &readfds)) {
            char buf[BUFSIZE];
            struct sockaddr_in from;
            socklen_t fromlen = sizeof(from);
#ifdef _WIN32
            int n = recvfrom(udp_sock, buf, BUFSIZE-1, 0, (struct sockaddr*)&from, &fromlen);
#else
            int n = recvfrom(udp_sock, buf, BUFSIZE-1, 0, (struct sockaddr*)&from, &fromlen);
#endif
            if (n > 0) {
                buf[n] = '\0';
                printf("UDP (servidor) -> %s\n", buf);
            }
        }

        /* new tcp connection? */
        if (FD_ISSET(listen_sock, &readfds)) {
            struct sockaddr_in raddr;
            socklen_t rlen = sizeof(raddr);
#ifdef _WIN32
            SOCKET ns = accept(listen_sock, (struct sockaddr*)&raddr, &rlen);
            if (ns != INVALID_SOCKET) {
                char rip[INET_ADDRSTRLEN];
                inet_ntop(AF_INET, &raddr.sin_addr, rip, sizeof(rip));
                set_nonblock(ns);
                /* create entry with seq unknown yet (0) */
                add_tcp_neighbor((int)ns, 0, 0, rip, ntohs(raddr.sin_port));
                printf("Ligação TCP recebida de %s:%d\n", rip, ntohs(raddr.sin_port));
            }
#else
            int ns = accept(listen_sock, (struct sockaddr*)&raddr, &rlen);
            if (ns >= 0) {
                char rip[INET_ADDRSTRLEN];
                inet_ntop(AF_INET, &raddr.sin_addr, rip, sizeof(rip));
                set_nonblock(ns);
                add_tcp_neighbor(ns, 0, 0, rip, ntohs(raddr.sin_port));
                printf("Ligacao TCP recebida de %s:%d\n", rip, ntohs(raddr.sin_port));
            }
#endif
        }

        /* handle tcp neighbors data */
        for (int i=0;i<MAX_TCP_SOCKS;i++){
            int s = tcp_neighbors[i].sock;
            if (!s) continue;
            if (FD_ISSET(s, &readfds)) {
                char buf[BUFSIZE];
#ifdef _WIN32
                int n = recv(s, buf, BUFSIZE-1, 0);
                if (n == SOCKET_ERROR || n == 0) { remove_tcp_neighbor_by_sock(s); continue; }
#else
                int n = read(s, buf, BUFSIZE-1);
                if (n <= 0) { remove_tcp_neighbor_by_sock(s); continue; }
#endif
                buf[n] = '\0';
                /* may contain multiple lines; process line by line */
                char *p = buf;
                while (*p) {
                    char line[512];
                    int li = 0;
                    while (*p && *p != '\n' && li < (int)sizeof(line)-1) line[li++] = *p++;
                    line[li] = '\0';
                    if (*p == '\n') p++;
                    if (li > 0) handle_tcp_message(s, line);
                }
            }
        }

    } /* end loop */

#ifdef _WIN32
    closesocket(listen_sock);
    closesocket(udp_sock);
    WSACleanup();
#else
    close(listen_sock);
    close(udp_sock);
#endif

    return 0;
}
