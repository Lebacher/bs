#include "client.h"
#include <arpa/inet.h>

pthread_t       reading_thread;
int             client_socket;
//Add your global variables here
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

void connect_to_server()
{
    //write your code here
	
	//Create a socket.
    struct sockaddr_in serv_addr;
    client_socket = socket(AF_INET, SOCK_STREAM, 0);
    
    if (client_socket == -1)
    {
        perror("init");
        exit_client(1);
    }
	
	//Initialize the Server Address Structure.
	//Clear the serv_addr structure. 
    bzero((char *)&serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(SERVER_PORT);

	//Convert the string representation of the IP address to binary format.
    if (inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0)
    {
        perror("inet_pton");
        exit_client(1);
    }
	
    //Detected error in connect
    if (connect(client_socket, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
        perror("connect");
        exit_client(1);
    }
}

//-------------------------------------------------------------

void handshake()
{

    //write your code here
	char s_message[MAX_MESSAGE_LENGTH] = "Hello, I'm the client";
	char r_message[MAX_MESSAGE_LENGTH];
	
	//Send a Message to the Server.
    if (send(client_socket, s_message, MAX_MESSAGE_LENGTH, 0) == -1)
    {
        perror("send");
        exit_client(1);
    }
	
	//Receive a Message from the Server.
    if (recv(client_socket, r_message, MAX_MESSAGE_LENGTH, 0) == -1)
    {
        perror("recv");
        exit_client(1);
    }
	
	//Print the Received Message.
    printf("%s", r_message);
}

//-------------------------------------------------------------

void close_connection()
{
    //write your code here
	//Acquire Mutex.
    if (pthread_mutex_lock(&mutex) != 0)
    {
        perror("lock");
        exit_client(1);
    }
	
    //Close the Socket.
	if (close(client_socket) < 0)
    {
        perror("close");
        exit_client(1);
    }
	
	//Release the Mutex.
    if (pthread_mutex_unlock(&mutex) != 0)
    {
        perror("unlock");
        exit_client(1);
    }
}

//-------------------------------------------------------------

void send_message()
{

    //write your code here
	//Declare a character array as the message buffer.
	char m_buffer[MAX_MESSAGE_LENGTH];
    while (1)
    {
		//Clear the message buffer.
        memset(m_buffer, 0, MAX_MESSAGE_LENGTH);
        prompt_user_input(m_buffer, MAX_MESSAGE_LENGTH);
		
		//Check User Input.
        if (strcmp(m_buffer, "\n") == 0)
        {
            get_request();
        }
        else
        {
            set_request(m_buffer);
        }
    }
}

//-------------------------------------------------------------

void set_request(char* message)
{
    //write your code here
	//add "s:" to s_message.
	char s_message[MAX_MESSAGE_LENGTH] = "s:";
	char r_message[MAX_MESSAGE_LENGTH];
    strcat(s_message, message);
	
	//Mutex lock.
    if (pthread_mutex_lock(&mutex) != 0)
    {
        perror("lock");
        exit_client(1);
    }
	
	//Send the Request Message.
	if (send(client_socket, s_message, MAX_MESSAGE_LENGTH, 0) == -1)
    {
        perror("send");
        exit_client(1);
    }
	
	//Receive the Server's Response.
    if (recv(client_socket, r_message, MAX_MESSAGE_LENGTH, 0) == -1)
    {
        perror("recv");
        exit_client(1);
    }
	
	//The message is not stored by the server.
	if (strcmp(r_message, "r:nack") == 0)
    {
        prompt_error();
    }
    else if (strcmp(r_message, "r:ack") == 0)
    {
		//Handling the case where the server acknowledges the request.
    }
	else
    {
        perror(NULL);
		exit_client(1);
    }
	
	//Mutex unlock. 
    if (pthread_mutex_unlock(&mutex)!= 0)
    {
        perror("unlock");
        exit_client(1);
    }
	
    //(void) message; //please remove this if you implement this function
}

//-------------------------------------------------------------

void get_request()
{
    //write your code here
	char s_message[MAX_MESSAGE_LENGTH] = "g:";
	char r_message[MAX_MESSAGE_LENGTH];
	
	//Mutex lock.
    if (pthread_mutex_lock(&mutex) != 0)
    {
        perror("lock");
        exit_client(1);
    }
	
	//Send the Request Message.
	if (send(client_socket, s_message, MAX_MESSAGE_LENGTH, 0) == -1)
    {
        perror("send");
        exit_client(1);
    }
	
	//Receive the Server's Response.
    if (recv(client_socket, r_message, MAX_MESSAGE_LENGTH, 0) == -1)
    {
        perror("recv");
        exit_client(1);
    }
	
	//print the message.
	if (strcmp("r:nack", r_message) != 0)
        {
            print_reply(r_message);
        }
	
	//Mutex unlock. 
    if (pthread_mutex_unlock(&mutex)!= 0)
    {
        perror("unlock");
        exit_client(1);
    }
}

//-------------------------------------------------------------

void* read_continously(void* unused)
{
    (void) unused; //Mark variable as used for the compiler :-)

    //write your code here
	while (1)
    {
        get_request();
        sleep(READING_INTERVAL);
    }
    //this method should not return so dont care about return value
    return NULL; 
}

//-------------------------------------------------------------

void start_reader_thread()
{
    //write your code here
	pthread_attr_t thread_attributes;
	
    //Initialize Thread Attributes.
    if (pthread_attr_init(&thread_attributes) != 0)
    {
        perror("init");
        exit_client(1);
    }
	
	//Set Thread to Detached State. 
    if (pthread_attr_setdetachstate(&thread_attributes, PTHREAD_CREATE_DETACHED) != 0)
    {
        perror("detach");
        exit_client(1);
    }
	
    //Create a New Detached Thread.
    if (pthread_create(&reading_thread, &thread_attributes, read_continously, NULL) != 0)
    {
        perror("create");
        exit_client(1);
    }
}
