#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>

int main() {
    int server_fd, new_socket;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);
    char *hello = "Hello from the persistent server!\n";

    // 1. Setup - This only happens ONCE
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(8080);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, 10) < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    printf("Server is live on port 8080. Press Ctrl+C to stop.\n");

    // 2. The Infinite Loop
    printf("\nWaiting for a new connection...\n");
    
    while(1) {
        
        new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen);
        
        if (new_socket < 0) {
            perror("accept");
            continue; // Don't kill the server, just wait for the next attempt
        }

        printf("Connection established with a client!\n");

        // Send data
        send(new_socket, hello, strlen(hello), 0);
        
        // 3. Close the CLIENT socket, but NOT the server_fd
        close(new_socket);
        printf("Response sent and connection closed. Ready for next client.\n");
    }

    // This part is technically unreachable now unless you add a break condition
    close(server_fd);
    return 0;
}