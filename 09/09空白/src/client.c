#include "client.h"

pthread_t       reading_thread;
int             client_socket;
//Add your global variables here

void connect_to_server()
{
    //write your code here
}

//-------------------------------------------------------------

void handshake()
{

    //write your code here
}

//-------------------------------------------------------------

void send_message()
{

    //write your code here
}

//-------------------------------------------------------------

void set_request(char* message)
{
    //write your code here
    (void) message; //please remove this if you implement this function
}

//-------------------------------------------------------------

void get_request()
{
    //write your code here
}

//-------------------------------------------------------------

void* read_continously(void* unused)
{
    (void) unused; //Mark variable as used for the compiler :-)

    //write your code here

    //this method should not return so dont care about return value
    return NULL; 
}

//-------------------------------------------------------------

void start_reader_thread()
{
    //write your code here
}
