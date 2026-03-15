#include "../../headers/clientsCommand.h"

void handle_list_connections(int unused_sock)
{
    printf("Scanning network for active servers via UDP Broadcast...\n");

    int sock;
    struct sockaddr_in broadcast_addr;
    int broadcast_enable = 1;

    // 1. Create UDP Socket (SOCK_DGRAM)
    if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
    {
        perror("Socket creation failed");
        return;
    }

    // 2. Enable Broadcast permission on the socket
    if (setsockopt(sock, SOL_SOCKET, SO_BROADCAST, &broadcast_enable, sizeof(broadcast_enable)) < 0)
    {
        perror("Setsockopt (SO_BROADCAST) failed");
        close(sock);
        return;
    }

    // 3. Set a Timeout (So we don't wait forever if no one responds)
    struct timeval tv;
    tv.tv_sec = 1; // Wait 1 second for replies
    tv.tv_usec = 0;
    if (setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0)
    {
        perror("Setsockopt (SO_RCVTIMEO) failed");
    }

    // 4. Configure Broadcast Address
    memset(&broadcast_addr, 0, sizeof(broadcast_addr));
    broadcast_addr.sin_family = AF_INET;
    broadcast_addr.sin_port = htons(PORT);
    broadcast_addr.sin_addr.s_addr = inet_addr("255.255.255.255");

    
    snprintf(command, sizeof(command), "status");

    sendto(sock, command, strlen(command), 0, (struct sockaddr *)&broadcast_addr, sizeof(broadcast_addr));

    // 6. Collect all responses
    struct sockaddr_in server_addr;
    socklen_t addr_len = sizeof(server_addr);
    int found_count = 0;

    while (1)
    {
        int valread = recvfrom(sock, buffer, sizeof(buffer) - 1, 0, (struct sockaddr *)&server_addr, &addr_len);

        if (valread < 0)
        {
            // Likely timed out (EAGAIN/EWOULDBLOCK), which means no more servers responding
            break;
        }

        buffer[valread] = '\0';
        printf("Active connection found at IP: %s\n", inet_ntoa(server_addr.sin_addr));
        found_count++;
    }

    if (found_count == 0)
    {
        printf("No active servers found on the local network.\n");
    }

    close(sock);
}