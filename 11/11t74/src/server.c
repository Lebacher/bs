#include "../include/server.h"

#include "ring_buffer.h"
#include <arpa/inet.h>
#include <errno.h>
#include <netinet/ip.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
// needed for inet_addr

uint16_t get_port_number()
{
    u_int16_t port       = 0;
    const char* username = getenv("USER");
    if (username == NULL)
    {
        return 31026;
    }
    char c;
    while ((c = *username) != '\0')
    {
        port = (port << 1) + port + (u_int16_t)c;
        username++;
    }
    return 31026 + (port % 4096);
}

unsigned int client_number_count = 0;
pthread_mutex_t server_mutex     = PTHREAD_MUTEX_INITIALIZER;

int initialize_server()
{
    // Define connection properties
    struct sockaddr_in socketaddress;
    socketaddress.sin_addr.s_addr = inet_addr("127.0.0.1");
    socketaddress.sin_family      = AF_INET;
    socketaddress.sin_port        = htons(SERVER_PORT);

    // Create a socket for the connection
    int socket_ret = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_ret == -1)
    {
        // Exit if it returns an unvalid return value
        perror("socket");
        exit(1);
    }

    // Try to bind the socket to our address
    if (bind(socket_ret, &socketaddress, sizeof(socketaddress)) < 0)
    {
        // Exit if it returns an unvalid return value
        perror("Could not bind socket to the address/port");
        exit(1);
    }

    return socket_ret;
}

//-----------------------------------------------------------------------------

int handshake(int file_descriptor)
{

    //! Read message from client
    char message_buffer[MAX_MESSAGE_LENGTH];
    ssize_t message_lenght = 0;
    if ((message_lenght =
             read(file_descriptor, message_buffer, MAX_MESSAGE_LENGTH - 1)) < 1)
    {
        return -1;
    }
    message_buffer[MAX_MESSAGE_LENGTH - 1] =
        '\0'; //! Enforce NULL Terminated string
    printf("Handshake: %s\n", message_buffer);

    //! Reply message to client
    const char server_message[] = SERVER_HANDSHAKE_MSG;
    if (write(file_descriptor, server_message, sizeof(server_message)) < 0)
    {
        return -1;
    }

    return 0;
}

//-----------------------------------------------------------------------------

void* handle_connection(void* socket)
{

    int file_descriptor = *(int*)socket;

    free(socket); // can also be freed when thread exits

    // Muxtex lock to get correct client number
    pthread_mutex_lock(&server_mutex);

    //! get client number
    unsigned int client_number = client_number_count;
    client_number_count++;

    // Mutex unlock
    pthread_mutex_unlock(&server_mutex);

    printf("adding reader no %d\n", client_number);

    //! get reader number for thread
    int thread_reader = ringbuffer_add_reader(client_number);

    //! handshake
    if (handshake(file_descriptor) < 0)
    {
        printf("handshake failed \n");
        ringbuffer_remove_reader(&thread_reader, file_descriptor);
        close(file_descriptor);
        return NULL;
    }

    void* buffer = malloc(sizeof(char) * (MAX_MESSAGE_LENGTH));
    if (buffer == NULL)
    {
        perror("malloc");
        ringbuffer_remove_reader(&thread_reader, file_descriptor);
        close(file_descriptor);
        return NULL;
    }
    //! main loop for each thread to continuesly read from file descriptor and
    //! handle input
    while (1)
    {

        //! clean the buffer
        memset(buffer, 0, MAX_MESSAGE_LENGTH);

        int number_of_read_bytes =
            read(file_descriptor, buffer, MAX_MESSAGE_LENGTH - 1);
        char* message                   = (char*)buffer;
        message[MAX_MESSAGE_LENGTH - 1] = '\0';

        //! error handling for read
        if (number_of_read_bytes < 0)
        {
            ringbuffer_remove_reader(&thread_reader, client_number);
            perror("read");
            if (close(file_descriptor) < 0)
            {
                perror("close");
            }
            break;
        }
        else if (number_of_read_bytes == 0)
        {
            ringbuffer_remove_reader(&thread_reader, client_number);
            if (close(file_descriptor) < 0)
            {
                perror("close");
            }
            printf("closing connection\n");
            break;
        }
        //! handle clients input
        if (handle_input(client_number, message, file_descriptor,
                         &thread_reader) != 0)
        {
            ringbuffer_remove_reader(&thread_reader, client_number);
            close(file_descriptor);
            break;
        }
    }
    free(buffer);
    return NULL;
}

