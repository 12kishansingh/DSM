#include "../../headers/clientsCommand.hpp"
#include "../../headers/shareFile.hpp"
#include "../../headers/Status_codes.hpp"

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

void *distributiveComputingOverNetwork(void *args)
{
    struct distributiveComputingargs *dis_args = (struct distributiveComputingargs *)args;
    const char *command = "distributiveComputing";
    char *res = sendToServer(command, dis_args->IP);
    if (strcmp(res, STATUS_MESSAGES[OPEN_SHAREFILE_CONNECTION]) == 0)
    {
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

    pthread_t threads[totalConnections];

    for (int i = 0; i < totalConnections; i++)
    {
        struct distributiveComputingargs *dis_args =
            (struct distributiveComputingargs *)malloc(sizeof(struct distributiveComputingargs));

        dis_args->codePath = codepath;
        // Also, implement the logic to give each i.p its respective train data before assigning
        dis_args->trainFilePath = trainfilepath;
        dis_args->IP = ip_list[i];

        if (pthread_create(&threads[i], NULL, distributiveComputingOverNetwork, dis_args) != 0)
        {
            perror("Failed to create distributive computing thread");
            return;
        }
    }

    // Starting each thread..
    for (int i = 0; i < totalConnections; i++)
    {
        pthread_join(threads[i], NULL);
    }

    freeMemory();
}