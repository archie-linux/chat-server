#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define PORT 8080
#define BUFFER_SIZE 1024

int create_socket() {
    int server_fd;
    int opt = 1;

    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }

    return server_fd;
}

void bind_socket(int server_fd) {
    struct sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }
}

int accept_connection(int server_fd) {
    struct sockaddr_in address;
    int addrlen = sizeof(address);
    int client_socket;

    printf("Waiting for new connection...\n");

    if ((client_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen)) < 0) {
        perror("accept");
        exit(EXIT_FAILURE);
    }

    printf("Connection Recevied: %d\n", client_socket);

    return client_socket;
}

void handle_connection(int client_socket) {
    int valread;
    char buffer[BUFFER_SIZE] = {0};

    while (1) {
        valread = recv(client_socket, buffer, BUFFER_SIZE, 0);
        if (valread == 0) {
            printf("Client disconnected\n");
            break;
        } else if (valread == -1) {
            perror("recv");
            exit(EXIT_FAILURE);
        } else {
            printf("Received: %s\n", buffer);
            send(client_socket, buffer, sizeof(buffer), 0);
            memset(buffer, 0, BUFFER_SIZE);
        }
    }
}


int main() {
    int server_fd, client_socket;

    server_fd = create_socket();
    bind_socket(server_fd);

    if (listen(server_fd, 3) < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    while (1) {
        client_socket = accept_connection(server_fd);

        handle_connection(client_socket);

        printf("Closing client socket!");
        close(client_socket);
    }

    return 0;
}