//-----------------------------------------------------------------------------

int handle_input(int client_number, char* input, int socket,
                 int* current_reader_pointer)
{

    const char* error_message_1       = "r:invalid input: short message";
    const char* error_message_2       = "r:invalid input: unknown message type";
    const char* write_error_message   = "r:nack";
    const char* read_error_message    = "r:nack";
    const char* write_success_message = "r:ack";

    //! check message length
    if (sizeof(input) < 2 * sizeof(char))
    {

        if (write(socket, error_message_1, strlen(error_message_1) + 1) < 0)
        {
            perror("write");
            return -1;
        }
        return 0;
    }

    //! check first two chars of message
    char control_character = input[0];
    char delimiter         = input[1];
    if (delimiter != ':' ||
        !(control_character == 'g' || control_character == 's'))
    {
        printf("invalid input\n");
        if (write(socket, error_message_2, strlen(error_message_2) + 1) < 0)
        {
            perror("write");
            return -1;
        }
        return 0;
    }

    char* message = ++input;
    message++;

    //! handle GET request
    if (control_character == 'g')
    {
        char buffer[MAX_MESSAGE_LENGTH - 2];

        ringbuffer_read(current_reader_pointer, buffer, client_number);

        // Required to pass test 1: send r:nack if no prior messages exist
        if (strlen(buffer) == 0)
        {
            if (write(socket, read_error_message,
                      strlen(read_error_message) + 1) < 0)
            {
                perror("write");
                return -1;
            }
        }

        printf("client %d read: %s length %zu\n", client_number, buffer,
               strlen(buffer));

        char message[MAX_MESSAGE_LENGTH] = "r:";

        strcat(message, buffer);

        if (write(socket, message, strlen(message) + 1) < 0)
        {
            perror("write");
            return (-1);
        }
        //! handle set request
    }
    //! handle SET request
    else if (control_character == 's')
    {
        //! write in reingbuffer
        int write_ack = ringbuffer_write(message);

        //! write failed
        if (write_ack != 0)
        {

            if (write(socket, write_error_message,
                      strlen(write_error_message) + 1) < 0)
            {
                perror("write");
                return -1;
            }
            printf("client %d write failed\n", client_number);
        }
        //! write success
        else
        {
            if (write(socket, write_success_message,
                      strlen(write_success_message) + 1) < 0)
            {
                perror("write");
                return -1;
            }
            printf("client %d write: %s length %zu\n", client_number, message,
                   strlen(message));
        }
    }
    else
    {
        printf("an unknown error occured\n");
        return (-1);
    }

    return 0;
}

//-----------------------------------------------------------------------------

void accept_connections(int socket_number)
{

    // Listen for a new client on the socket
    if (listen(socket_number, MAX_SOCKET_QUEUE) == -1)
    {
        perror("Failed to listen on a port");
        exit(1);
    }

    while (1)
    {

        // Reserve space
        int* client_connection = malloc(sizeof(int));
        if (client_connection == NULL)
        {
            perror("malloc");
            exit(EXIT_FAILURE);
        }
        // Accept clients connection
        *client_connection = accept(socket_number, NULL, NULL);
        if (*client_connection == -1)
        {
            perror("Error accepting a clients connection");
            free(client_connection);
            continue;
        }

        // Prepare+create pthread with detached attribute
        pthread_t new_client_thread;
        pthread_attr_t thread_attributes;
        if (pthread_attr_init(&thread_attributes) != 0)
        {
            // Could not initialize attribute variable
            free(client_connection);
            perror("init pthread");
            continue;
        }
        if (pthread_attr_setdetachstate(&thread_attributes,
                                        PTHREAD_CREATE_DETACHED) != 0)
        {
            // Failed to set detachstate attribute
            free(client_connection);
            perror("setdetachstate pthread");
            continue;
        }

        // Create the client connection thread
        if (pthread_create(&new_client_thread, &thread_attributes,
                           handle_connection, client_connection) != 0)
        {
            perror("Failed creating a thread for a new client connection");
            exit(1);
        }
    }
}
