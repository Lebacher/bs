#include "../include/server.h"

#include "pthread.h"
#include "ring_buffer.h"
#include "stdlib.h"
#include "string.h"
#include "string.h"
#include "unistd.h"
#include <errno.h>
#include <netinet/ip.h>
#include <stdio.h>
#include <sys/socket.h>
#include <sys/types.h>

//Anzahl Clients
unsigned int    client_number_count = 0;
//RW Lock Variable
pthread_rwlock_t rw_lock = PTHREAD_RWLOCK_INITIALIZER;


int initialize_server()
{

    int server_socket;
    uint16_t portnumber = SERVER_PORT;
    struct sockaddr_in address;

    //Inititalisiere den Socket
    server_socket = socket(AF_INET, SOCK_STREAM, 0);

    if(server_socket == -1)
    {
        perror("initialize from socket is failed");
        return errno;
    }

    //erste Bytes im Speicher auf die adress zeigt
    memset(&address, 0, sizeof(address));

    //IP4 Protokoll
    address.sin_family = AF_INET;
    address.sin_port = htons(portnumber);
    address.sin_addr.s_addr = INADDR_ANY;

    //binde adress an server_socket
    if(bind(server_socket, &address, sizeof(address)) == -1)
    {
        perror("ERROR on binding");
        return errno;
    }

    //warte auf Antwort
    if(listen(server_socket, MAX_SOCKET_QUEUE) == -1)
    {
        perror("ERROR on listen");
        return errno;
    }

    return server_socket;
}

//-----------------------------------------------------------------------------

int handshake(int file_descriptor)
{

    //! Read message from client
    char    message_buffer[MAX_MESSAGE_LENGTH];
    ssize_t message_lenght = 0;
    if((message_lenght = read(file_descriptor, message_buffer, MAX_MESSAGE_LENGTH - 1)) < 1)
    {
        return -1;
    }
    message_buffer[MAX_MESSAGE_LENGTH - 1] = '\0'; //! Enforce NULL Terminated string
    printf("Handshake: %s\n", message_buffer);

    //! Reply message to client
    const char server_message[] = SERVER_HANDSHAKE_MSG;
    if(write(file_descriptor, server_message, sizeof(server_message)) < 0)
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

    //! get client number
    pthread_rwlock_wrlock(&rw_lock); //lock
    unsigned int client_number = client_number_count;
    client_number_count++;
    pthread_rwlock_unlock(&rw_lock); //unlock

    printf("adding reader no %d\n", client_number);

    //! get reader number for thread
    int thread_reader = ringbuffer_add_reader(client_number);

    //! handshake
    if(handshake(file_descriptor) < 0)
    {
        printf("handshake failed \n");
        ringbuffer_remove_reader(&thread_reader, file_descriptor);
        close(file_descriptor);
        return NULL;
    }

    void* buffer = malloc(sizeof(char) * (MAX_MESSAGE_LENGTH));
    if(buffer == NULL)
    {
        perror("malloc");
        ringbuffer_remove_reader(&thread_reader, file_descriptor);
        close(file_descriptor);
        return NULL;
    }
    //! main loop for each thread to continuesly read from file descriptor and handle input
    while(1)
    {

        //! clean the buffer
        memset(buffer, 0, MAX_MESSAGE_LENGTH);

        int   number_of_read_bytes = read(file_descriptor, buffer, MAX_MESSAGE_LENGTH - 1);
        char* message              = (char*)buffer;
        message[MAX_MESSAGE_LENGTH - 1]   = '\0';

        //! error handling for read
        if(number_of_read_bytes < 0)
        {
            ringbuffer_remove_reader(&thread_reader, client_number);
            perror("read");
            if(close(file_descriptor) < 0)
            {
                perror("close");
            }
            break;
        }
        else if(number_of_read_bytes == 0)
        {
            ringbuffer_remove_reader(&thread_reader, client_number);
            if(close(file_descriptor) < 0)
            {
                perror("close");
            }
            printf("closing connection\n");
            break;
        }
        //! handle clients input
        if(handle_input(client_number, message, file_descriptor, &thread_reader) != 0)
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

int handle_input(int client_number, char* input, int socket, int* current_reader_pointer)
{

    const char* error_message_1       = "r:invalid input: short message";
    const char* error_message_2       = "r:invalid input: unknown message type";
    const char* write_error_message   = "r:nack";
    const char* write_success_message = "r:ack";

    //! check message length
    if(sizeof(input) < 2 * sizeof(char))
    {

        if(write(socket, error_message_1, strlen(error_message_1) + 1) < 0)
        {
            perror("write");
            return -1;
        }
        return 0;
    }

    //! check first two chars of message
    char control_character = input[0];
    char delimiter         = input[1];
    if(delimiter != ':' || !(control_character == 'g' || control_character == 's'))
    {
        printf("invalid input\n");
        if(write(socket, error_message_2, strlen(error_message_2) + 1) < 0)
        {
            perror("write");
            return -1;
        }
        return 0;
    }

    char* message = ++input;
    message++;

    //! handle GET request
    if(control_character == 'g')
    {
        char buffer[MAX_MESSAGE_LENGTH - 2];

        ringbuffer_read(current_reader_pointer, buffer, client_number);

        printf("client %d read: %s length %zu\n", client_number, buffer, strlen(buffer));

        char message[MAX_MESSAGE_LENGTH] = "r:";

        strcat(message, buffer);

        if(write(socket, message, strlen(message) + 1) < 0)
        {
            perror("write");
            return (-1);
        }
        //! handle set request
    }
    //! handle SET request
    else if(control_character == 's')
    {

        //! write in reingbuffer
        int write_ack = ringbuffer_write(message);

        //! write failed
        if(write_ack != 0)
        {
            if(write(socket, write_error_message, strlen(write_error_message) + 1) < 0)
            {
                perror("write");
                return -1;
            }
            printf("client %d write failed\n", client_number);
        }
        //! write success
        else
        {
            if(write(socket, write_success_message, strlen(write_success_message) + 1) < 0)
            {
                perror("write");
                return -1;
            }
            printf("client %d write: %s length %zu\n", client_number, message, strlen(message));
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
    struct sockaddr_in client;
        socklen_t len = sizeof( client );

        pthread_t my_thread_id;
        pthread_attr_t my_attributes;

        while(1)
        {
            //init neuen socket + verbinde
            int accept_return = accept(socket_number, &client, &len);

            //accept schlaegt fehl
            if( accept_return == -1)
            {
                perror("ERROR on accept");
                continue;
            }

            //allocate Speicher
            int* accept_pointer = malloc(sizeof(int));
            *accept_pointer = accept_return;

            //Init das attribute Objekt
            if(pthread_attr_init(&my_attributes) == -1)
            {
                printf("ERROR pthread_attr_init");
                //springe zum Kontrollpunkt
                continue;
            }

            //TODO: Kommentar einfügen
            if(pthread_attr_setdetachstate(&my_attributes, PTHREAD_CREATE_DETACHED) == -1)
            {
                printf("ERROR detachstate");
                //springe zum Kontrollpunkt
                continue;
            }

            //erzeuge neuen Thread
            if(pthread_create(&my_thread_id, &my_attributes, &handle_connection, accept_pointer) == -1)
            {
                printf("ERROR can`t create a pthread");
                //springe zum Kontrollpunkt
                continue;
            }
        }
}
