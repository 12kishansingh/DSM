#ifndef SERVER_SERVICE_H
#define SERVER_SERVICE_H
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>

typedef void (*command_handler_t)(int);

typedef struct
{
    const char *cmd_name;
    command_handler_t handler;
} CommandMapServer;

void handle_status_check(int sock);

void handle_connect_server(int sock);

void handle_share_file_server(int sock);

extern CommandMapServer server_dispatch_table[];

#endif