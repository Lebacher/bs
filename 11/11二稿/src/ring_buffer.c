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
    char text[MAX_MESSAGE_LENGTH];
    int            reader_counter;

} ring_buffer_element_t;


static ring_buffer_element_t ring_buffer[RINGBUFFER_SIZE];

unsigned int current_writer = 0;

unsigned int number_of_readers = 0;

//! HINT: maybe global synchronization variables are needed

static sem_t* to_write;
static pthread_mutex_t ring_buffer_mutex = PTHREAD_MUTEX_INITIALIZER;

//-----------------------------------------------------------------------------

void ringbuffer_initialize(void)
{
    //! HINT: maybe some additional initialization is needed
    for (int i = 0; i < RINGBUFFER_SIZE; i++)
    {
        ring_buffer[i].reader_counter = 0;
    }
    to_write = malloc(sizeof(sem_t));
    if (to_write == NULL)
    {
        perror("malloc");
        exit(-1);
    }
    if (sem_init(to_write, 0, 10) < 0)
    {
        perror("ERROR sem_init");
        exit(-1);
    }
}

//-----------------------------------------------------------------------------

int ringbuffer_write(char* text)
{
    //! HINTS: Check if thread can write a new element, and synchronization will be needed
    /* ... */

    //! Write element

    if (sem_trywait(to_write) != -1)
    {
        strcpy(ring_buffer[current_writer % RINGBUFFER_SIZE].text, text);

        pthread_mutex_lock(&ring_buffer_mutex);
        ring_buffer[current_writer % RINGBUFFER_SIZE].reader_counter = number_of_readers;
        current_writer++;
        pthread_mutex_unlock(&ring_buffer_mutex);

        reads_list_increment_all();


        return 0;
    }
    return -1;
}

//-----------------------------------------------------------------------------

void ringbuffer_read(int* current_reader, char* buffer, unsigned int client_number)
{

    int reader = *current_reader;
    //! HINT: Check if thread can read a new element & synchronization will be needed
    /* ... */

    int old_reader_counter = ring_buffer[reader % RINGBUFFER_SIZE].reader_counter;

    int can_read = 0;
    if (sem_getvalue(reads_list_get_reader_semaphore(client_number), &can_read) < 0)
    {
        perror("ERROR getvalue");
        exit(-1);
    }

    reads_list_decrement(client_number);

    //! Read Element
    if (can_read == 0)
    {
        strcpy(buffer, "nack");
    }
    else
    {
        pthread_mutex_lock(&ring_buffer_mutex);
        ring_buffer[reader % RINGBUFFER_SIZE].reader_counter--;
        (*current_reader)++;
        pthread_mutex_unlock(&ring_buffer_mutex);
        strcpy(buffer, (const char*)ring_buffer[reader % RINGBUFFER_SIZE].text);
    }

    //! HINT: notify the writer
    if (old_reader_counter == 1 && ring_buffer[reader % RINGBUFFER_SIZE].reader_counter == 0)
    {
        if (sem_post(to_write) < 0)
        {
            perror("ERROR sem_post");
            exit(-1);
        }
    }

    //! Update reader count

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

    pthread_mutex_lock(&ring_buffer_mutex);
    number_of_readers++;
    int new_reader = current_writer;
    pthread_mutex_unlock(&ring_buffer_mutex);

    return new_reader;
}

//-----------------------------------------------------------------------------

void ringbuffer_remove_reader(int* current_reader, unsigned int client_number)
{
    //! HINT: synchronization is needed in this function

    while(1)
    {
        int reads = reads_list_get_reads(client_number);

        if(reads == 0)
        {
            reads_list_remove_reader(client_number);
            pthread_mutex_lock(&ring_buffer_mutex);
            number_of_readers--;
            pthread_mutex_unlock(&ring_buffer_mutex);
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
