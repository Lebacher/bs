#include "client.h"
#include <arpa/inet.h>
// needed for inet_addr

pthread_t reading_thread;
int client_socket;
// Add your global variables here

void connect_to_server()
{
    // Define connection properties
    struct sockaddr_in socketaddress;
    socketaddress.sin_addr.s_addr = inet_addr("127.0.0.1");
    socketaddress.sin_family      = AF_INET;
    socketaddress.sin_port        = htons(SERVER_PORT);

    // Create a socket for the connection
    client_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (client_socket == -1)
    {
        // Exit if we receive an unvalid return value
        perror("Could not create socket");
        exit_client(1);
    }
    if (connect(client_socket, (struct sockaddr*)&socketaddress,
                sizeof(socketaddress)) == -1)
    {
        // Exit if the connecton failed
        perror("Could not connect");
        exit_client(1);
    }
}

//-------------------------------------------------------------

void handshake()
{
    char* message = "Hello, I'm the client";
    if (send(client_socket, message, strlen(message), 0) == -1)
    {
        // Exit if we can't send a message to the server
        perror("Could not send handshake");
        exit_client(1);
    }
    char buffer[MAX_MESSAGE_LENGTH];
    if (recv(client_socket, buffer, MAX_MESSAGE_LENGTH, 0) == -1)
    {
        // Exit if we don't receive a valid response from the server
        perror("Could not receive handshake");
        exit_client(1);
    }
    // Print handshake response
    printf("%s", buffer);
}

//-------------------------------------------------------------

void send_message()
{
    while (1)
    {
        char buffer[MAX_MESSAGE_LENGTH];
        // Read user input
        char* input_buffer = prompt_user_input(buffer, MAX_USER_INPUT);
        if (input_buffer[0] == '\n')
        {
            // Empty input
            get_request();
        }
        else
        {
            // Input not empty
            set_request(input_buffer);
        }
    }
}

//-------------------------------------------------------------

void set_request(char* message)
{
    char msg_to_send[MAX_MESSAGE_LENGTH] = "s:";
    strcat(msg_to_send, message);

    if (send(client_socket, &msg_to_send, strlen(msg_to_send), 0) == -1)
    {
        // Exit if sending a message is failing
        perror("Could not send message");
        exit_client(1);
    }

    char receive_buffer[MAX_MESSAGE_LENGTH];
    if (recv(client_socket, &receive_buffer, MAX_MESSAGE_LENGTH, 0) == -1)
    {
        prompt_error();
        exit_client(1);
    }

    if ((strcmp(msg_to_send, "r:nack") == 0) ||
        (strcmp(msg_to_send, "r:ack") == 0))
    {
        perror(msg_to_send);
        exit_client(1);
    }
}
//}

//-------------------------------------------------------------

void get_request()
{
    if (send(client_socket, "g:", 2, 0) == -1)
    {
        perror("Could not send get request");
        exit_client(1);
    }

    char receive_buffer[MAX_MESSAGE_LENGTH];
    if (recv(client_socket, receive_buffer, MAX_MESSAGE_LENGTH, 0) == -1)
    {
        perror("Could not receive server response to a g: message");
        exit_client(1);
    }

    if (strcmp(receive_buffer, "r:nack") == 0)
    {
        perror("Received r:nack in response to g: message");
        exit_client(1);
    }
    else
    {
        // Received message
        // Remove r: from the beginning
        memmove(receive_buffer, receive_buffer + 2, strlen(receive_buffer));
        // Show message to user
        print_reply((char*)&receive_buffer);
    }
}

//-------------------------------------------------------------

void* read_continously(void* unused)
{
    (void)unused; // Mark variable as used for the compiler :-)

    // write your code here

    // this method should not return so dont care about return value
    return NULL;
}

//-------------------------------------------------------------

void start_reader_thread()
{
    // write your code here
}
