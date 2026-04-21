#include "../../headers/serverService.hpp"
#include "../../headers/Status_codes.hpp"
#include "../../headers/sockets.hpp"
#include "../../headers/shareFile.hpp"
#include "../../headers/threadSafety.hpp"

void *handle_distributive_systems(void *arg)
{
    TCP *socket = (TCP *)arg;

    OPEN_RECEIVE_FILE_CONNECTION = 1;

    socket->sendData(
        STATUS_MESSAGES[OPEN_SHAREFILE_CONNECTION],
        strlen(STATUS_MESSAGES[OPEN_SHAREFILE_CONNECTION]));

    int askClientShareFile = 1;
    
    while(true);

    OPEN_RECEIVE_FILE_CONNECTION = 0;

    delete socket;
    return NULL;
}