#include "../include/network.h"
#include <fcntl.h>

/* Cria socket UDP */
int create_udp_socket() {
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("socket");
        return -1;
    }
    return sockfd;
}

/* Envia mensagem UDP */
int udp_send(int sockfd, const char *ip, int port, const char *message) {
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    
    if (inet_pton(AF_INET, ip, &server_addr.sin_addr) <= 0) {
        perror("inet_pton");
        return -1;
    }
    
    int sent = sendto(sockfd, message, strlen(message), 0,
                     (struct sockaddr *)&server_addr, sizeof(server_addr));
    if (sent < 0) {
        perror("sendto");
        return -1;
    }
    
    return sent;
}

/* Recebe mensagem UDP */
int udp_receive(int sockfd, char *buffer, int buffer_size, 
                struct sockaddr_in *from_addr) {
    socklen_t addr_len = sizeof(*from_addr);
    int received = recvfrom(sockfd, buffer, buffer_size - 1, 0,
                           (struct sockaddr *)from_addr, &addr_len);
    if (received < 0) {
        perror("recvfrom");
        return -1;
    }
    
    buffer[received] = '\0';
    return received;
}

/* Conecta via TCP */
int tcp_connect(const char *ip, int port) {
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("socket");
        return -1;
    }
    
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    
    if (inet_pton(AF_INET, ip, &server_addr.sin_addr) <= 0) {
        perror("inet_pton");
        close(sockfd);
        return -1;
    }
    
    if (connect(sockfd, (struct sockaddr *)&server_addr, 
                sizeof(server_addr)) < 0) {
        perror("connect");
        close(sockfd);
        return -1;
    }
    
    return sockfd;
}

/* Envia mensagem TCP */
int tcp_send(int sockfd, const char *message) {
    int len = strlen(message);
    int sent = write(sockfd, message, len);
    if (sent < 0) {
        perror("write");
        return -1;
    }
    return sent;
}

/* Recebe mensagem TCP */
int tcp_receive(int sockfd, char *buffer, int buffer_size) {
    int received = read(sockfd, buffer, buffer_size - 1);
    if (received < 0) {
        perror("read");
        return -1;
    }
    buffer[received] = '\0';
    return received;
}

/* Cria servidor TCP */
int create_tcp_server(int port) {
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("socket");
        return -1;
    }
    
    int opt = 1;
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("setsockopt");
        close(sockfd);
        return -1;
    }
    
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);
    
    if (bind(sockfd, (struct sockaddr *)&server_addr, 
             sizeof(server_addr)) < 0) {
        perror("bind");
        close(sockfd);
        return -1;
    }
    
    if (listen(sockfd, 10) < 0) {
        perror("listen");
        close(sockfd);
        return -1;
    }
    
    return sockfd;
}

/* Aceita conexão TCP */
int tcp_accept(int server_fd, char *client_ip) {
    struct sockaddr_in client_addr;
    socklen_t addr_len = sizeof(client_addr);
    
    int client_fd = accept(server_fd, (struct sockaddr *)&client_addr, 
                          &addr_len);
    if (client_fd < 0) {
        perror("accept");
        return -1;
    }
    
    if (client_ip) {
        inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, 16);
    }
    
    return client_fd;
}

/* Fecha socket */
void close_socket(int sockfd) {
    if (sockfd >= 0) {
        close(sockfd);
    }
}

/* Define socket como não-bloqueante */
int set_socket_nonblocking(int sockfd) {
    int flags = fcntl(sockfd, F_GETFL, 0);
    if (flags < 0) {
        perror("fcntl F_GETFL");
        return -1;
    }
    
    if (fcntl(sockfd, F_SETFL, flags | O_NONBLOCK) < 0) {
        perror("fcntl F_SETFL");
        return -1;
    }
    
    return 0;
}
