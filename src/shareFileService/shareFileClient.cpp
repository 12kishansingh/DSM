#include "../../headers/shareFile.hpp"
#include "../../headers/Status_codes.hpp"
#include "../../headers/clientsCommand.hpp"

#define QUEUE_DEPTH 64
#define BLOCK_SIZE_ 65536

int send_all_sync(int sock, const void *data, size_t len)
{
    const char *ptr = static_cast<const char *>(data);
    while (len > 0)
    {
        ssize_t sent = send(sock, ptr, len, 0);
        if (sent <= 0)
            return false;
        ptr += sent;
        len -= sent;
    }
    return true;
}

int send_all_uring(io_uring &ring, int sock, const char *data, size_t len)
{
    size_t offset = 0;

    while (offset < len)
    {
        io_uring_sqe *sqe = io_uring_get_sqe(&ring);
        if (!sqe)
            return false;

        io_uring_prep_send(sqe, sock, data + offset, len - offset, 0);

        int ret = io_uring_submit(&ring);
        if (ret < 0)
            return false;

        io_uring_cqe *cqe;
        ret = io_uring_wait_cqe(&ring, &cqe);
        if (ret < 0)
            return false;

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

int send_file(const char *filename, const char *IP, const char *folder)
{

    int file = open(filename, O_RDONLY);
    if (file < 0)
    {
        perror("open");
        return -1;
    }

    struct stat st{};
    if (fstat(file, &st) < 0)
    {
        perror("fstat");
        close(file);
        return -1;
    }

    // creating and connecting socket.
    int sock = create_socket();
    connect_socket(sock, IP);

    if (sock < 0)
    {
        std::cerr << "Failed to create socket\n";
        return -1;
    }

    const char *command = "shareFile";

    if (!send_all_sync(sock, command, strlen(command)))
    {
        perror("Failed to send command");
        return -1;
    }

    printf("\t\tWait till the receiver accept the connection request............\n");

    char response[128];
    int valread = read(sock, response, sizeof(response) - 1);
    response[valread] = '\0';
    if (strcmp(response, STATUS_MESSAGES[SUCCESS]) != 0)
    {
        printf("Server response: %s\n", response);
        return -1;
    }

    io_uring ring;
    int ret = io_uring_queue_init(QUEUE_DEPTH, &ring, 0);
    if (ret < 0)
    {
        std::cerr << "io_uring_queue_init failed: " << strerror(-ret) << "\n";
        return -1;
    }

    uint64_t filesize = st.st_size;
    uint32_t namelen = strlen(filename);

    int folderlen = strlen(folder);

    if (!send_all_sync(sock, &folderlen, sizeof(folderlen)) ||
        !send_all_sync(sock, folder, folderlen) ||
        !send_all_sync(sock, &namelen, sizeof(namelen)) ||
        !send_all_sync(sock, filename, namelen) ||
        !send_all_sync(sock, &filesize, sizeof(filesize)))
    {
        std::cerr << "Failed to send file metadata\n";
        close(file);
        return -1;
    }

    TransferStats stats;
    ProgressUI ui;

    stats.start(filesize);

    char buffer[BLOCK_SIZE_];
    while (true)
    {
        ssize_t bytes = read(file, buffer, sizeof(buffer));
        if (bytes < 0)
        {
            perror("read");
            close(file);
            return -1;
        }
        if (bytes == 0)
            break;

        if (!send_all_uring(ring, sock, buffer, bytes))
        {
            std::cerr << "Failed to send file data\n";
            close(file);
            return -1;
        }

        stats.update(bytes);
        ui.render(stats);
    }

    close(file);
    close(sock);
    ui.done();
    io_uring_queue_exit(&ring);
    return 1;
}
