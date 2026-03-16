#ifndef CLIENTS_COMMAND_H
#define CLIENTS_COMMAND_H
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/time.h>

typedef void (*command_handler_t)(int);

typedef struct
{
    const char *cmd_name;
    command_handler_t handler;
} CommandMapClient;

#define ANSI_COLOR_GREEN "\x1b[32m"
#define ANSI_COLOR_RESET "\x1b[0m"

#define PORT 8080 // Ensure your server is listening on this UDP port
#define BUFFER_SIZE 1024

extern char buffer[BUFFER_SIZE];
extern char command[BUFFER_SIZE];

extern CommandMapClient client_dispatch_table[];

int sendToServer(int sock , const char *command, const char *SERVER_IP);

int create_socket(void);

int connect_socket(int sock, const char *ip);

void handle_exit(int sock);

void handle_connect_client(int sock);

void handleStatus(int sock);

void handle_list_connections(int sock);

void *commands(void *args);

#endif
