#include "../../headers/clientsCommand.hpp"
#include "../../headers/shareFile.hpp"
#include "../../headers/threadSafety.hpp"
#include "../../headers/Status_codes.hpp"

off_t *train_offset;
int_fast64_t *train_chunk_size;

int findTotatConnections()
{
    int connections = 0;
    while (ip_list[connections] != NULL)
        connections++;
    return connections;
}

void freeMemory()
{
    for (int i = 0; ip_list[i] != NULL; i++)
    {
        free(ip_list[i]);
        free(ip_status[i]);
    }
    free(ip_list);
    free(train_offset);
    free(train_chunk_size);
    free(ip_status);
}

int_fast64_t getFileSize(const char *filePath)
{
    struct stat st;

    if (stat(filePath, &st) == 0)
    {
        return st.st_size;
    }
    else
    {
        perror("Failed to get file size");
        return -1;
    }
}

// int_fast64_t adjustToNextLine(int fd, int_fast64_t pos, int_fast64_t fileSize)
// {
//     if (pos >= fileSize)
//         return fileSize;

//     const size_t BUF_SIZE = 4096;
//     char buffer[BUF_SIZE];

//     while (pos < fileSize)
//     {
//         ssize_t bytesRead = pread(fd, buffer, BUF_SIZE, pos);
//         if (bytesRead <= 0)
//             break;

//         for (ssize_t i = 0; i < bytesRead; i++)
//         {
//             if (buffer[i] == '\n')
//             {
//                 return pos + i + 1; // next line start
//             }
//         }

//         pos += bytesRead;
//     }

//     return fileSize;
// }

void divideFileIntoChunks(const char *trainfilepath, int totalConnections)
{
    int fd = open(trainfilepath, O_RDONLY);
    if (fd < 0)
    {
        perror("File open failed");
        return;
    }

    int_fast64_t fileSize = getFileSize(trainfilepath);
    if (fileSize <= 0)
    {
        perror("Invalid file size");
        close(fd);
        return;
    }

    int_fast64_t offset = 0;

    // total weight = (n-1)*1 + 3
    int totalWeight = (totalConnections - 1) + 3;

    for (int i = 0; i < totalConnections; i++)
    {
        int currWeight = (strcmp(ip_list[i], myIp) == 0) ? 3 : 1;

        int_fast64_t chunk = (fileSize * currWeight) / totalWeight;

        int_fast64_t tentative_end = offset + chunk;
        int_fast64_t adjusted_end;

        if (i == totalConnections - 1)
        {
            adjusted_end = fileSize; // last node gets all remaining
        }
        else
        {
            // adjusted_end = adjustToNextLine(fd, tentative_end, fileSize);
        }

        int_fast64_t final_chunk = adjusted_end - offset;

        train_offset[i] = offset;
        train_chunk_size[i] = final_chunk;

        offset = adjusted_end;
    }

    close(fd);
}

void *distributiveComputingOverNetwork(void *args)
{
    struct distributiveComputingargs *dis_args = (struct distributiveComputingargs *)args;
    const char *command = "distributiveComputing";
    char *res = sendToServer(command, dis_args->IP);
    if (strcmp(res, STATUS_MESSAGES[OPEN_SHAREFILE_CONNECTION]) == 0)
    {
        // now, send the code file and the train file chunk to the server using shareFile functions...
        // 1. Send code file...
        const bool iscmdSendFile = false;
        if (send_file(dis_args->codePath, dis_args->IP, "code", iscmdSendFile) < 0)
        {
            fprintf(stderr, "Failed to send code file to %s\n", dis_args->IP);
            return NULL;
        }

        if (send_file(dis_args->trainFilePath, dis_args->IP, "train", iscmdSendFile, dis_args->train_offset, dis_args->train_chunk_size) < 0)
        {
            fprintf(stderr, "Failed to send train file chunk to %s\n", dis_args->IP);
            return NULL;
        }
    }
    else
    {
        printf("Failed to open connection for distributive computing on IP: %s\n", dis_args->IP);
        return NULL;
    }

    return NULL;
}

void *distributiveComputingLocal(void *args)
{
    return NULL;
}

void handle_distributive_systems()
{
    // Send UDP packet to all the IP's in the network, to know which IP's are actually running the service.
    sendRequest();

    int totalConnections = findTotatConnections();

    train_offset = (off_t *)malloc(sizeof(off_t) * totalConnections);
    train_chunk_size = (int_fast64_t *)malloc(sizeof(int_fast64_t) * totalConnections);

    printf("Enter the code file path: ");
    char codepath[128];
    if (!fgets(codepath, sizeof(codepath), stdin))
    {
        perror("Failed to read codepath");
        return;
    }
    codepath[strcspn(codepath, "\n")] = 0;

    char trainfilepath[128];

    printf("Enter the train file path: ");
    if (!fgets(trainfilepath, sizeof(trainfilepath), stdin))
    {
        perror("Failed to read trainfilepath");
        return;
    }
    trainfilepath[strcspn(trainfilepath, "\n")] = 0;

    // Make threads equal to totalConnections.

    std::thread threads[totalConnections];

    for (int i = 0; i < totalConnections; i++)
    {
        struct distributiveComputingargs *dis_args =
            (struct distributiveComputingargs *)malloc(sizeof(struct distributiveComputingargs));

        dis_args->codePath = codepath;
        dis_args->trainFilePath = trainfilepath;
        dis_args->IP = ip_list[i];
        dis_args->train_offset = train_offset[i];
        dis_args->train_chunk_size = train_chunk_size[i];

        if (strcmp(ip_list[i], myIp) == 0)
        {
            if (std::thread(distributiveComputingLocal, dis_args).joinable())
            {
                threads[i] = std::thread(distributiveComputingLocal, dis_args);
            }
            else
            {
                perror("Failed to create Local distributive computing thread");
                return;
            }
            continue;
        }
        if (std::thread(distributiveComputingOverNetwork, dis_args).joinable())
        {
            // Also, implement the logic to give each i.p its respective train data before assigning
            threads[i] = std::thread(distributiveComputingOverNetwork, dis_args);
        }
        else
        {
            perror("Failed to create distributive computing thread");
            return;
        }
    }

    // Starting each thread..
    for (int i = 0; i < totalConnections; i++)
    {
        threads[i].join();
    }

    freeMemory();
}