#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <pthread.h>
#include "../headers/connection.h"
#include "../headers/clientsCommand.h"
#include "../headers/serverService.h"
#include "../headers/threadSafety.h"

// --- THREAD 1: Server Listener ---
void *server_listener_thread_tcp(void *args)
{
    struct server_args *s_args = (struct server_args *)args;
    struct sockaddr_in address;
    int addrlen = sizeof(address);
    int new_socket;

    while (1)
    {
        new_socket = accept(s_args->server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen);

        if (new_socket < 0)
        {
            perror("accept failed");
            continue;
        }

        // 1. Create a buffer to hold the incoming message
        char buffer[1024] = {0};

        // 2. Read the message sent by the client
        // recv() returns the number of bytes received
        ssize_t bytes_read = recv(new_socket, buffer, sizeof(buffer) - 1, 0);

        if (bytes_read > 0)
        {
            // Null-terminate the string and clean up newlines
            buffer[bytes_read] = '\0';
            buffer[strcspn(buffer, "\r\n")] = 0;

            // 3. Dispatch Logic: Execute code based on the command
            int found = 0;
            for (int i = 0; server_dispatch_table[i].cmd_name != NULL; i++)
            {
                if (strcmp(buffer, server_dispatch_table[i].cmd_name) == 0)
                {
                    server_dispatch_table[i].handler(new_socket);
                    wait(); // Wait for the command thread to signal that it's done processing
                    found = 1;
                    break;
                }
            }

            if (!found)
            {
                char *err = "Unknown Command\n";
                send(new_socket, err, strlen(err), 0);
            }
        }

        // pthread_mutex_unlock(&mesh_info.lock);

        // 4. IMPORTANT: Close the socket ONLY after you are done responding

        close(new_socket);
    }
    return NULL;
}


void *udp_discovery_responder(void *arg) {
    int sock;
    struct sockaddr_in serv_addr, cli_addr;
    char buffer[1024];
    socklen_t len = sizeof(cli_addr);

    // 1. Create UDP socket
    sock = socket(AF_INET, SOCK_DGRAM, 0);

    // 2. Bind to the same port the scanner is broadcasting to
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY; // Listen on all interfaces
    serv_addr.sin_port = htons(8080); 

    if (bind(sock, (const struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("UDP Bind failed");
        return NULL;
    }

    // It's only active to respond to the status request from the scanner, so we can keep it in an infinite loop

    while (1) {
        // 3. Wait for "status" message
        int n = recvfrom(sock, buffer, sizeof(buffer) - 1, 0, (struct sockaddr *)&cli_addr, &len);
        if (n > 0) {
            buffer[n] = '\0';
            if (strcmp(buffer, "status") == 0) {
                // 4. Send a reply back to the scanner's IP
                char reply = '1'; // You can send any status info you want here
                sendto(sock, &reply, sizeof(reply), 0, (struct sockaddr *)&cli_addr, len);
            }
        }
    }
    return NULL;
}


void connection()
{
    int server_fd;
    struct sockaddr_in address;
    int opt = 1;

    // 1. Setup Socket
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(8080);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0)
    {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, 10) < 0)
    {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    // 2. Thread Management
    pthread_t server_tid, ui_tid , server_udp_tid;

     // Create the UDP Discovery Responder Thread
    if (pthread_create(&server_udp_tid, NULL, udp_discovery_responder, NULL) != 0) {
        perror("Failed to create UDP discovery responder thread");
        return;
    }

    struct server_args s_args;
    s_args.server_fd = server_fd;

    // Create the Server Thread
    if (pthread_create(&server_tid, NULL, server_listener_thread_tcp, &s_args) != 0)
    {
        perror("Failed to create server thread");
        return;
    }

    // Create the UI/Command Thread

    struct commands_args c_args;

    c_args.mesh_info = &mesh_info;

    if (pthread_create(&ui_tid, NULL, commands, &c_args) != 0)
    {
        perror("Failed to create UI thread");
        return;
    }

    // Wait for threads to finish (which they won't in this infinite loop)
    pthread_join(server_tid, NULL);
    pthread_join(ui_tid, NULL);
    pthread_join(server_udp_tid, NULL);

    close(server_fd);
}
