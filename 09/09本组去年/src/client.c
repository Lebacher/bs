#include "client.h"
#include <arpa/inet.h>


pthread_t reading_thread;
int client_socket;
// Add your global variables here
pthread_mutex_t lock;
void connect_to_server()
{
    // write your code here
    client_socket = socket(AF_INET, SOCK_STREAM, 0);
    // socket return -1 for error
    if (client_socket == -1)
    {
        perror("Could not create socket");
    }
    else
    {
        printf("socket error");
    }

    struct sockaddr_in server_addr;

    server_addr.sin_family      = AF_INET;
    server_addr.sin_port        = htons(SERVER_PORT);
    if (inet_pton(AF_INET, "127.0.0.1", &server_addr.sin_addr) <= 0)
    {
        perror("\n inet_pton error \n");
        exit_client(1);
    }
    // Create connection with socket
    if (connect(client_socket, (struct sockaddr*)&server_addr,sizeof(server_addr)) == -1)
    {
        perror("connection to server is successful:");
    }
    else
    {
        printf("connection failed");
    }
}

//-------------------------------------------------------------
void handshake()
{
    char handshake_message[MAX_MESSAGE_LENGTH];
    snprintf(handshake_message, MAX_MESSAGE_LENGTH, "Hello, I'm the client");
    int m_send = send(client_socket, handshake_message, MAX_MESSAGE_LENGTH, 0);
    int m_recv = recv(client_socket, handshake_message, MAX_MESSAGE_LENGTH, 0);
    // Detect that there was an error sending the message
    if (m_send == -1)
    {
        perror(NULL);
        exit_client(1);
    }
    // Detecting an error in receiving the message
    if (m_recv == -1)
    {
        perror(NULL);
        exit_client(1);
    }
    print_reply("Hello, I'm the server");
}

//-------------------------------------------------------------

void close_connection()
{
    // write your code here
}

//-------------------------------------------------------------

void send_message()
{

    // write your code here
     char message_input[MAX_USER_INPUT];
     char *user_inputs;
    while(1)
    {
        user_inputs  = prompt_user_input(message_input, MAX_USER_INPUT);

        //See if the input is empty
        if(user_inputs[0] == '\n')
        {
            //then call get
            get_request();
        }
        else
        {
            //Otherwise call set with input as argument
            set_request(user_inputs);
        }

    }
}

//-------------------------------------------------------------

void set_request(char* message)
{
    // write your code here
    char answer[MAX_MESSAGE_LENGTH];
    char msg_data[MAX_MESSAGE_LENGTH] = "s:";
    strcat(msg_data, message);
    switch (pthread_mutex_lock(&lock))
    {
        case 0:
            if (send(client_socket, msg_data, strlen(msg_data), 0) == -1)
            {
                perror(NULL);
                exit_client(1);
            }
            else
            {
            if (recv(client_socket, answer, MAX_MESSAGE_LENGTH, 0)==-1)
            {
                    prompt_error();
            }
            }
            if (pthread_mutex_unlock(&lock) != 0)
            {
                perror(NULL);
                exit_client(1);
            }
            break;
            default:
            perror(NULL);
            exit_client(1);
            break;
    }
    if (strcmp(answer, "r:nack") == 0)
    {
        prompt_error();
    }
    else if (strcmp(answer, "r:ack") != 0)
    {
        prompt_error(answer);
        exit_client(1);
    }
}

//-------------------------------------------------------------

void get_request()
{
    // write your code here
    char answer [MAX_MESSAGE_LENGTH];
    char *msg_data = "g:";

    switch (pthread_mutex_lock(&lock))
    {
        case 0:
        if (send(client_socket, msg_data, strlen(msg_data), 0) == -1)
        {
            perror(NULL);
            exit_client(1);
        }
        if (recv(client_socket, answer, MAX_MESSAGE_LENGTH, 0) == -1)
        {
            perror(NULL);
            exit_client(1);
        }
        else if (strcmp("r:nack", answer) != 0)
        {
            print_reply(answer);
        }
        if (pthread_mutex_unlock(&lock) != 0)
        {
            perror(NULL);
            exit_client(1);
        }
        break;
        default:
        perror(NULL);
        exit_client(1);
        break;
    }

}

//-------------------------------------------------------------

void* read_continously(void* unused)
{
    (void)unused; // Mark variable as used for the compiler :-)

    // write your code here
     while (1)
    {
        get_request();
        sleep(READING_INTERVAL);
    }

    // this method should not return so dont care about return value
    return NULL;
}

//-------------------------------------------------------------

void start_reader_thread()
{
    // write your code here
     pthread_attr_t thread_attribute;
     pthread_attr_init(&thread_attribute);
     int error_no = pthread_create(&reading_thread, NULL, read_continously, NULL);
    if (error_no != 0)
    {
         perror(NULL);
        exit_client(1);
        
    }
    pthread_attr_destroy(&thread_attribute);
}
