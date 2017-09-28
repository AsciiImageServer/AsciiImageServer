#ifndef image_server_h
#define image_server_h

#include <stdio.h>

void set_server_defaults(void);
void display_welcome(int sock);
bool handle_command(int sock, char* data);

#endif /* image_server_h */
