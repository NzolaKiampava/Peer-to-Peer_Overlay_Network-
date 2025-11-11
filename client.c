#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#define MAX_BUFFER 1024

int main(int argc, char **argv) {
    if(argc != 3){
        printf("Usage: %s <server-ip> <port>\n", argv[0]);
        return 1;
    }

    char *server_ip = argv[1];
    int server_port = atoi(argv[2]);
    int sockfd;
    struct sockaddr_in server_addr;
    socklen_t addr_size;

    // Criar socket UDP
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if(sockfd < 0){
        perror("[-]Socket error");
        exit(1);
    }

    // Configurar endereço do servidor
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(server_port);
    server_addr.sin_addr.s_addr = inet_addr(server_ip);
    addr_size = sizeof(server_addr);

    char buffer[MAX_BUFFER];
    char command[16];

    printf("Cliente conectado a %s:%d\n\n", server_ip, server_port);
    printf("Comandos disponíveis:\n  REG <porta_tcp>\n  UNR <seqnumber>\n  PEERS\n  exit\n");

    while(1){
        printf("> ");
        fflush(stdout);
        if(!fgets(buffer, MAX_BUFFER, stdin)) continue;
        buffer[strcspn(buffer, "\n")] = '\0'; // remove \n

        if(sscanf(buffer, "%s", command) != 1) continue;

        if(strcmp(command, "exit") == 0){
            break;
        }

        // Envia comando para o servidor
        sendto(sockfd, buffer, strlen(buffer), 0, (struct sockaddr*)&server_addr, addr_size);

        if(strcmp(command, "PEERS") == 0){
            // Recebe LST
            int n = recvfrom(sockfd, buffer, MAX_BUFFER-1, 0, (struct sockaddr*)&server_addr, &addr_size);
            if(n < 0) { perror("recvfrom"); continue; }
            buffer[n] = '\0';
            printf("%s", buffer);

            // Recebe lista de peers linha a linha
            while(1){
                n = recvfrom(sockfd, buffer, MAX_BUFFER-1, 0, (struct sockaddr*)&server_addr, &addr_size);
                if(n < 0) { perror("recvfrom"); break; }
                buffer[n] = '\0';

                // remove \n do final
                if(n > 0 && buffer[n-1] == '\n') buffer[n-1] = '\0';

                if(strlen(buffer) == 0) break; // linha vazia indica fim
                printf("%s\n", buffer);
            }
        } else {
            // Recebe resposta simples (REG, UNR)
            int n = recvfrom(sockfd, buffer, MAX_BUFFER-1, 0, (struct sockaddr*)&server_addr, &addr_size);
            if(n < 0) { perror("recvfrom"); continue; }
            buffer[n] = '\0';
            printf("[+] Resposta: %s\n", buffer);
        }
    }

    close(sockfd);
    printf("Cliente fechado.\n");
    return 0;
}



