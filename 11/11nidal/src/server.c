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

//! HINT: maybe global synchronization variables are needed
//! HINT: but try to keep the critical region as small as possible

int clinet_socket;
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

int initialize_server()
{
    //  create a Socket
    struct sockaddr_in serv_addr;
    clinet_socket = socket(AF_INET, SOCK_STREAM, 0);
    // socket return -1 for error
    if (clinet_socket < 0)
    {
        perror("ERROR opening socket");
        exit(1);    // FB exit(EXIT_FAILURE) ist äquivalent und aussagekräftiger und verbessert daher das Verständnis des Codes
    }
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port   = htons(SERVER_PORT);
    if (inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0)
    {
        perror("\n inet_pton error \n");    // FB Da perror an euren String ein ": <error message>\n" anfügt, ist der Zeilenumbruch hier ungünstig.
        exit(1);    // FB exit(EXIT_FAILURE) ist äquivalent und aussagekräftiger und verbessert daher das Verständnis des Codes
    }
    // Detected error in connect
    if (bind(clinet_socket, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) <
        0)  // FB Fügt bitte keinen Zeilenumbruch wegen 2 Zeichen ein.
    {
        perror("\n connection Error \n");   // FB Da perror an euren String ein ": <error message>\n" anfügt, ist der Zeilenumbruch hier ungünstig.
        exit(1);    // FB exit(EXIT_FAILURE) ist äquivalent und aussagekräftiger und verbessert daher das Verständnis des Codes
    }
    listen(clinet_socket, 5);
    return clinet_socket;
    // done...
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
    //! HINT: Synchronization is needed in this function

    int file_descriptor = *(int*)socket;
    free(socket); // can also be freed when thread exits

    //! get client number
    unsigned int client_number;
    if (pthread_mutex_lock(&lock) == 0)
    {
        client_number = client_number_count;
        client_number_count++;
        if (pthread_mutex_unlock(&lock) != 0)
        {
            perror("unlock Error\n");   // FB Da perror an euren String ein ": <error message>\n" anfügt, ist der Zeilenumbruch hier ungünstig.
            exit(1);    // FB exit(EXIT_FAILURE) ist äquivalent und aussagekräftiger und verbessert daher das Verständnis des Codes
        }
    }
    else
    {
        perror("lock Error\n"); // FB Da perror an euren String ein ": <error message>\n" anfügt, ist der Zeilenumbruch hier ungünstig.
        exit(1);    // FB exit(EXIT_FAILURE) ist äquivalent und aussagekräftiger und verbessert daher das Verständnis des Codes
    }
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
    //! implement this function
    while (1)
    {
        socklen_t clilen;   // FB Variablennamen sollen nicht abgekürzt werden -1P
        struct sockaddr_in cli_addr;
        int* newsockfd;
        newsockfd = malloc(sizeof(int));    // FB Hier fehlt eine Fehlerbehandlung -1P
        clilen    = sizeof(cli_addr);
        *newsockfd =
            accept(socket_number, (struct sockaddr*)&cli_addr, &clilen);    // FB Hier fehlt eine Fehlerbehandlung -1P
        pthread_t new_worker;
        pthread_attr_t thread_attr;
        pthread_attr_init(&thread_attr);    // FB Hier fehlt eine Fehlerbehandlung -1P
        pthread_attr_setdetachstate(&thread_attr,PTHREAD_CREATE_DETACHED);  // FB Hier fehlt eine Fehlerbehandlung
        if (pthread_create(&new_worker, &thread_attr, handle_connection,
                           newsockfd) != 0)
        {
            perror("creation of thread\n"); // FB Da perror an euren String ein ": <error message>\n" anfügt, ist der Zeilenumbruch hier ungünstig.
        }
        // FB Hier fehlt ein pthread_attr_destroy
    }
}
