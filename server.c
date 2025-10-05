#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <errno.h>

#define PORT 8080
#define MAX_CLIENTS 5
#define BUFFER_SIZE 1024
#define USERNAME_MAX_LENGTH 32


struct Client {
    int socket;
    char username[USERNAME_MAX_LENGTH];
};


int server_fd, client_sockets[MAX_CLIENTS];
struct Client registered_clients[MAX_CLIENTS];
int num_registered_clients = 0;


int handle_registration(int client_socket, const char *username) {
    // Check if the username is already in use
    for (int i = 0; i < MAX_CLIENTS; ++i) {
        if (strcmp(registered_clients[i].username, username) == 0) {
            // User is already registered, reject the registration
            char rejection_msg[] = "Username is already taken. Please choose a different one.\n";
            send(client_socket, rejection_msg, strlen(rejection_msg), 0);
            close(client_socket);
            return 0;
        }
    }

    // Add the client to the list of registered clients
    struct Client new_client;
    new_client.socket = client_socket;
    strncpy(new_client.username, username, USERNAME_MAX_LENGTH);
    registered_clients[num_registered_clients++] = new_client;

    // Notify the client of successful registration
    char success_msg[] = "Registration successful. Welcome to the chat server!\n";
    char success_msg_server[USERNAME_MAX_LENGTH + 20];

    sprintf(success_msg_server, "Client registered: %s\n", new_client.username);
    printf("%s", success_msg_server);

    send(client_socket, success_msg, strlen(success_msg), 0);
    return 1;
}


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
    char buffer[BUFFER_SIZE];
    int client_sd;

    if ((client_sd = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen)) < 0) {
        perror("accept");
        exit(EXIT_FAILURE);
    }

    printf("Client connected - IP address: [%s] Port: [%d]\n", inet_ntoa(address.sin_addr), ntohs(address.sin_port));

    // Handle client registration
    send(client_sd, "Please enter your username: ", strlen("Please enter your username: "), 0);

    int valread = recv(client_sd, buffer, BUFFER_SIZE, 0);

    if (valread > 0) {
        // When using netcat client, the '\n' character gets added on enter
        // in the username when sending username over to the server
        for (int i = 0; i < valread; i++) {
            if (buffer[i] == '\n') {
                buffer[i] = '\0'; // Replace newline with null terminator
                break; // Stop after the first newline is found
            }
        }

        if (handle_registration(client_sd, buffer)) {
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
    } else {
        printf("Error reading username from client\n");
        close(client_sd);
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
                FD_CLR(sd, read_fds);
                printf("Deregistered client: %s\n", registered_clients[i].username);

                // Remove the client from the registered clients list
                for (int j = i; j < num_registered_clients - 1; ++j) {
                    registered_clients[j] = registered_clients[j+1];
                }

                num_registered_clients--;
                client_sockets[i] = 0;
            } else if (valread < 0) {
                // Error reading from client
                perror("recv");
            } else {
                char recipient[USERNAME_MAX_LENGTH];
                char message[BUFFER_SIZE - USERNAME_MAX_LENGTH - 1]; // Leave space for null terminator
                
                sscanf(buffer, "%s %[^\n]", recipient, message);

                // Find the recipient in the registered client's list
                int recipient_socket = -1;
                for (int j = 0; j < num_registered_clients; j++) {
                    if (strcmp(registered_clients[j].username, recipient) == 0) {
                        recipient_socket = registered_clients[j].socket;
                        break;
                    }
                }

                // If recipient found, send the message
                if (recipient_socket != -1) {
                    char composed_message[BUFFER_SIZE];
                    sprintf(composed_message, "%s: %s\n", registered_clients[i].username, message);

                    // Send the message to the recipient
                    send(recipient_socket, composed_message, strlen(composed_message), 0);
                } else {
                    // Recipient not found, notify sender
                    char not_found_msg[] = "Recipient not found\n";
                    send(sd, not_found_msg, strlen(not_found_msg), 0);
                }
                
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