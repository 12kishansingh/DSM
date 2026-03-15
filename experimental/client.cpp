#include <iostream>
#include <fcntl.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <liburing.h>
#include <sys/stat.h>
#include <cstring>

#define PORT 8080
#define QUEUE_DEPTH 64
#define BLOCK_SIZE_ 65536

bool send_all_sync(int sock, const void* data, size_t len)
{
    const char* ptr = static_cast<const char*>(data);
    while (len > 0)
    {
        ssize_t sent = send(sock, ptr, len, 0);
        if (sent <= 0) return false;
        ptr += sent;
        len -= sent;
    }
    return true;
}

bool send_all_uring(io_uring& ring, int sock, const char* data, size_t len)
{
    size_t offset = 0;

    while (offset < len)
    {
        io_uring_sqe* sqe = io_uring_get_sqe(&ring);
        if (!sqe) return false;

        io_uring_prep_send(sqe, sock, data + offset, len - offset, 0);

        int ret = io_uring_submit(&ring);
        if (ret < 0) return false;

        io_uring_cqe* cqe;
        ret = io_uring_wait_cqe(&ring, &cqe);
        if (ret < 0) return false;

        if (cqe->res <= 0)
        {
            io_uring_cqe_seen(&ring, cqe);
            return false;
        }

        offset += cqe->res;
        io_uring_cqe_seen(&ring, cqe);
    }

    return true;
}

bool send_file(io_uring& ring, int sock, const char* filename)
{
    int file = open(filename, O_RDONLY);
    if (file < 0)
    {
        perror("open");
        return false;
    }

    struct stat st{};
    if (fstat(file, &st) < 0)
    {
        perror("fstat");
        close(file);
        return false;
    }

    uint64_t filesize = st.st_size;
    uint32_t namelen = strlen(filename);

    if (!send_all_sync(sock, &namelen, sizeof(namelen)) ||
        !send_all_sync(sock, filename, namelen) ||
        !send_all_sync(sock, &filesize, sizeof(filesize)))
    {
        std::cerr << "Failed to send file metadata\n";
        close(file);
        return false;
    }

    char buffer[BLOCK_SIZE_];

    while (true)
    {
        ssize_t bytes = read(file, buffer, sizeof(buffer));
        if (bytes < 0)
        {
            perror("read");
            close(file);
            return false;
        }
        if (bytes == 0) break;

        if (!send_all_uring(ring, sock, buffer, bytes))
        {
            std::cerr << "Failed to send file data\n";
            close(file);
            return false;
        }
    }

    close(file);
    return true;
}

int main()
{
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0)
    {
        perror("socket");
        return 1;
    }

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(PORT);
    inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr);

    if (connect(sock, (sockaddr*)&addr, sizeof(addr)) < 0)
    {
        perror("connect");
        close(sock);
        return 1;
    }

    io_uring ring;
    int ret = io_uring_queue_init(QUEUE_DEPTH, &ring, 0);
    if (ret < 0)
    {
        std::cerr << "io_uring_queue_init failed: " << strerror(-ret) << "\n";
        close(sock);
        return 1;
    }

    send_file(ring, sock, "sum");
    send_file(ring, sock, "input");

    io_uring_queue_exit(&ring);
    close(sock);

    return 0;
}