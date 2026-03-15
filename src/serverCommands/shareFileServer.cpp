#include "../../headers/serverService.h"
#include <iostream>
#include <fcntl.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <cstring>
#include <sys/socket.h>
#include <sys/stat.h>

#define PORT 8080
#define BLOCK_SIZE_ 65536

// Receive exactly len bytes from socket
bool recv_all(int sock, void *buf, size_t len)
{
    char *ptr = static_cast<char *>(buf);

    while (len > 0)
    {
        ssize_t bytes = recv(sock, ptr, len, 0);

        // Stop on socket error or close
        if (bytes <= 0)
            return false;

        ptr += bytes;
        len -= bytes;
    }

    return true;
}

// Receive file metadata and file data
void receive_file(int sock)
{
    uint32_t namelen;

    // Read filename length
    if (!recv_all(sock, &namelen, sizeof(namelen)))
    {
        std::cerr << "Failed to receive filename length\n";
        return;
    }

    // Validate filename length
    if (namelen == 0 || namelen >= 256)
    {
        std::cerr << "Invalid filename length: " << namelen << "\n";
        return;
    }

    char filename[256];

    // Read filename
    if (!recv_all(sock, filename, namelen))
    {
        std::cerr << "Failed to receive filename\n";
        return;
    }

    // Add null terminator
    filename[namelen] = '\0';

    uint64_t filesize;

    // Read file size
    if (!recv_all(sock, &filesize, sizeof(filesize)))
    {
        std::cerr << "Failed to receive filesize\n";
        return;
    }

    // Print received path
    std::cout << "Received filename from client: " << filename << "\n";

    char cwd[1024];

    // Print server working directory
    if (getcwd(cwd, sizeof(cwd)))
    {
        std::cout << "Server working directory: " << cwd << "\n";
    }

    // Create output folder
    mkdir("received_files", 0755);

    // Extract only file name from path
    const char *base = strrchr(filename, '/');
    base = (base != nullptr) ? base + 1 : filename;

    char outpath[512];

    // Build destination path
    snprintf(outpath, sizeof(outpath), "received_files/%s", base);

    // Open output file
    int file = open(outpath, O_CREAT | O_WRONLY | O_TRUNC, 0644);
    if (file < 0)
    {
        perror("open");
        std::cerr << "Tried to create file at: " << outpath << "\n";
        return;
    }

    char buffer[BLOCK_SIZE_];
    uint64_t received = 0;

    // Receive file content
    while (received < filesize)
    {
        size_t need = filesize - received;

        // Limit read size to buffer size
        if (need > sizeof(buffer))
            need = sizeof(buffer);

        // Read file chunk from socket
        ssize_t bytes = recv(sock, buffer, need, 0);
        if (bytes <= 0)
        {
            std::cerr << "Socket closed or recv failed while receiving file data\n";
            break;
        }

        // Write chunk to file
        ssize_t written = write(file, buffer, bytes);
        if (written != bytes)
        {
            perror("write");
            close(file);
            return;
        }

        received += bytes;
    }

    // Close file after writing
    close(file);

    // Confirm full file received
    if (received == filesize)
    {
        std::cout << "File received successfully: " << outpath
                  << " (" << filesize << " bytes)\n";

        const char *msg = "SUCCESS";

        // Send success reply to client
        send(sock, msg, strlen(msg), 0);
    }
    else
    {
        std::cout << "Incomplete file received: " << outpath
                  << " (" << received << "/" << filesize << " bytes)\n";
    }
}

// Handle shareFile command
void handle_share_file_server(int sock)
{
    receive_file(sock);
}