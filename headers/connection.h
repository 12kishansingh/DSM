#ifndef CONNECTION_H
#define CONNECTION_H
 

struct server_args {
    int server_fd;
};

void* server_listener_thread_tcp(void* args);
void* udp_discovery_responder(void* args);

void connection();

#endif