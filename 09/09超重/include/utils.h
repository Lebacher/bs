#ifndef _UTILS_H_
#define _UTILS_H_

#include <netinet/in.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdint.h>

// VT100 codes (including escape character \x1b)
#define CLEAREOL "\x1b[K"
#define CURSORUP1 "\x1b[1A"
#define CLEARLINE "\x1b[2K\r"
#define REVINDEX "\x1b[I"
#define PROMPT "Send:"
#define ERROR_MESSAGE "Operation Failed (press any key to continue)"
#define DISPLAY_TIME 2

/**
 * @brief prompts the user for input and reads the user input afterwards
 * @param buffer: pointer to memory where the user input should be stored
 * @param size: maximum buffer size
 * @return: returns a pointer to the buffer
 */
char* prompt_user_input(char* buffer, size_t size);

/**
 * @brief set the cursor after an input on the first position of the upper line
 */
void prompt_clear();

/**
 * @brief write an error message on the output stream
 */
void prompt_error();

/**
 * @brief print the prompt for the user imput.
 */
void prompt_print();

/**
 * @brief returns a port number in range of 31026-35122 based on $USER
 */
uint16_t get_port_number();

/**
 * @brief closes socket connection and exits process
 * @see close_connection()
 * @param exit_code is the exit code of the process (see exit())
 */
void exit_client(int exit_code);

/**
 * @brief registers an own signal handler for various signals
 * @note: __attribute__((constructor)) will call this method automagically before the main()
 * @see signal_handler()
 */
void initialize_signal_handler(void) __attribute__((constructor));

/**
 * @brief ensures that the socket is closed befor the process terminates
 * @param sig: signal
 */
void signal_handler(int sig);

/**
 * @brief erases all user input that got buffered by the tty so far
 */
void clear_user_input();

/**
 * @brief prints the reply message without messing up the prompt
 * @param message
 * @bug: long messages may be interrupted visually (they are still stored correctly in the tty buffer)
 */
void print_reply(char* message);

#endif /* include guard */
