#include "../../headers/clientsCommand.h"

void handle_connect_client(int sock)
{

    int INPUT_SIZE = 100;
    char ip_input[INPUT_SIZE];

    printf("Enter the IP address of the server to connect: ");

    fflush(stdout);

    if (fgets(ip_input, sizeof(ip_input), stdin) != NULL)
    {
        ip_input[strcspn(ip_input, "\n")] = 0; // Remove the newline
    }

    if (connect_socket(sock, ip_input) < 0)
    {
        printf("Failed to connect to server at %s\n", ip_input);
        close(sock);
        return;
    }

    // 2. Keep it open for multiple commands
    while (1)
    {
        printf(ANSI_COLOR_GREEN "\nDevices/%s > " ANSI_COLOR_RESET, ip_input);
        fgets(command, sizeof(command), stdin);
        command[strcspn(command, "\n")] = 0;

        if (strcmp(command, "exit") == 0)
            break;

        // Send command through the persistent socket
        send(sock, command, strlen(command), 0);

        // Receive response
        int valread = read(sock, buffer, sizeof(buffer) - 1);
        if (valread > 0)
        {
            buffer[valread] = '\0';
            printf("Server: %s\n", buffer);
        }
        else
        {
            printf("Connection lost.\n");
            break;
        }
    }

    // 3. Close only when finished
    close(sock);
}