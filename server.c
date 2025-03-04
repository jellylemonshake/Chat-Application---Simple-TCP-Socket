#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <signal.h>

#define MAX_CLIENTS 100
#define BUFFER_SIZE 1024
#define USERNAME_SIZE 50
#define MESSAGE_SIZE (BUFFER_SIZE + USERNAME_SIZE + 10) // Extra space for formatting
#define PORT 8888

// Structure to hold client information
typedef struct {
    int socket;
    char username[USERNAME_SIZE];
    int id;
} client_t;

// Global variables
client_t *clients[MAX_CLIENTS];
pthread_mutex_t clients_mutex = PTHREAD_MUTEX_INITIALIZER;
int client_count = 0;

// Function to add a client to the array
void add_client(client_t *client) {
    pthread_mutex_lock(&clients_mutex);
    
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (!clients[i]) {
            clients[i] = client;
            client->id = i;
            break;
        }
    }
    
    client_count++;
    pthread_mutex_unlock(&clients_mutex);
}

// Function to remove a client from the array
void remove_client(int id) {
    pthread_mutex_lock(&clients_mutex);
    
    if (clients[id]) {
        free(clients[id]);
        clients[id] = NULL;
        client_count--;
    }
    
    pthread_mutex_unlock(&clients_mutex);
}

// Function to send message to all clients except sender
void broadcast_message(char *message, int sender_id) {
    pthread_mutex_lock(&clients_mutex);
    
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i] && clients[i]->id != sender_id) {
            if (send(clients[i]->socket, message, strlen(message), 0) < 0) {
                perror("Error: Failed to send message");
                break;
            }
        }
    }
    
    pthread_mutex_unlock(&clients_mutex);
}

// Function to handle client communication
void *handle_client(void *arg) {
    char buffer[BUFFER_SIZE];
    char message[MESSAGE_SIZE];
    int leave_flag = 0;
    
    client_t *client = (client_t *)arg;
    
    // Get username
    if (recv(client->socket, buffer, BUFFER_SIZE, 0) <= 0) {
        printf("Client didn't provide a username.\n");
        leave_flag = 1;
    } else {
        // Copy the username with length check to prevent buffer overflow
        strncpy(client->username, buffer, USERNAME_SIZE - 1);
        client->username[USERNAME_SIZE - 1] = '\0'; // Ensure null termination
        
        // Announce new client
        snprintf(message, MESSAGE_SIZE, "%s has joined the chat.\n", client->username);
        printf("%s", message);
        broadcast_message(message, client->id);
    }
    
    memset(buffer, 0, BUFFER_SIZE);
    
    while (leave_flag == 0) {
        int receive = recv(client->socket, buffer, BUFFER_SIZE - 1, 0);
        
        if (receive > 0) {
            buffer[receive] = '\0'; // Ensure null termination
            
            if (strlen(buffer) > 0) {
                if (strcmp(buffer, "EXIT") == 0) {
                    snprintf(message, MESSAGE_SIZE, "%s has left the chat.\n", client->username);
                    printf("%s", message);
                    broadcast_message(message, client->id);
                    leave_flag = 1;
                } else {
                    // Format and broadcast message
                    snprintf(message, MESSAGE_SIZE, "%s: %s\n", client->username, buffer);
                    printf("%s", message);
                    broadcast_message(message, client->id);
                }
            }
        } else if (receive == 0 || receive < 0) {
            snprintf(message, MESSAGE_SIZE, "%s has left the chat.\n", client->username);
            printf("%s", message);
            broadcast_message(message, client->id);
            leave_flag = 1;
        }
        
        memset(buffer, 0, BUFFER_SIZE);
    }
    
    // Close connection and clean up
    close(client->socket);
    remove_client(client->id);
    pthread_detach(pthread_self());
    
    return NULL;
}

// Signal handler for graceful shutdown
void handle_signal(int sig) {
    // Clean up and close all client connections
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i]) {
            close(clients[i]->socket);
            free(clients[i]);
            clients[i] = NULL;
        }
    }
    
    printf("\nServer shutting down...\n");
    exit(0);
}

int main() {
    int server_socket, client_socket;
    struct sockaddr_in server_addr, client_addr;
    pthread_t thread_id;
    socklen_t client_size;
    
    // Set up signal handler
    signal(SIGINT, handle_signal);
    
    // Create socket
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0) {
        perror("Error: Socket creation failed");
        exit(EXIT_FAILURE);
    }
    
    // Set socket options to reuse address
    int opt = 1;
    if (setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("Error: setsockopt failed");
        close(server_socket);
        exit(EXIT_FAILURE);
    }
    
    // Configure server address
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);
    
    // Bind socket
    if (bind(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("Error: Bind failed");
        close(server_socket);
        exit(EXIT_FAILURE);
    }
    
    // Listen for connections
    if (listen(server_socket, 10) < 0) {
        perror("Error: Listen failed");
        close(server_socket);
        exit(EXIT_FAILURE);
    }
    printf("Soham_Ghosh-23BPS1146\n");
    printf("==== CHAT SERVER STARTED ====\n");
    printf("Listening on port %d...\n", PORT);
    
    // Accept and handle client connections
    while (1) {
        client_size = sizeof(client_addr);
        client_socket = accept(server_socket, (struct sockaddr*)&client_addr, &client_size);
        
        if (client_socket < 0) {
            perror("Error: Accept failed");
            continue;
        }
        
        // Check if maximum client limit reached
        if (client_count >= MAX_CLIENTS) {
            printf("Maximum clients connected. Connection rejected.\n");
            close(client_socket);
            continue;
        }
        
        // Client setup
        client_t *client = (client_t *)malloc(sizeof(client_t));
        if (!client) {
            perror("Error: Memory allocation failed");
            close(client_socket);
            continue;
        }
        
        client->socket = client_socket;
        
        // Print connection details
        char client_ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &(client_addr.sin_addr), client_ip, INET_ADDRSTRLEN);
        printf("Client connected from %s:%d\n", client_ip, ntohs(client_addr.sin_port));
        
        // Add client to array and create thread
        add_client(client);
        if (pthread_create(&thread_id, NULL, &handle_client, (void *)client) != 0) {
            perror("Error: Thread creation failed");
            close(client_socket);
            remove_client(client->id);
            free(client);
            continue;
        }
    }
    
    return 0;
}
