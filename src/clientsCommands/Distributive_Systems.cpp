#include "../../headers/clientsCommand.hpp"

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

void handle_distributive_systems()
{
    // Send UDP packet to all the IP's in the network, to know which IP's are actually running the service.
    sendRequest();

    int totalConnections = findTotatConnections();
    
    // Make threads equal to totalConnections.
    


    freeMemory();
}