#include "client.h"
#include <arpa/inet.h>

pthread_t reading_thread;
int client_socket;
// Add your global variables here

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

void connect_to_server()
{
    // write your code here

    client_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (client_socket < 0)
    {
        perror("ERROR opening socket");
        exit_client(-1);
    }
    struct sockaddr_in server_addresse;
    bzero((char*)&server_addresse, sizeof(server_addresse));
    server_addresse.sin_family = AF_INET;
    server_addresse.sin_port   = htons(SERVER_PORT);
    if (inet_pton(AF_INET, "127.0.0.1", &server_addresse.sin_addr) <= 0)
    {
        perror("ERROR on inet_aton");
        exit_client(-2);
    }
    if (connect(client_socket, (struct sockaddr*)&server_addresse,
                sizeof(server_addresse)) < 0)
    {
        perror("ERROR on connect");
        exit_client(-3);
    }
}

//-------------------------------------------------------------

void handshake()
{
    char sent_message[MAX_MESSAGE_LENGTH] = "Hello, I'm the client";
    if (send(client_socket, sent_message, MAX_MESSAGE_LENGTH, 0) < 0)
    {
        perror("ERROR on send");
        exit_client(-4);
    }
    char receive_buffer[MAX_MESSAGE_LENGTH];
    if (recv(client_socket, receive_buffer, MAX_MESSAGE_LENGTH, 0) < 0)
    {
        perror("ERROR on recv");
        exit_client(-5);
    }
    printf("%s", receive_buffer);
}

//-------------------------------------------------------------

void close_connection()
{
    // write your code here

    int error_no = pthread_mutex_lock(&mutex);
    if (error_no != 0)
    {
        fprintf(stderr, "ERROR on mutex lock: %s/n", strerror(error_no));
        exit_client(-6);
    }
    int return_value = close(client_socket);
    error_no         = pthread_mutex_unlock(&mutex);
    if (error_no != 0)
    {
        fprintf(stderr, "ERROR on mutex unlock: %s/n", strerror(error_no));
        exit_client(-7);
    }
    if (return_value < 0)
    {
        perror("ERROR on close");
        exit_client(-8);
    }
}

//-------------------------------------------------------------

void send_message()
{

    // write your code here

    char input_buffer[MAX_MESSAGE_LENGTH];
    while (1)
    {
        memset(input_buffer, 0, MAX_MESSAGE_LENGTH);
        prompt_user_input(input_buffer, MAX_MESSAGE_LENGTH);
        if (strcmp(input_buffer, "\n") == 0)
        {
            get_request();
        }
        else
        {
            set_request(input_buffer);
        }
    }
}

//-------------------------------------------------------------

void set_request(char* message)
{
    // write your code here

    char concatenated_message[MAX_MESSAGE_LENGTH] = "s:";
    strcat(concatenated_message, message);
    int error_no = pthread_mutex_lock(&mutex);
    if (error_no != 0)
    {
        fprintf(stderr, "ERROR on mutex lock: %s/n", strerror(error_no));
        exit_client(-6);
    }
    int return_value =
        send(client_socket, concatenated_message, MAX_MESSAGE_LENGTH, 0);
    error_no = pthread_mutex_unlock(&mutex);
    if (error_no != 0)
    {
        fprintf(stderr, "ERROR on mutex unlock: %s/n", strerror(error_no));
        exit_client(-7);
    }
    if (return_value < 0)
    {
        perror("ERROR on send");
        exit_client(-4);
    }
    char receive_buffer[MAX_MESSAGE_LENGTH];
    error_no = pthread_mutex_lock(&mutex);
    if (error_no != 0)
    {
        fprintf(stderr, "ERROR on mutex lock: %s/n", strerror(error_no));
        exit_client(-6);
    }
    return_value = recv(client_socket, receive_buffer, MAX_MESSAGE_LENGTH, 0);
    error_no     = pthread_mutex_unlock(&mutex);
    if (error_no != 0)
    {
        fprintf(stderr, "ERROR on mutex unlock: %s/n", strerror(error_no));
        exit_client(-7);
    }
    if (return_value < 0)
    {
        perror("ERROR on recv");
        exit_client(-5);
    }
    if (strcmp(receive_buffer, "r:nack") == 0)
    {
        prompt_error();
    }
    else
    {
        if (strcmp(receive_buffer, "r:ack") != 0)
        {
            fprintf(stderr, "%s", receive_buffer);
            exit_client(-9);
        }
    }

    // (void) message; //please remove this if you implement this function
}

//-------------------------------------------------------------

void get_request()
{
    // write your code here

    char get_message[MAX_MESSAGE_LENGTH] = "g:";
    int error_no                         = pthread_mutex_lock(&mutex);
    if (error_no != 0)
    {
        fprintf(stderr, "ERROR on mutex lock: %s/n", strerror(error_no));
        exit_client(-6);
    }
    int return_value = send(client_socket, get_message, MAX_MESSAGE_LENGTH, 0);
    error_no         = pthread_mutex_unlock(&mutex);
    if (error_no != 0)
    {
        fprintf(stderr, "ERROR on mutex unlock: %s/n", strerror(error_no));
        exit_client(-7);
    }
    if (return_value < 0)
    {
        perror("ERROR on send");
        exit_client(-4);
    }
    char receive_buffer[MAX_MESSAGE_LENGTH];
    error_no = pthread_mutex_lock(&mutex);
    if (error_no != 0)
    {
        fprintf(stderr, "ERROR on mutex lock: %s/n", strerror(error_no));
        exit_client(-6);
    }
    return_value = recv(client_socket, receive_buffer, MAX_MESSAGE_LENGTH, 0);
    error_no     = pthread_mutex_unlock(&mutex);
    if (error_no != 0)
    {
        fprintf(stderr, "ERROR on mutex unlock: %s/n", strerror(error_no));
        exit_client(-7);
    }
    if (return_value < 0)
    {
        perror("ERROR on recv");
        exit_client(-5);
    }
    if (strcmp(receive_buffer, "r:ack") == 0)
    {
        print_reply(receive_buffer);
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

    pthread_attr_t thread_attributes;
    int error_no = pthread_attr_init(&thread_attributes);
    if (error_no != 0)
    {
        fprintf(stderr, "ERROR on thread initialization: %s/n",
                strerror(error_no));
        exit_client(-10);
    }
    error_no = pthread_attr_setdetachstate(&thread_attributes,
                                           PTHREAD_CREATE_DETACHED);
    if (error_no != 0)
    {
        fprintf(stderr, "%s/n", strerror(error_no));
        exit_client(-11);
    }
    error_no = pthread_create(&reading_thread, NULL, read_continously, NULL);
    if (error_no != 0)
    {
        fprintf(stderr, "%s/n", strerror(error_no));
        exit_client(-12);
    }
}
