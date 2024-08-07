#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define MAX_LEN 30
#define RESPONSE 11

int connectToServer(const char *server_ip, int port){
    int cfd;
    struct sockaddr_in server_address;

    memset(&server_address, 0, sizeof(struct sockaddr_in));
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(port);
    if(inet_pton(AF_INET, server_ip, &server_address.sin_addr) == 0){
        perror("That's not an IPv4 address.\n"); 
        return 1;
    }

    cfd = socket(AF_INET, SOCK_STREAM, 0);
    if(cfd == -1){
        perror("Cannot connect to server.\n"); 
        return 1;
    }
    
    
    if (connect(cfd, (struct sockaddr *)&server_address, sizeof(server_address)) == -1) {
        perror("connect.\n"); 
        return 1;
    }

    return cfd;

}


int main(int argc, char *argv[])
{
    if (argc != 3) {
        fprintf(stderr, "Need server IPv4 address.\n");
        return 1;
    }

    const char *server_ip = argv[1];
    int port = atoi(argv[2]);
    int cfd = connectToServer(server_ip, port);

    if (cfd == -1) {
        return 1;
    }

    printf("Ready\n");

    char name[MAX_LEN+1];
    while(fgets(name, MAX_LEN+1, stdin)){
        size_t name_len = strlen(name);

        if(name[0] == '\n' && name_len == 1){
            close(cfd);
            return 0;
        }
        
        if(name[name_len-1] != '\n'){
            close(cfd);
            return 0;
        }

        name[name_len-1]='\0';

        if(send(cfd, name, strlen(name), 0) < 0){
            perror("Failed to send data\n");
            close(cfd); 
            return 1; 
        }
        
        char response[RESPONSE+1];
        ssize_t read_b = recv(cfd, response, RESPONSE, 0);
        if(read_b < 0 || read_b > 11 || response[read_b-1] != '\n'){
            perror("Failed to receive data\n");
            close(cfd); 
            return 1;
        }else if(read_b == 0){
            perror("unexpected disconnection\n");
            close(cfd); 
            return 1;
        }
        response[read_b-1] = '\0';
        printf("%s\n", response);
    }
    close(cfd);
    return 0;
}