#include "client.h"

pthread_t reading_thread;
int client_socket;
// Add your global variables here
pthread_mutex_t lock;

void connect_to_server()
{
    // create a Socket
    struct sockaddr_in serv_addr;
    client_socket = socket(AF_INET, SOCK_STREAM, 0);
    // socket return -1 for error
    if (client_socket == -1)
    {
        perror("error in init");
        exit_client(1);
    }
    bzero((char *)&serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(SERVER_PORT);

    if (inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0)
    {
        perror("\n inet_pton error \n");
        exit_client(1);
    }
    // Detected error in connect
    if (connect(client_socket, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
        perror("\n connection Error \n");
        exit_client(1);
    }
    //init of the mutex
    if (pthread_mutex_init(&lock, NULL) != 0)
    {
        printf("\n mutex init failed \n");
        exit_client(1);
    }
}

//-------------------------------------------------------------

void handshake()
{

    // write your code here
    // use Max_MESSAGE_LENGTH for massage length
    char handshake_massage[MAX_MESSAGE_LENGTH];
    snprintf(handshake_massage, MAX_MESSAGE_LENGTH, "Hello, I'm the client");
    int m_send = send(client_socket, handshake_massage, MAX_MESSAGE_LENGTH, 0);
    int m_recv = recv(client_socket, handshake_massage, MAX_MESSAGE_LENGTH, 0);
    // Detecting if there is error in sending the massage
    if (m_send == -1)
    {
        perror(NULL);
        exit_client(1);
    }
    // Detecting if there is error in receiving the massage
    if (m_recv == -1)
    {
        perror(NULL);
        exit_client(1);
    }
    print_reply("Hello, I'm the server");
}

//-------------------------------------------------------------

void send_message()
{

    // write your code here
    char message_buffer[MAX_USER_INPUT];
    char *user_input;
    while (1)
    {
        user_input = prompt_user_input(message_buffer, MAX_USER_INPUT);
        // if input is empty
        if (strcmp(user_input, "\n") == 0)
        {
            get_request();
        }
        // if input is not empty
        else
        {
            set_request(user_input);
        }
    }
}

//-------------------------------------------------------------

void set_request(char *message)
{
    // write your code here
    char response[MAX_MESSAGE_LENGTH];
    char msgcat[MAX_MESSAGE_LENGTH] = "s:";
    int error_send;
    int error_recv;
    strcat(msgcat, message);
    switch (pthread_mutex_lock(&lock))
    {
    case 0:
        error_send = send(client_socket, msgcat, strlen(msgcat), 0);
        //error sending message
        if (error_send == -1)
        {
            perror(NULL);
            exit_client(1);
        }
        // if there is no error while sending look at recv
        else
        {
            error_recv = recv(client_socket, response, MAX_MESSAGE_LENGTH, 0);
            //error recv message
            if (error_recv == 0)
            {
                prompt_error();
            }
            else if (error_recv < 0)
            {
                perror(NULL);
                exit_client(1);
            }
        }
        // unlock pthread_mutex
        if (pthread_mutex_unlock(&lock) != 0)
        {
            // error
            perror(NULL);
            exit_client(1);
        }
        break;

    default:
        // error in lock
        perror(NULL);
        exit_client(1);
        break;
    }
    if (strcmp(response, "r:nack") == 0)
    {
        prompt_error();
    }
    else if (strcmp(response, "r:ack") != 0)
    {
        prompt_error(response);
        exit_client(1);
    }
}

//-------------------------------------------------------------

void get_request()
{
    // write your code here
    char response[MAX_MESSAGE_LENGTH];
    int error_send;
    int error_recv;
    switch (pthread_mutex_lock(&lock))
    {
    case 0:
        error_send = send(client_socket, "g:", strlen("g:"), 0);
        //error sending message
        if (error_send == -1)
        {
            perror(NULL);
            exit_client(1);
        }
        error_recv = recv(client_socket, response, MAX_MESSAGE_LENGTH, 0);
        //error recv message
        if (error_recv == -1)
        {
            perror(NULL);
            exit_client(1);
        }
        // print the message if no nack
        else if (strcmp("r:nack", response) != 0)
        {
            print_reply(response);
        }
        // unlock pthread_mutex
        if (pthread_mutex_unlock(&lock) != 0)
        {
            // error
            perror(NULL);
            exit_client(1);
        }
        break;

    default:
        // error in lock
        perror(NULL);
        exit_client(1);
        break;
    }
}

//-------------------------------------------------------------

void *read_continously(void *unused)
{
    (void)unused; // Mark variable as used for the compiler :-)

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
    // pthread_attr_t initialisieren 
    pthread_attr_t thread_attribute;
    pthread_attr_init(&thread_attribute);
    // Create pthread and detecting error
    if (pthread_create(&reading_thread, &thread_attribute, read_continously, NULL) != 0)
    {
        perror(NULL);
        exit_client(1);
    }
    pthread_attr_destroy(&thread_attribute); 
}
