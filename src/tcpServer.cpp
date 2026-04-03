#include "../headers/connection.hpp"
#include "../headers/serverService.hpp"
#include "../headers/threadSafety.hpp"

char client_ip[INET_ADDRSTRLEN] = {0};

void *server_listener_thread_tcp(void *args)
{
    struct server_args *s_args = (struct server_args *)args;
    struct sockaddr_in address;
    int addrlen = sizeof(address);
    int new_socket;

    while (1)
    {
        new_socket = accept(s_args->server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen);

        if (new_socket < 0)
        {
            perror("accept failed");
            continue;
        }

        // Convert IP to human-readable form
        inet_ntop(AF_INET, &address.sin_addr, client_ip, INET_ADDRSTRLEN);

        char buffer[256];
        // 2. Read the message sent by the client
        // recv() returns the number of bytes received
        ssize_t bytes_read = recv(new_socket, buffer, sizeof(buffer) - 1, 0);

        if (bytes_read > 0)
        {
            // Null-terminate the string and clean up newlines
            buffer[bytes_read] = '\0';
            buffer[strcspn(buffer, "\r\n")] = 0;

            // 3. Dispatch Logic: Execute code based on the command
            int found = 0;
            for (int i = 0; server_dispatch_table[i].cmd_name != NULL; i++)
            {
                if (strcmp(buffer, server_dispatch_table[i].cmd_name) == 0)
                {
                    int *sock_ptr = new int(new_socket); // replace malloc

                    try
                    {
                        std::thread t(server_dispatch_table[i].handler, sock_ptr);
                        t.detach(); // equivalent to pthread_detach
                    }
                    catch (...)
                    {
                        perror("thread creation failed");
                        delete sock_ptr;
                        close(new_socket);
                    }

                    found = 1;
                    break;
                }
            }

            if (!found)
            {
                const char *err = "Unknown Command\n";
                send(new_socket, err, strlen(err), 0);
            }
        }
    }
}