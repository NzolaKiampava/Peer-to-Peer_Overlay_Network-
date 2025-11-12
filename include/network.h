#ifndef NETWORK_H
#define NETWORK_H

#include "common.h"

/* Funções UDP */
int create_udp_socket();
int udp_send(int sockfd, const char *ip, int port, const char *message);
int udp_receive(int sockfd, char *buffer, int buffer_size, 
                struct sockaddr_in *from_addr);

/* Funções TCP Cliente */
int tcp_connect(const char *ip, int port);
int tcp_send(int sockfd, const char *message);
int tcp_receive(int sockfd, char *buffer, int buffer_size);

/* Funções TCP Servidor */
int create_tcp_server(int port);
int tcp_accept(int server_fd, char *client_ip);

/* Funções auxiliares */
void close_socket(int sockfd);
int set_socket_nonblocking(int sockfd);

#endif /* NETWORK_H */
