#ifndef _RINGBUFFER_H_
#define _RINGBUFFER_H_

#define RINGBUFFER_SIZE 10 //! number of ring buffer elements


/**
 * @brief ringbuffer_initialize, method to initialize the ringbuffer
 */
void ringbuffer_initialize(void);

/**
 * @brief ringbuffer_write_element, method to write a text into the ringbuffer
 * @param text, text to write into ringbuffer
 * @return 0 on success, -1 on fail
 */
int ringbuffer_write(char* text);

/**
 * @brief ringbuffer_read_element, method to read the element at the current reader
 * @param current_reader, number to identify element to read
 * @param buffer, char* to store read text
 * @param client_number, number of the client to update reads_list
 */
void ringbuffer_read(int* current_reader, char* buffer, unsigned int client_number);

/**
 * @brief ringbuffer_add_reader, method to add another reader to the ringbuffer
 * @return new current reader number for client
 */
int ringbuffer_add_reader(unsigned int client_number);

/**
 * @brief ringbuffer_remove_reader, method to delete Reader from Ringbuffer when reader disconnects
 * @param current_reader, current reader number of client
 * @param client_number, number of client
 */
void ringbuffer_remove_reader(int* current_reader, unsigned int client_number);


#endif
