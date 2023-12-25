#include "../include/server.h"
#include "../include/ring_buffer.h"

#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <netinet/ip.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>

uint16_t get_port_number() {
    u_int16_t port = 0;
    const char* username = getenv("USER");
    if (username == NULL)
    {
        return 31026;
    }
    char c;
    while ((c = *username) != '\0') {
        port = (port << 1) + port +(u_int16_t)c;
        username++;
    }
    return 31026 + (port % 4096);
}


unsigned int    client_number_count = 0;

//! HINT: maybe global synchronization variables are needed
//! HINT: but try to keep the critical region as small as possible
pthread_mutex_t server_mutex = PTHREAD_MUTEX_INITIALIZER;


int initialize_server()
{

    //! implement this function

	int serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket < 0)
    {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in serv_addr;
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(SERVER_PORT);

    if (inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0)
    {
        perror("Inet_pton failed");
        exit(EXIT_FAILURE);
    }

    if (bind(serverSocket, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0)
    {
        perror("Bind failed");
        exit(EXIT_FAILURE);
    }
	
	if (listen(serverSocket, MAX_SOCKET_QUEUE) < 0)
    {
        perror("Listen failed");
        exit(EXIT_FAILURE);
    }

    return serverSocket;
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
    //! HINT: Synchronization is needed in this function

    int file_descriptor = *(int*)socket;
    free(socket); // can also be freed when thread exits

    //! get client number
    if (pthread_mutex_lock(&server_mutex) != 0) 
	{
        perror("Mutex lock failed.");
        exit(EXIT_FAILURE);
    }
	
    unsigned int client_number = client_number_count;
    client_number_count++;
    
	if (pthread_mutex_unlock(&server_mutex) != 0) 
	{
        perror("Mutex unlock failed.");
        exit(EXIT_FAILURE);
    }
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
            perror("read");
            break;
        }
        else if(number_of_read_bytes == 0)
        {
            printf("closing connection\n");
            break;
        }
        //! handle clients input
        if(handle_input(client_number, message, file_descriptor, &thread_reader) != 0)
        {
            break;
        }
    }
    ringbuffer_remove_reader(&thread_reader, client_number);
    if(close(file_descriptor) < 0)
    {
        perror("close");
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
    if(strlen(input) < 2)
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
	while (1){
        int* client_socket = malloc(sizeof(int));
		
		if (client_socket == NULL) 
		{
            perror("malloc failed.");
            exit(EXIT_FAILURE);
        }
		
        struct sockaddr_in client_address;
        socklen_t client_address_length = sizeof(client_address);
		
        *client_socket = accept(socket_number, 
							(struct sockaddr*)&client_address, 
							&client_address_length); 

		if (*client_socket < 0) 
		{
            perror("Accept failed");
            free(client_socket);
            exit(EXIT_FAILURE);
        }
		
        pthread_t new_worker_thread;
        pthread_attr_t new_thread_attributes;
		
		if (pthread_attr_init(&new_thread_attributes) != 0) 
		{
            perror("Pthread_attr_init failed");
            exit(EXIT_FAILURE);
        }
		
        if (pthread_attr_setdetachstate(&new_thread_attributes,
										PTHREAD_CREATE_DETACHED) != 0) 
		{
            perror("Pthread_attr_setdetachstate failed");
            exit(EXIT_FAILURE);
        }
	
        if (pthread_create(&new_worker_thread, &new_thread_attributes, 
						   handle_connection, client_socket) != 0) 
		{
            perror("Pthread_create failed");
			exit(EXIT_FAILURE);
        }
        
		if (pthread_attr_destroy(&new_thread_attributes) != 0) 
		{
            perror("Pthread_attr_destroy failed");
            exit(EXIT_FAILURE);
        }
    }	
}
