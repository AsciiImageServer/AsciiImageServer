
#ifndef server_comms_h
#define server_comms_h

#include <stdio.h>

#define log_to_user(log, ...) dprintf(sock, log, ##__VA_ARGS__);

#endif /* server_comms_h */
