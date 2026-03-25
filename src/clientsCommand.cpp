#include "../headers/clientsCommand.hpp"
#include "../headers/threadSafety.hpp"
#include "../headers/Status_codes.hpp"
#include <math.h>
CommandMapClient client_dispatch_table[] = {
    {"listConnections", handle_list_connections},
    {"status", handleStatus},
    {"exit", handle_exit},
    {"connect", handle_connect_client},
    {"sendFile", handle_send_file},
    {"receiveFile", handle_receive_file},
    {NULL, NULL} // Sentinel value to mark the end
};

char buffer[BUFFER_SIZE] = {0};
char command[BUFFER_SIZE] = {0};

char **ip_list = NULL;
char **ip_status = NULL;
const char *SELFIP = "127.0.0.1";


char *sendToServer(const char *command, const char *IP)
{
    int sock = create_socket();
    connect_socket(sock, IP);
    send(sock, command, strlen(command), 0);

    int valread = read(sock, buffer, sizeof(buffer));

    if (valread > 0)
    {
        buffer[valread] = '\0';

        char *result = (char *)malloc(valread + 1); // +1 for '\0'
        if (!result)
            return NULL;

        memcpy(result, buffer, valread + 1); // copy only valid bytes
        close(sock);
        return result;
    }
    else
    {
        char *err = (char *)STATUS_MESSAGES[CONNECTION_ERROR];
        close(sock);
        return err;
    }
}

int create_socket(void)
{
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0)
    {
        perror("socket");
        return -1;
    }
    return sock;
}

int connect_socket(int sock, const char *ip)
{
    struct sockaddr_in addr;

    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(PORT);

    if (inet_pton(AF_INET, ip, &addr.sin_addr) <= 0)
    {
        perror("inet_pton");
        return -1;
    }

    if (connect(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0)
    {
        perror("connect");
        return -1;
    }

    return 0;
}

//Add Levenshtein Distance (core logic)
int levenshtein(const char *s1, const char *s2)
{
    int len1 = strlen(s1), len2 = strlen(s2);
    int dp[len1 + 1][len2 + 1];

    for (int i = 0; i <= len1; i++) dp[i][0] = i;
    for (int j = 0; j <= len2; j++) dp[0][j] = j;

    for (int i = 1; i <= len1; i++)
    {
        for (int j = 1; j <= len2; j++)
        {
            int cost = (s1[i - 1] == s2[j - 1]) ? 0 : 1;

            dp[i][j] = fmin(
                dp[i - 1][j] + 1,
                fmin(dp[i][j - 1] + 1,
                     dp[i - 1][j - 1] + cost));
        }
    }

    return dp[len1][len2];
}
//Find closest command
const char* find_closest_command(const char *input)
{
    int min_dist = 1000;
    const char *best_match = NULL;

    for (int i = 0; client_dispatch_table[i].cmd_name != NULL; i++)
    {
        int dist = levenshtein(input, client_dispatch_table[i].cmd_name);

        if (dist < min_dist)
        {
            min_dist = dist;
            best_match = client_dispatch_table[i].cmd_name;
        }
    }

    // Threshold (IMPORTANT)
    if (min_dist <= 2)
        return best_match;

    return NULL;
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
                    client_dispatch_table[i].handler();
                    found = 1;
                    break;
                }
            }

            if (!found)
{
    const char *suggestion = find_closest_command(command);

    if (suggestion)
    {
        printf("Did you mean '%s'? [y/n/a/e]: ", suggestion);
        char choice;
        scanf(" %c", &choice);
        getchar(); // consume newline

        if (choice == 'y')
        {
            // run corrected command
            for (int i = 0; client_dispatch_table[i].cmd_name != NULL; i++)
            {
                if (strcmp(suggestion, client_dispatch_table[i].cmd_name) == 0)
                {
                    client_dispatch_table[i].handler();
                    break;
                }
            }
        }
        else if (choice == 'n')
        {
            printf("Running original command: %s\n", command);
        }
        else if (choice == 'a')
        {
            printf("Aborted\n");
        }
        else if (choice == 'e')
        {
            printf("Re-enter command: ");
            fgets(command, sizeof(command), stdin);
            command[strcspn(command, "\n")] = 0;
        }
    }
    else
    {
        printf("Unknown Command\n");
    }
}
        }
    }
    return NULL;
}