#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <errno.h>

#define PORT 8080
#define MAX_CLIENTS 5
#define BUFFER_SIZE 1024

int server_fd, client_sockets[MAX_CLIENTS];

void initialize_server() {
    struct sockaddr_in address;
    int addrlen = sizeof(address);
    int opt = 1;

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, 3) < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    printf("Server listening on port %d\n", PORT);

    // Initialize client socket array
    for (int i = 0; i < MAX_CLIENTS; ++i) {
        client_sockets[i] = 0;
    }
}

int accept_new_connection(fd_set *read_fds) {
    struct sockaddr_in address;
    int addrlen = sizeof(address);
    int client_sd;

    if ((client_sd = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen)) < 0) {
        perror("accept");
        exit(EXIT_FAILURE);
    }

    printf("Client connected - IP address: [%s] Port: [%d]\n", inet_ntoa(address.sin_addr), ntohs(address.sin_port));

    // Add the new socket to the client's socket array
    for (int i = 0; i < MAX_CLIENTS; ++i) {
        if (client_sockets[i] == 0) {
            client_sockets[i] = client_sd;
            FD_SET(client_sd, read_fds);
            break;
        }
    }
    return client_sd;
}

void handle_client_activity(fd_set *read_fds, fd_set *temp_fds) {
    char buffer[BUFFER_SIZE];

    for (int i = 0; i < MAX_CLIENTS; ++i) {
        int sd = client_sockets[i];
        if (FD_ISSET(sd, temp_fds)) {
            // Clear the buffer
            memset(buffer, 0, BUFFER_SIZE);
            int valread = recv(sd, buffer, BUFFER_SIZE, 0);
            if (valread == 0) {
                // Client disconnected
                printf("Client disconnected\n");
                close(sd);
                client_sockets[i] = 0;
                FD_CLR(sd, read_fds); 
            } else if (valread < 0) {
                // Error reading from client
                if (errno != EAGAIN && errno != EWOULDBLOCK) {
                    perror("recv");
                }
            } else {
                printf("Client: %s\n", buffer);

                // Read input from Server's console
                printf("Server: ");
                fgets(buffer, BUFFER_SIZE, stdin);

                // Send response to client
                send(sd, buffer, strlen(buffer), 0);
            }
        }
    }
}


int main() {
    initialize_server();

    fd_set read_fds, temp_fds;
    FD_ZERO(&read_fds);
    FD_SET(server_fd, &read_fds);
    int max_sd;
    max_sd = server_fd;

    while (1) {
        temp_fds = read_fds;
        int activity = select(max_sd + 1, &temp_fds, NULL, NULL, NULL);
        if (activity < 0) {
            perror("select:");
            exit(EXIT_FAILURE);
        }

        // Check if there's a new connection
        if (FD_ISSET(server_fd, &temp_fds)) {
            int client_sd = accept_new_connection(&read_fds);
            if (client_sd > max_sd) {
                max_sd = client_sd;
            }
        }

        handle_client_activity(&read_fds, &temp_fds);
    }

    return 0;
}