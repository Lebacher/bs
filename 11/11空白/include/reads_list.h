#ifndef _LILI_H_
#define _LILI_H_

#include "semaphore.h"

/**
 * @brief reads_list_insert_element, insert an element into reads_list for a new reader
 * @param client_number, number of client that connected
 * @return 0 on success
 */
int reads_list_insert_element(unsigned int client_number);

/**
 * @brief reads_list_get_reader_semaphore, returns reads_list semaphore pointer of client with client_number
 * @param client_number
 * @return semaphore pointer to reads_list element semaphore, NULL if client_number did not exist
 */
sem_t* reads_list_get_reader_semaphore(unsigned int client_number);

/**
 * @brief reads_list_increment_all, increment the possible reads for every client
 * @note should be used after write
 */
void reads_list_increment_all();

/**
 * @brief reads_list_decrement, decrements possible reads for particular client
 * @param client_number
 * @note should be used after read
 */
void reads_list_decrement(unsigned int client_number);

/**
 * @brief reads_list_remove_reader, remove lili element when client disconnects
 * @param client_number, number of client that disconnected
 * @return 0 on success
 */
int reads_list_remove_reader(unsigned int client_number);

/**
 * @brief gets you the reads value
 * @param client_number, the unique client id
 * @return returns how many reads a client can do
 */
int reads_list_get_reads(unsigned int client_number);

#endif
