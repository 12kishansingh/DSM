#include "../headers/serverService.h"

CommandMapServer server_dispatch_table[] = {
    {"status", handle_status_check},
    {"connect", handle_connect_server},
    {"shareFile", handle_share_file_server},
    {NULL, NULL} // Sentinel value to mark the end
};
