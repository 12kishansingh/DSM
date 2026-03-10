#ifndef CLIENTS_COMMAND_H
#define CLIENTS_COMMAND_H

typedef void (*command_handler_t)(int);

typedef struct
{
    const char *cmd_name;
    command_handler_t handler;
} CommandMapClient;

extern CommandMapClient client_dispatch_table[];

int sendToServer(char *command , char *SERVER_IP);

void handle_exit(int sock);

void handleStatus(int sock);

void handle_list_connections(int sock);

void *commands(void *args);

#endif
