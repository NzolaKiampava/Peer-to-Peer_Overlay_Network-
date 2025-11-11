#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define MAX_PEERS 100

typedef struct {
    int active;
    char ip[INET_ADDRSTRLEN];
    int port;
    int seq;
} Peer;

Peer peers[MAX_PEERS];
int next_seq = 1; // contador de sequência global

int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("Uso: %s <porta>\n", argv[0]);
        exit(1);
    }

    int port = atoi(argv[1]);
    int sockfd;
    struct sockaddr_in server_addr, client_addr;
    char buffer[1024];
    socklen_t addr_len = sizeof(client_addr);

    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if(sockfd < 0){
        perror("socket error");
        exit(1);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    if(bind(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0){
        perror("bind error");
        exit(1);
    }

    printf("Servidor de peers ativo na porta %d\n", port);

    while(1) {
        int n = recvfrom(sockfd, buffer, 1023, 0, (struct sockaddr*)&client_addr, &addr_len);
        if(n < 0){
            perror("recvfrom error");
            continue;
        }
        buffer[n] = '\0';
        printf("Recebido: %s\n", buffer);

        char cmd[16];
        sscanf(buffer, "%s", cmd);

        if(strcmp(cmd, "REG") == 0){
            int peer_port;
            sscanf(buffer, "REG %d", &peer_port);

            for(int i=0; i<MAX_PEERS; i++){
                if(!peers[i].active){
                    peers[i].active = 1;
                    strcpy(peers[i].ip, inet_ntoa(client_addr.sin_addr));
                    peers[i].port = peer_port;
                    peers[i].seq = next_seq++;
                    
                    // envia SQN <seq>
                    sprintf(buffer, "SQN %d", peers[i].seq);
                    sendto(sockfd, buffer, strlen(buffer), 0, (struct sockaddr*)&client_addr, addr_len);
                    break;
                }
            }
        }
        else if(strcmp(cmd, "UNR") == 0){
            int seq;
            sscanf(buffer, "UNR %d", &seq);
            int found = 0;

            for(int i=0; i<MAX_PEERS; i++){
                if(peers[i].active && peers[i].seq == seq){
                    peers[i].active = 0;
                    found = 1;
                    break;
                }
            }

            if(found){
                sendto(sockfd, "OK", 2, 0, (struct sockaddr*)&client_addr, addr_len);
            } else {
                sendto(sockfd, "NOK", 3, 0, (struct sockaddr*)&client_addr, addr_len);
            }
        }
        else if(strcmp(cmd, "PEERS") == 0){
            sendto(sockfd, "LST\n", 4, 0, (struct sockaddr*)&client_addr, addr_len);

            for(int i=0; i<MAX_PEERS; i++){
                if(peers[i].active){
                    sprintf(buffer, "%s:%d#%d\n", peers[i].ip, peers[i].port, peers[i].seq);
                    sendto(sockfd, buffer, strlen(buffer), 0, (struct sockaddr*)&client_addr, addr_len);
                }
            }

            sendto(sockfd, "\n", 1, 0, (struct sockaddr*)&client_addr, addr_len);
        }
    }

    close(sockfd);
    return 0;
}



