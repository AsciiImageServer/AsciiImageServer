#include <errno.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "image_server.h"
#include "server_comms.h"
#include "default_images.h"

#define MAX_IMAGE_COUNT 100
#define HASH_LENGTH 256

typedef struct
{
    long is_restricted;
    char* ascii_image;
    char caption[0x60];
} StoredImage;

long total_image_count = 0;
long is_user_logged_in = false;
char server_password[0x60];
StoredImage images[MAX_IMAGE_COUNT];


void add_image_to_server(int f, char* ascii, char* caption, bool is_restricted);

/*
 Keep the server really secure by randomisng the password and not telling it to anyone.
 To gain authorisation, clients will need to prove their worth by divining the password
 */
void generate_random_password()
{
    unsigned int length = sizeof(server_password);
    // For security, add some hard to guess characters at the start
    server_password[0] = '_';
    server_password[1] = '1';
    server_password[2] = '!';
    server_password[3] = '0';
    for (int i = 4; i < length-1; i++)
    {
        server_password[i] = 'a' + (rand() % 26);
    }
    
    // null terminate the password
    server_password[length-1] = '\0';
}

/*
 Setup a fresh server
 */
void set_server_defaults()
{
    generate_random_password();
    
    // Add the default images
    add_image_to_server(-1, image1, "Access Restricted", true);
    add_image_to_server(-1, image2, "", false);
}

/*
 Adds a new image to the server
 */
void add_image_to_server(int sock, char* ascii, char* caption, bool is_restricted)
{
    if (total_image_count < MAX_IMAGE_COUNT)
    {
        strcpy(images[total_image_count].caption, caption);
        images[total_image_count].ascii_image = ascii;
        images[total_image_count].is_restricted = is_restricted;
        
        log_to_user("Your image was added at index %ld\n", total_image_count);
        
        total_image_count++;
    }
    else
    {
        log_to_user("Sorry, the image server is full\n");
    }
}

/*
 Get the width of an image (number of characters on first line)
 */
unsigned int get_ascii_image_width(char* data)
{
    unsigned int width = 0;
    char* pos = data;
    while (*pos != '\n')
    {
        pos ++;
        width++;
    }
    return width;
}

/*
 Get the height of an image (number of new lines)
 */
unsigned int get_ascii_image_height(char* data)
{
    unsigned int height = 0;
    char* pos = data;
    while (*pos != '\0')
    {
        pos++;
        if (*pos == '\n')
        {
            height++;
        }
    }
    return height;
}

/*
 Avoid clogging the server with duplicate images.
 Checks if the image will be a unique addition to the server
 */
bool is_image_on_server(int sock, char* ascii, char* hash_bytes)
{
    char fast_buffer[100];
    char* comparison_buffer;
    
    // Calculate the dimensions of the image so that we know how much space we need to store it
    unsigned int width = get_ascii_image_width(ascii);
    unsigned int height = get_ascii_image_height(ascii);
    unsigned int size_to_check = (height * width) + HASH_LENGTH;
    
    log_to_user("Checking image is not on the server already...\n");
    log_to_user("Width %d and height %d (size %d)\n", width, height, size_to_check);
    
    if (size_to_check < 100)
    {
        // small image, put it on the stack instead of on the heap
        // Its quicker this way.
        comparison_buffer = fast_buffer;
    }
    else
    {
        comparison_buffer = malloc(size_to_check);
    }
    
    // Copy the image to the comparision buffer
    strcpy(comparison_buffer, ascii);
    // Now add the hash on the end. The hash is simply stored after the image in memory
    // At some point I'll compare the hashes instead of doing a strcmp below but
    // I've not got around to it yet.
    memcpy(comparison_buffer + strlen(ascii), hash_bytes, HASH_LENGTH);
    
    // Check if it matches any image on the server already
    for (int i = 0; i < total_image_count; i++)
    {
        if (strcmp(comparison_buffer, images[i].ascii_image) == 0)
        {
            return true;
        }
    }
    return false;
}

/*
 Get image at specified index
 */
