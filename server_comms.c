#include <stdio.h>
#include <stdbool.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include "server_comms.h"
#include "image_server.h"

/*
 Reads a packet which consits of:
    a 4 byte length - X
    X bytes of data
 The data is returned and should be freed by the caller
 */
char* read_client_packet (int sock)
{
    char* buffer;
    uint32_t buffer_length;
    
    if (read (sock, &buffer_length, 4) != 4)
    {
        printf("Length packet was not 4 bytes long\n");
        return NULL;
    }
    
    // allocatate a buffer to store in coming data
    buffer = (char*) malloc(buffer_length);
    
    if (buffer == NULL)
    {
        printf("Failed to allocate memory for buffer of size 0x%x\n", buffer_length);
        return NULL;
    }
    
    if (buffer_length != read (sock, buffer, buffer_length))
    {
        printf("Failed to read in client data correctly");
        free(buffer);
        return NULL;
    }
    
    return buffer;
}

/*
 Create a socket for the server to listen on
 */
int create_server_socket (uint16_t port)
{
    struct sockaddr_in name;
    /* Create the socket. */
    int sock = socket (PF_INET, SOCK_STREAM, 0);
    if (sock < 0)
    {
        perror ("socket");
        exit (EXIT_FAILURE);
    }
    
    /* Give the socket a name. */
    name.sin_family = AF_INET;
    name.sin_port = htons (port);
    name.sin_addr.s_addr = htonl (INADDR_LOOPBACK); // Only listen on localhost
    
    // Make it quick to restart
    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &(int){ 1 }, sizeof(int));
               
    if (bind (sock, (struct sockaddr *) &name, sizeof (name)) < 0)
    {
        perror ("bind");
        exit (EXIT_FAILURE);
    }
    
    if (listen (sock, 1) < 0)
    {
        perror ("listen");
        exit (EXIT_FAILURE);
    }
    
    return sock;
}

/*
 Run the server
 */
void loop_forever(int server_sock)
{
    char* incoming_data = NULL;
    struct sockaddr_in clientname;
    socklen_t size;
    bool should_disconnect = false;;
    
    while (1)
    {
        // wait for a new client
        int client = accept (server_sock, (struct sockaddr *) &clientname, &size);
        
        // Split the client in to a new process
        // So that we can handle multiple clients and continue if any crash
//        if (fork() == 0)
        {
            // Say hello to the new client
            display_welcome(client);
            
            do
            {
                // Loop, reading there commands until they quit or there is an error
                incoming_data = read_client_packet(client);
                if (incoming_data)
                {
                    should_disconnect = handle_command(client, incoming_data);
                    free(incoming_data);
                }
                else
                {
                    // they quit or was an error
                    should_disconnect = true;
                }
            } while (!should_disconnect);
            
            close(client);
        }
    }

}

int main (void)
{
    int port = 5555;
    int server_sock = create_server_socket(port);
    set_server_defaults();
    printf("Server started, listening on localhost port %d\n", port);
    loop_forever(server_sock);
}
