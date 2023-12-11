#include "../include/ring_buffer.h"
#include "../include/reads_list.h"
#include "../include/server.h"

#include "pthread.h"
#include "semaphore.h"
#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "time.h"

//! HINT: when designing the ring_buffer_element remember to
//! HINT: keep the critical region as small as necessary.
typedef struct ring_buffer_element
{
    //! HINT: something is missing here
    char text[MAX_MESSAGE_LENGTH];
    int            reader_counter;

} ring_buffer_element_t;


static ring_buffer_element_t ring_buffer[RINGBUFFER_SIZE];

unsigned int current_writer = 0;

unsigned int number_of_readers = 0;

//! HINT: maybe global synchronization variables are needed

//-----------------------------------------------------------------------------

int ringbuffer_write(char* text)
{
    //! HINTS: Check if thread can read a new element, and synchronization will be needed
    /* ... */

    //! Write element
    strcpy(ring_buffer[current_writer % RINGBUFFER_SIZE].text, text);

    ring_buffer[current_writer % RINGBUFFER_SIZE].reader_counter = number_of_readers;

    reads_list_increment_all();

    current_writer++;

    return 0;
}

//-----------------------------------------------------------------------------

void ringbuffer_read(int* current_reader, char* buffer, unsigned int client_number)
{

    int reader = *current_reader;
    //! HINT: Check if thread can read a new element & synchronization will be needed
    /* ... */

    ring_buffer[reader % RINGBUFFER_SIZE].reader_counter--;

    reads_list_decrement(client_number);

    //! Read Element
    strcpy(buffer, (const char*)ring_buffer[reader % RINGBUFFER_SIZE].text);

    //! HINT: notify the writer

    //! Update reader count
    (*current_reader)++;

    return;
}

//-----------------------------------------------------------------------------

int ringbuffer_add_reader(unsigned int client_number)
{
    //! HINT: synchronization is needed in this function

    if(reads_list_insert_element(client_number) != 0)
    {
        exit(EXIT_FAILURE);
    }

    number_of_readers++;
    int new_reader = current_writer;

    return new_reader;
}

//-----------------------------------------------------------------------------

void ringbuffer_remove_reader(int* current_reader, unsigned int client_number)
{
    //! HINT: synchronization is needed in this function

    int reads = reads_list_get_reads(client_number);

    //! perform all unfinished reads for the disconnected client
    while(reads != 0)
    {
        char buffer[MAX_MESSAGE_LENGTH];
        ringbuffer_read(current_reader, buffer, client_number);
        reads = reads_list_get_reads(client_number);
    }

    reads_list_remove_reader(client_number);
    number_of_readers--;

    return;
}