void handle_get_command(int sock, char* data)
{
    // Try to get the requested index
    errno = 0;
    int image_index = atoi(data);
    
    if (image_index == 0 && errno == EINVAL)
    {
        log_to_user("Failed to convert '%s' to a number 0x%x\n", data, strlen(data));
        return;
    }
    
    log_to_user("Requested image at index %d\n", image_index);
    
    // Validate the user isnt requesting an image outside of the available range.
    if (image_index < total_image_count)
    {
        // Does the user need to loggined to access this image?
        // If so, are they logged in?
        if (images[image_index].is_restricted && !is_user_logged_in)
        {
            // Not authorised to see this just now
            // Let them see the captain so that they know what they are missing.
            log_to_user("Access to the image with caption '%s' is restricted\n", images[image_index].caption);
            log_to_user("You need to be signed in to view it\n");
        }
        else
        {
            log_to_user("Your requested image:\n\n");
            if (strlen(images[image_index].caption) > 0)
            {
                // There is a caption associated with this image, print that
                log_to_user(images[image_index].caption);
            }
        
            // Print the image with the caption
            log_to_user("\n%s\n\n", images[image_index].ascii_image);
        }
    }
    else
    {
        log_to_user("Sorry, you asked for the image at index %d\n", image_index)
        log_to_user("but there are only %ld images on the server. (indexing starts at 0)\n", total_image_count);
    }
}

/*
 Allow the user to attempt to login
 */
void handle_login_command(int sock, char* data)
{
    if (strcmp(server_password, data) == 0)
    {
        log_to_user("Log in successfull\n");
        is_user_logged_in = true;
    }
    else
    {
        log_to_user("Log in unsuccessfull\n");
        is_user_logged_in = false;
    }
}

/*
 Add a new image to the server
 */
void handle_add_command(int sock, char* data)
{
    // Read out 1 byte of is restricted
    // Read out null terminated caption
    // Read out null terminated ascii
    bool is_restricted = data[0];
    char* caption = strdup(data+1);
    char* ascii = strdup(data + 1 + strlen(caption) + 1);
    char* hash_bytes = data + 1 + strlen(caption) + 1 + strlen(ascii) + 1;
    
    // Only add new images to the server
    if (is_image_on_server(sock, ascii, hash_bytes))
    {
        log_to_user("Not adding image %s, it is already present\n", caption);
        free(caption);
        free(ascii);
    }
    else
    {
        add_image_to_server(sock, ascii, caption, is_restricted);
        free(caption);
    }
}

/*
 Handle each command
 */
bool handle_command(int sock, char* data)
{
    bool should_disconnect = false;
    switch (data[0])
    {
        case 'c':
            // Get count of images on server
            log_to_user("There are %ld images on the server\n", total_image_count);
            break;
            
        case 'g':
            // get an image from the server
            handle_get_command(sock, data + 1);
            break;
            
        case 'l':
            // login
            handle_login_command(sock, data + 1);
            break;
            
        case 'a':
            // add a new image to the server
            handle_add_command(sock, data + 1);
            break;
        
        case 'q':
            // add a new image to the server
            log_to_user("Goodbye.\n");
            should_disconnect = true;
            break;
            
        default:
            log_to_user("Unrecognised command %c\n", data[0]);
            break;
    }
    
    return should_disconnect;
}


/*
 Display welcome and warning message
 */
void display_welcome(int sock)
{
    log_to_user("Welcome to the image server\n");
    log_to_user("Available commands:\n");
    log_to_user("\t%-40s - %s\n", "Command", "Description");
    log_to_user("\t%-40s - %s\n", "q", "Disconnect.");
    log_to_user("\t%-40s - %s\n", "c", "Get the number of images on the server.");
    log_to_user("\t%-40s - %s\n", "g<num>", "Get the image at index <num>.");
    log_to_user("\t%-40s - %s\n", "l<password>", "Login with the specified null terminated password.");
    log_to_user("\t%-40s - %s\n", "a<is_restricted><caption><imageascii><hash>", "Add a new image to the server.");
    log_to_user("\t%-40s - %s\n", "", "<is_restricted>\t1 byte, 0 if false, 1 if true.");
    log_to_user("\t%-40s - %s\n", "", "<caption>\t\tNull terminated caption string.");
    log_to_user("\t%-40s - %s\n", "", "<imageascii>\t\tNull terminated ascii image data.");
    log_to_user("\t%-40s - %s\n", "", "<hash>\t\t256 byte hash of the image.");
    
    
    log_to_user("\n\n");
    log_to_user("Please do not attempt to access restricted images without correct authorisation.\n");
}
