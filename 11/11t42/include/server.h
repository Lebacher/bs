#ifndef _SERVER_H_
#define _SERVER_H_

#define SERVER_PORT 31026
#define MAX_MESSAGE_LENGTH 201
#define MAX_SOCKET_QUEUE 20
#define CLIENT_HANDSHAKE_MSG "Hello, I'm the client"
#define SERVER_HANDSHAKE_MSG "Hello, I'm the server"

/**
 * @brief initialize_server, method to initialize socket
 * @return socket file descriptor
 */
int initialize_server();

/**
 * @brief handle_connection, method to handle connection, continuesly read from file descriptor and if connection was
 * not closed give the read text to handle input
 * @param socket, socket file descriptor to handle connection for
 * @return nothing
 */
void* handle_connection(void* socket);

/**
 * @brief handshake, method to do a handshake with a certain client
 * @param file_descriptor, file descriptor to do handshake with
 * @return 0 on success
 */
int handshake(int file_descriptor);

/**
 * @brief handle_input, method to handle a message from client
 * @param client_number, number of client to identify it
 * @param input, char* of client input
 * @param socket, socket number of client to reply
 * @param current_reader_pointer, current reader of client
 * @return 0 on success
 */
int handle_input(int client_number, char* input, int socket, int* current_reader_pointer);

/**
 * @brief accept_connections, method to continuously accept new connections
 * @param socket_number, server socket id
 */
void accept_connections(int socket_number);


void initialize_signal_handler(void);

#endif
