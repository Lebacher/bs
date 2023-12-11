#include "../include/ring_buffer.h"
#include "../include/reads_list.h"
#include "../include/server.h"

#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

//! HINT: when designing the ring_buffer_element remember to
//! HINT: keep the critical region as small as necessary.
typedef struct ring_buffer_element
{
    //! HINT: something is missing here
    pthread_rwlock_t lock;
    char text[MAX_MESSAGE_LENGTH];
    int            reader_counter;

} ring_buffer_element_t;


static ring_buffer_element_t ring_buffer[RINGBUFFER_SIZE];

unsigned int current_writer = 0;

unsigned int number_of_readers = 0;

//! HINT: maybe global synchronization variables are needed

pthread_rwlock_t lock = PTHREAD_RWLOCK_INITIALIZER;

sem_t freeSpaceResourece;
//-----------------------------------------------------------------------------

void ringbuffer_initialize(void)
{
    sem_init(&freeSpaceResourece, 0, 0);
    //! HINT: maybe some additional initialization is needed
    for (int i = 0; i < RINGBUFFER_SIZE; i++)
    {
         sem_post(&freeSpaceResourece);
        ring_buffer[i].reader_counter = 0;
    }
}

//-----------------------------------------------------------------------------

int ringbuffer_write(char* text)
{
    //! HINTS: Check if thread can write a new element, and synchronization will be needed
    /* ... */

    ring_buffer_element_t *write_element = &ring_buffer[current_writer % RINGBUFFER_SIZE];

    pthread_rwlock_rdlock(&(write_element->lock));
    int read_client = write_element->reader_counter;
    pthread_rwlock_unlock(&(write_element->lock));

    if (read_client != 0)
    {
    
        return -1;
    }
    pthread_rwlock_wrlock(&(write_element->lock));

    //! Write element
    strcpy(ring_buffer[current_writer % RINGBUFFER_SIZE].text, text);

    ring_buffer[current_writer % RINGBUFFER_SIZE].reader_counter = number_of_readers;
    pthread_rwlock_unlock(&(write_element->lock));

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

    ring_buffer_element_t *read_element = &ring_buffer[reader % RINGBUFFER_SIZE];
    pthread_rwlock_rdlock(&read_element->lock);
    int new = read_element->reader_counter <= 0;
    pthread_rwlock_unlock(&read_element->lock);

    new = new || (unsigned int)*current_reader >= current_writer;

    if(new)
    {
        strcpy(buffer, "nack"); 
        return;
    }
    pthread_rwlock_wrlock(&read_element->lock);
    ring_buffer[reader % RINGBUFFER_SIZE].reader_counter--;
    pthread_rwlock_unlock(&read_element->lock);

    reads_list_decrement(client_number);

    //! Read Element
    pthread_rwlock_rdlock(&read_element->lock);
    strcpy(buffer, (const char*)ring_buffer[reader % RINGBUFFER_SIZE].text);
    pthread_rwlock_unlock(&read_element->lock);

    //! HINT: notify the writer

    //! Update reader count
    (*current_reader)++;

    return;
}

//-----------------------------------------------------------------------------

int ringbuffer_add_reader(unsigned int client_number)
{
    //! HINT: synchronization is needed in this function

    ringbuffer_initialize();

    if(reads_list_insert_element(client_number) != 0)
    {
        exit(EXIT_FAILURE);
    }
    pthread_rwlock_wrlock(&lock);

    number_of_readers++;
    int new_reader = current_writer;

    pthread_rwlock_unlock(&lock);

    return new_reader;
}

//-----------------------------------------------------------------------------

void ringbuffer_remove_reader(int* current_reader, unsigned int client_number)
{
    //! HINT: synchronization is needed in this function
    pthread_rwlock_rdlock(&lock);

    while(1)
    {
        int reads = reads_list_get_reads(client_number);

        if(reads == 0)
        {
            pthread_rwlock_unlock(&lock);
            pthread_rwlock_wrlock(&lock);
            reads_list_remove_reader(client_number);
            number_of_readers--;
            pthread_rwlock_unlock(&lock);
            return;
        }

        //! perform all unfinished reads for the disconnected client
        while(reads > 0)
        {
            char buffer[MAX_MESSAGE_LENGTH];
            ringbuffer_read(current_reader, buffer, client_number);
            reads = reads_list_get_reads(client_number);
        }
    }
}
