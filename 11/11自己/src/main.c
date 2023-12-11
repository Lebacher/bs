#include "../include/ring_buffer.h"
#include "../include/server.h"

#include "stdlib.h"
#include <signal.h>
#include <stdio.h>

/**
 * @brief signal_handler: be sure to exit the programm
 * @param sig: signal
 */
void signal_handler(int sig)
{
    //close all threads and the socket by receiving a signal
    //close_connection();
    printf("\n");
    exit(sig);
}

/**
 * @brief initialize_signal_handler: handle signals by users
 */
void initialize_signal_handler(void)
{
    signal(SIGINT,  signal_handler);
    signal(3,       signal_handler);
    signal(SIGILL,  signal_handler);
    signal(SIGABRT, signal_handler);
    signal(SIGFPE,  signal_handler);
    signal(SIGSEGV, signal_handler); 
    signal(SIGTERM, signal_handler);

}

int main()
{

    int server_socket = initialize_server();
    printf("server successfully initialized\n");

    initialize_signal_handler();

    accept_connections(server_socket);

    return 0;
}
