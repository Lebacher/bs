#ifndef _CLIENT_H_
#define _CLIENT_H_

#include "client.h"
#include "utils.h"
#include <netinet/in.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#define SERVER_PORT (get_port_number())

enum constants
{
    MAX_MESSAGE_LENGTH = 200,
    MAX_USER_INPUT     = 198,
    READING_INTERVAL   = 2
};


extern pthread_t reading_thread;
extern int       client_socket;

/**
 * @brief initialize_server_socket initialize the socket for the server
 */
void connect_to_server();

/**
 * @brief Sends an arbitrary message to the server
 * and in return receives a similar arbitrary message from the server that is printed on the screen
 */
void handshake();

/**
 * @brief prompts the user for an input and depending on it a get or a set request is send to the server
 */
void send_message();

/**
 * @brief sends a set request containing the message as payload
 * @param message
 */
void set_request(char* message);
/**
 * @brief sends a get request to the server
 */
void get_request();

/**
 * @brief starts a thread that continously performs get requests
 */
void start_reader_thread();

/**
 * @brief this method should be run in a thread, it performs continous read requests
 */
void* read_continously(void* unused);

#endif /* include guard */
