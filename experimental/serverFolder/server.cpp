#include <iostream>
#include <fcntl.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <cstring>
#include <sys/socket.h>

#define PORT 8080
#define BLOCK_SIZE_ 65536

bool recv_all(int sock, void* buf, size_t len)
{
    char* ptr = static_cast<char*>(buf);
    while (len > 0)
    {
        ssize_t bytes = recv(sock, ptr, len, 0);
        if (bytes <= 0) return false;
        ptr += bytes;
        len -= bytes;
    }
    return true;
}

void receive_file(int sock)
{
    uint32_t namelen;

    if (!recv_all(sock, &namelen, sizeof(namelen)))
    {
        std::cerr << "Failed to receive filename length\n";
        return;
    }

    if (namelen == 0 || namelen >= 256)
    {
        std::cerr << "Invalid filename length: " << namelen << "\n";
        return;
    }

    char filename[256];
    if (!recv_all(sock, filename, namelen))
    {
        std::cerr << "Failed to receive filename\n";
        return;
    }
    filename[namelen] = '\0';

    uint64_t filesize;
    if (!recv_all(sock, &filesize, sizeof(filesize)))
    {
        std::cerr << "Failed to receive filesize\n";
        return;
    }

    int file = open(filename, O_CREAT | O_WRONLY | O_TRUNC, 0644);
    if (file < 0)
    {
        perror("open");
        return;
    }

    char buffer[BLOCK_SIZE_];
    uint64_t received = 0;

    while (received < filesize)
    {
        size_t need = filesize - received;
        if (need > sizeof(buffer)) need = sizeof(buffer);

        ssize_t bytes = recv(sock, buffer, need, 0);
        if (bytes <= 0)
        {
            std::cerr << "Socket closed or recv failed while receiving file data\n";
            break;
        }

        ssize_t written = write(file, buffer, bytes);
        if (written != bytes)
        {
            perror("write");
            close(file);
            return;
        }

        received += bytes;
    }

    close(file);

    if (received == filesize)
        std::cout << "Received file: " << filename << "\n";
    else
        std::cout << "Incomplete file received: " << filename
                  << " (" << received << "/" << filesize << " bytes)\n";
}

int main()
{
    int server = socket(AF_INET, SOCK_STREAM, 0);
    if (server < 0)
    {
        perror("socket");
        return 1;
    }

    int opt = 1;
    setsockopt(server, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(PORT);

    if (bind(server, (sockaddr*)&addr, sizeof(addr)) < 0)
    {
        perror("bind");
        close(server);
        return 1;
    }

    if (listen(server, 10) < 0)
    {
        perror("listen");
        close(server);
        return 1;
    }

    std::cout << "Waiting for connection...\n";

    int client = accept(server, NULL, NULL);
    if (client < 0)
    {
        perror("accept");
        close(server);
        return 1;
    }

    receive_file(client);
    receive_file(client);

    close(client);
    close(server);
    return 0;
}