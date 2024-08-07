#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <signal.h>
#include <sys/wait.h>
#include "record.h"

#define RESPONSE 11

int get_sunspots(FILE *f, const char *name, unsigned short *psunspots) {  
    record  rec;  
    size_t input_name_len = strlen(name);  
  
    fseek(f,0,SEEK_SET);  
  
    while(fread(&rec, sizeof(record), 1, f)==1){  
        if(strncmp(rec.name,name,rec.name_len)==0){  
            *psunspots = rec.sunspots;  
            return 1;  
        }  
    }  
    return 0;  
}

void term_zombies(int signum) {
    (void)signum; 
    while (waitpid(-1, NULL, WNOHANG) > 0);
}


void handle_client(int client_socket, const char *customer_file) {
    char buffer[NAME_LEN_MAX+1];
    ssize_t n;
    unsigned short sunspots;
    FILE *f = fopen(customer_file, "rb");
    if (f == NULL) {
        strcpy(buffer, "Error opening customer file\n");
    }
    while ((n = recv(client_socket, buffer, NAME_LEN_MAX+1, 0)) > 0) {
        buffer[n] = '\0'; 

        if (buffer[n - 1] == '\n') {
            buffer[n - 1] = '\0';  
        }
        if (get_sunspots(f, buffer, &sunspots)) {
            char response[RESPONSE];
            snprintf(response, RESPONSE, "%hu\n", sunspots);
            send(client_socket, response, strlen(response), 0);
        } else {
            send(client_socket, "none\n", 5, 0);
        }
    }
    fclose(f);
    close(client_socket);
    exit(0);
}


int main(int argc, char *argv[]) {
    int port = atoi(argv[1]);
    char *file = argv[2];

    if (!file) {
        perror("Failed to open customer file");
        return 1;
    }

    int server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == -1) {
        perror("socket");
        return 1;
    }

    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(struct sockaddr_in));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(server_socket, (struct sockaddr*)&server_addr, sizeof(struct sockaddr)) == -1) {
        perror("bind");
        close(server_socket);
        return 1;
    }

    if (listen(server_socket, 10) == -1) {
        perror("listen");
        close(server_socket);
        return 1;
    }

    struct sigaction sa;
    sa.sa_handler = term_zombies;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    if (sigaction(SIGCHLD, &sa, NULL) == -1) {
        perror("sigaction");
        close(server_socket);
        exit(EXIT_FAILURE);
    }

    while(1){
        struct sockaddr_in client_addr;
        socklen_t sin_size = sizeof(client_addr);
        int client_socket = accept(server_socket, (struct sockaddr*)&client_addr, &sin_size);

        if (client_socket == -1) {
            perror("accept");
            continue;
        }

        pid_t pid = fork();
        if(pid == 0) {
            close(server_socket);
            handle_client(client_socket, file);
        }
        close(client_socket); 
    }
    close(server_socket);
    return 0;
}