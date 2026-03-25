#include "../headers/clientsCommand.hpp"
#include "../headers/threadSafety.hpp"
#include "../headers/Status_codes.hpp"

#include <math.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>

CommandMapClient client_dispatch_table[] = {
    {"listConnections", handle_list_connections},
    {"status", handleStatus},
    {"exit", handle_exit},
    {"connect", handle_connect_client},
    {"sendFile", handle_send_file},
    {"receiveFile", handle_receive_file},
    {NULL, NULL}
};

char buffer[BUFFER_SIZE] = {0};
char command[BUFFER_SIZE] = {0};

char **ip_list = NULL;
char **ip_status = NULL;
const char *SELFIP = "127.0.0.1";


// ================= SOCKET FUNCTIONS =================

char *sendToServer(const char *command, const char *IP)
{
    int sock = create_socket();
    if (sock < 0) return NULL;

    if (connect_socket(sock, IP) < 0)
    {
        close(sock);
        return (char *)STATUS_MESSAGES[CONNECTION_ERROR];
    }

    send(sock, command, strlen(command), 0);

    int valread = read(sock, buffer, sizeof(buffer) - 1);

    if (valread > 0)
    {
        buffer[valread] = '\0';

        char *result = (char *)malloc(valread + 1);
        if (!result)
        {
            close(sock);
            return NULL;
        }

        memcpy(result, buffer, valread + 1);
        close(sock);
        return result;
    }
    else
    {
        close(sock);
        return (char *)STATUS_MESSAGES[CONNECTION_ERROR];
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


// ================= HELPER =================

void to_lowercase(char *str)
{
    for (int i = 0; str[i]; i++)
        str[i] = tolower(str[i]);
}


// ================= LEVENSHTEIN =================

int levenshtein(const char *s1, const char *s2)
{
    int len1 = strlen(s1), len2 = strlen(s2);

    int **dp = (int **)malloc((len1 + 1) * sizeof(int *));
    for (int i = 0; i <= len1; i++)
        dp[i] = (int *)malloc((len2 + 1) * sizeof(int));

    for (int i = 0; i <= len1; i++) dp[i][0] = i;
    for (int j = 0; j <= len2; j++) dp[0][j] = j;

    for (int i = 1; i <= len1; i++)
    {
        for (int j = 1; j <= len2; j++)
        {
            int cost = (s1[i - 1] == s2[j - 1]) ? 0 : 1;

            int a = dp[i - 1][j] + 1;
            int b = dp[i][j - 1] + 1;
            int c = dp[i - 1][j - 1] + cost;

            dp[i][j] = fmin(a, fmin(b, c));
        }
    }

    int result = dp[len1][len2];

    for (int i = 0; i <= len1; i++)
        free(dp[i]);
    free(dp);

    return result;
}


// ================= FIND CLOSEST COMMAND =================

const char* find_closest_command(const char *input)
{
    char input_copy[BUFFER_SIZE];
    strcpy(input_copy, input);
    to_lowercase(input_copy);

    // 🔥 1. SUBSTRING MATCH (PRIORITY)
    for (int i = 0; client_dispatch_table[i].cmd_name != NULL; i++)
    {
        char cmd_copy[BUFFER_SIZE];
        strcpy(cmd_copy, client_dispatch_table[i].cmd_name);
        to_lowercase(cmd_copy);

        if (strstr(cmd_copy, input_copy) != NULL)
        {
            return client_dispatch_table[i].cmd_name;
        }
    }

    // 🔥 2. LEVENSHTEIN MATCH
    int min_dist = 1000;
    const char *best_match = NULL;

    for (int i = 0; client_dispatch_table[i].cmd_name != NULL; i++)
    {
        char cmd_copy[BUFFER_SIZE];
        strcpy(cmd_copy, client_dispatch_table[i].cmd_name);
        to_lowercase(cmd_copy);

        int dist = levenshtein(input_copy, cmd_copy);

        if (dist < min_dist)
        {
            min_dist = dist;
            best_match = client_dispatch_table[i].cmd_name;
        }
    }

    int max_len = strlen(input);
    int threshold = fmax(3, max_len / 2);

    if (min_dist <= threshold)
        return best_match;

    return NULL;
}


// ================= COMMAND LOOP =================

void *commands(void *args)
{
    SharedData *mesh_info = ((struct commands_args *)args)->mesh_info;

    while (1)
    {
        printf(ANSI_COLOR_GREEN "\n> " ANSI_COLOR_RESET);
        fflush(stdout);

        if (fgets(command, sizeof(command), stdin) != NULL)
        {
            command[strcspn(command, "\n")] = 0;

            int found = 0;

            // ✅ CASE-INSENSITIVE MATCH
            for (int i = 0; client_dispatch_table[i].cmd_name != NULL; i++)
            {
                if (strcasecmp(command, client_dispatch_table[i].cmd_name) == 0)
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
                    printf("Did you mean '%s'? [y/n/e]: ", suggestion);

                    char choice;
                    scanf(" %c", &choice);
                    getchar();

                    if (choice == 'y')
                    {
                        for (int i = 0; client_dispatch_table[i].cmd_name != NULL; i++)
                        {
                            if (strcasecmp(suggestion, client_dispatch_table[i].cmd_name) == 0)
                            {
                                client_dispatch_table[i].handler();
                                break;
                            }
                        }
                    }
                    
                    else if (choice == 'n')
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