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

#define BUFFER_SIZE 2048
#define SERVER_IP "127.0.0.1"
#define PORT 8888

volatile sig_atomic_t flag = 0;
int socket_fd = 0;

// Function to trim newline characters
void trim_newline(char *str) {
    int len = strlen(str);
    if (len > 0 && str[len-1] == '\n') {
        str[len-1] = '\0';
    }
}

// Signal handler
void catch_ctrl_c(int sig) {
    flag = 1;
}

// Function to send messages to server
void send_message() {
    char message[BUFFER_SIZE];
    
    while (1) {
        if (flag) {
            break;
        }
        
        fgets(message, BUFFER_SIZE, stdin);
        trim_newline(message);
        
        if (strlen(message) > 0) {
            if (send(socket_fd, message, strlen(message), 0) < 0) {
                perror("Error: Failed to send message");
                break;
            }
            
            if (strcmp(message, "EXIT") == 0) {
                flag = 1;
                break;
            }
        }
    }
    
    catch_ctrl_c(2);
}

// Function to receive messages from server
void receive_message() {
    char message[BUFFER_SIZE];
    
    while (1) {
        if (flag) {
            break;
        }
        
        int receive = recv(socket_fd, message, BUFFER_SIZE, 0);
        if (receive > 0) {
            printf("%s", message);
        } else if (receive == 0) {
            printf("Connection closed by the server.\n");
            flag = 1;
            break;
        }
        
        // Clear the buffer
        memset(message, 0, sizeof(message));
    }
}

int main() {
    char username[50];
    struct sockaddr_in server_addr;
    pthread_t send_thread, recv_thread;
    
    // Set up signal handling
    signal(SIGINT, catch_ctrl_c);
    
    // Create socket
    socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_fd < 0) {
        perror("Error: Socket creation failed");
        exit(EXIT_FAILURE);
    }
    
    // Configure server address
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    
    // Convert IP address from string to binary form
    if (inet_pton(AF_INET, SERVER_IP, &server_addr.sin_addr) <= 0) {
        perror("Error: Invalid address");
        close(socket_fd);
        exit(EXIT_FAILURE);
    }
    
    // Connect to server
    if (connect(socket_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("Error: Connection failed");
        close(socket_fd);
        exit(EXIT_FAILURE);
    }
    printf("Soham_Ghosh_23BPS1146\n");
    printf("==== CHAT CLIENT ====\n");
    
    // Get username
    printf("Enter your username: ");
    fgets(username, 50, stdin);
    trim_newline(username);
    
    if (strlen(username) < 2 || strlen(username) >= 50-1) {
        printf("Username must be between 2-49 characters.\n");
        close(socket_fd);
        exit(EXIT_FAILURE);
    }
    
    // Send username to server
    if (send(socket_fd, username, strlen(username), 0) < 0) {
        perror("Error: Failed to send username");
        close(socket_fd);
        exit(EXIT_FAILURE);
    }
    printf("Soham_Ghosh_22BPS1146\n");
    printf("\nWelcome to the chat, %s!\n", username);
    printf("You can exit anytime by typing 'EXIT'\n\n");
    
    // Create threads for sending and receiving messages
    if (pthread_create(&send_thread, NULL, (void *)send_message, NULL) != 0) {
        perror("Error: Failed to create send thread");
        close(socket_fd);
        exit(EXIT_FAILURE);
    }
    
    if (pthread_create(&recv_thread, NULL, (void *)receive_message, NULL) != 0) {
        perror("Error: Failed to create receive thread");
        close(socket_fd);
        exit(EXIT_FAILURE);
    }
    
    // Wait for threads to complete
    while (1) {
        if (flag) {
            printf("\nExiting...\n");
            break;
        }
        sleep(1);
    }
    
    close(socket_fd);
    return 0;
}
