#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include "../headers/clientsCommand.h"
#include "../headers/threadSafety.h"

CommandMapClient client_dispatch_table[] = {
    {"listConnections", handle_list_connections},
    {"status", handleStatus},
    {"exit", handle_exit},
    {NULL, NULL} // Sentinel value to mark the end
};

#define BUFFER_SIZE 1024
char buffer[BUFFER_SIZE];


void handle_exit(int sock)
{
    exit(0);
}


void handleStatus(int sock)
{
    char *SERVER_IP = "127.0.0.1"; // Assuming server is on localhost for testing
    int result = sendToServer("status", SERVER_IP);
    if (result)
    {
        printf("Server is active and responded to status check.\n");
    }
    else
    {
        printf("Failed to connect to server or server did not respond.\n");
    }
}

 

#define BROADCAST_PORT 8080 // Ensure your server is listening on this UDP port
#define BUFFER_SIZE 1024

// This replaces your entire handle_list_connections loop logic
void handle_list_connections(int unused_sock) 
{
    printf("Scanning network for active servers via UDP Broadcast...\n");
    
    int sock;
    struct sockaddr_in broadcast_addr;
    int broadcast_enable = 1;
    char buffer[BUFFER_SIZE];
    
    // 1. Create UDP Socket (SOCK_DGRAM)
    if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("Socket creation failed");
        return;
    }

    // 2. Enable Broadcast permission on the socket
    if (setsockopt(sock, SOL_SOCKET, SO_BROADCAST, &broadcast_enable, sizeof(broadcast_enable)) < 0) {
        perror("Setsockopt (SO_BROADCAST) failed");
        close(sock);
        return;
    }

    // 3. Set a Timeout (So we don't wait forever if no one responds)
    struct timeval tv;
    tv.tv_sec = 1;  // Wait 1 second for replies
    tv.tv_usec = 0;
    if (setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0) {
        perror("Setsockopt (SO_RCVTIMEO) failed");
    }

    // 4. Configure Broadcast Address
    memset(&broadcast_addr, 0, sizeof(broadcast_addr));
    broadcast_addr.sin_family = AF_INET;
    broadcast_addr.sin_port = htons(BROADCAST_PORT);
    broadcast_addr.sin_addr.s_addr = inet_addr("255.255.255.255"); 

    // 5. Send the discovery command
    char *command = "status";
    sendto(sock, command, strlen(command), 0, (struct sockaddr*)&broadcast_addr, sizeof(broadcast_addr));

    // 6. Collect all responses
    struct sockaddr_in server_addr;
    socklen_t addr_len = sizeof(server_addr);
    int found_count = 0;

    while (1) {
        int valread = recvfrom(sock, buffer, sizeof(buffer) - 1, 0, (struct sockaddr*)&server_addr, &addr_len);
        
        if (valread < 0) {
            // Likely timed out (EAGAIN/EWOULDBLOCK), which means no more servers responding
            break; 
        }

        buffer[valread] = '\0';
        printf("Active connection found at IP: %s\n", inet_ntoa(server_addr.sin_addr));
        found_count++;
    }

    if (found_count == 0) {
        printf("No active servers found on the local network.\n");
    }

    close(sock);
}

int sendToServer(char *command, char *SERVER_IP)
{
    int sock = 0, valread;
    struct sockaddr_in serv_addr;

    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        return 0;
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(8080);

    if (inet_pton(AF_INET, SERVER_IP, &serv_addr.sin_addr) <= 0)
    {
        return 0;
    }

    // Connect with error checking
    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
        return 0;
    }

    send(sock, command, strlen(command), 0);

    valread = read(sock, buffer, sizeof(buffer));

    int established_connection = 0;

    if (valread > 0)
    {
        buffer[valread] = '\0';
        resume();
        established_connection = 1;
    }

    close(sock);
    return established_connection;
}

void *commands(void *args)
{
    char command[1024];

    SharedData *mesh_info = ((struct commands_args *)args)->mesh_info;

    while (1)
    {
        printf("\n> ");
        fflush(stdout); // Ensure prompt shows up

        if (fgets(command, sizeof(command), stdin) != NULL)
        {
            command[strcspn(command, "\n")] = 0; // Remove newline

            int found = 0;
            for (int i = 0; client_dispatch_table[i].cmd_name != NULL; i++)
            {
                if (strcmp(command, client_dispatch_table[i].cmd_name) == 0)
                {
                    client_dispatch_table[i].handler(0); // Pass 0 or any relevant argument if needed
                    found = 1;
                    break;
                }
            }

            if (!found)
            {
                printf("Unknown Command\n");
            }
        }
    }
    return NULL;
}