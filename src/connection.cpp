#include "../headers/connection.hpp"
#include "../headers/clientsCommand.hpp"
#include "../headers/serverService.hpp"
#include "../headers/threadSafety.hpp"


void connection()
{
    int server_fd;
    struct sockaddr_in address;
    int opt = 1;

    // 1. Setup Socket
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(8080);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0)
    {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, 10) < 0)
    {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    // 2. Thread Management (std::thread)

    // UDP thread
    std::thread server_udp_thread(udp_discovery_responder, nullptr);

    // Server thread
    struct server_args s_args;
    s_args.server_fd = server_fd;

    std::thread server_thread(server_listener_thread_tcp, &s_args);

    // UI thread
    struct commands_args c_args;
    c_args.mesh_info = &mesh_info;

    std::thread ui_thread(commands, &c_args);

    // 3. Join threads
    server_thread.join();
    ui_thread.join();
    server_udp_thread.join();

    close(server_fd);
}