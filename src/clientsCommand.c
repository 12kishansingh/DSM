#include "../headers/clientsCommand.h"
#include "../headers/threadSafety.h"

CommandMapClient client_dispatch_table[] = {
    {"listConnections", handle_list_connections},
    {"status", handleStatus},
    {"exit", handle_exit},
    {"connect", handle_connect_client},
    {"shareFile", handle_share_file_client},
    {NULL, NULL} // Sentinel value to mark the end
};

char buffer[BUFFER_SIZE] = {0};
char command[BUFFER_SIZE] = {0};

int sendToServer(const char *command, const char *SERVER_IP)
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
        established_connection = 1;
    }

    close(sock);
    return established_connection;
}

void *commands(void *args)
{

    SharedData *mesh_info = ((struct commands_args *)args)->mesh_info;

    while (1)
    {
        printf(ANSI_COLOR_GREEN "\n> " ANSI_COLOR_RESET);
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