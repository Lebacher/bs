#include "../include/ring_buffer.h"
#include "../include/reads_list.h"
#include "../include/server.h"

#include "pthread.h"
#include "semaphore.h"
#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "time.h"
#include <stdbool.h>

typedef struct ring_buffer_element
{
    char text[MAX_MESSAGE_LENGTH];
    int reader_counter;
    // Mutex used to prevent access on this element
    pthread_mutex_t mutex;

} ring_buffer_element_t;

static ring_buffer_element_t ring_buffer[RINGBUFFER_SIZE];

unsigned int current_writer    = 0;
unsigned int number_of_readers = 0;
// Mutex lock for current writer and number_of_readers
pthread_mutex_t thread_mutex = PTHREAD_MUTEX_INITIALIZER;
// Condition to signal the writer after a reader is done
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;

//-----------------------------------------------------------------------------

int ringbuffer_write(char* text)
{
    // Lock for current_writer
    pthread_mutex_lock(&thread_mutex);
    int writer = current_writer;

    // Lock for this element
    pthread_mutex_lock(&ring_buffer[writer % RINGBUFFER_SIZE].mutex);
    if (ring_buffer[writer % RINGBUFFER_SIZE].reader_counter > 0)
    {

        struct timespec time;
        memset(&time, 0, sizeof(time));
        clock_gettime(CLOCK_REALTIME, &time);
        time.tv_sec++;
        // Wait for 1s to hopefully get notified from a writer
        if (pthread_cond_timedwait(&cond,
                                   &ring_buffer[writer % RINGBUFFER_SIZE].mutex,
                                   &time) != 0)
        {
            // The reader didnt notify the writer in time.
            printf("pthread cond timedwait\n");
            // Unlock mutexs before we return -1 as error
            pthread_mutex_unlock(&ring_buffer[writer % RINGBUFFER_SIZE].mutex);
            pthread_mutex_unlock(&thread_mutex);
            return -1;
        }
    }

    // Set text and reset reader counter to number of readers
    strcpy(ring_buffer[writer % RINGBUFFER_SIZE].text, text);
    ring_buffer[writer % RINGBUFFER_SIZE].reader_counter = number_of_readers;

    // Unlock the element mutex
    pthread_mutex_unlock(&ring_buffer[writer % RINGBUFFER_SIZE].mutex);
    reads_list_increment_all();

    // Set current writer up for next time it gets used
    current_writer++;
    // Unlock current writer variable
    pthread_mutex_unlock(&thread_mutex);

    return 0;
}

//-----------------------------------------------------------------------------

void ringbuffer_read(int* current_reader, char* buffer,
                     unsigned int client_number)
{
    int reader = *current_reader;

    // if reads list element has nothing unread then send nack back
    if (reads_list_get_reads(client_number) == 0)
    {
        strcpy(buffer, "nack");
        return;
    }

    pthread_mutex_lock(&ring_buffer[reader % RINGBUFFER_SIZE].mutex);
    int notify = ring_buffer[reader % RINGBUFFER_SIZE].reader_counter;
    if (notify == 0)
    {
        // reader counter is 0 so message was already read. Unlock mutex then
        // return nack to the client (write to shared buffer)
        printf("Error, message cant be read any longer\n");
        pthread_mutex_unlock(&ring_buffer[reader % RINGBUFFER_SIZE].mutex);
        strcpy(buffer, "nack");
        return;
    }

    // Decrese reader counter because we're reading this element
    ring_buffer[reader % RINGBUFFER_SIZE].reader_counter--;

    reads_list_decrement(client_number);

    //! Read Element
    strcpy(buffer, (const char*)ring_buffer[reader % RINGBUFFER_SIZE].text);

    // Notify the writer
    if (notify - 1 == 0)
    {
        pthread_cond_signal(&cond);
    }

    // Unlock mutex for this element
    pthread_mutex_unlock(&ring_buffer[reader % RINGBUFFER_SIZE].mutex);

    //! Update reader count
    (*current_reader)++;
    return;
}

//-----------------------------------------------------------------------------

int ringbuffer_add_reader(unsigned int client_number)
{

    // Lock variables
    pthread_mutex_lock(&thread_mutex);

    // insert client and exit if insert fails
    if (reads_list_insert_element(client_number) != 0)
    {
        exit(EXIT_FAILURE);
    }

    // Add reader on number_of_readers
    number_of_readers++;
    // set new_reader to current position of messages
    int new_reader = current_writer;

    // Unlock variables
    pthread_mutex_unlock(&thread_mutex);
    return new_reader;
}

//-----------------------------------------------------------------------------

void ringbuffer_remove_reader(int* current_reader, unsigned int client_number)
{
    // Lock variables
    pthread_mutex_lock(&thread_mutex);

    int reads = reads_list_get_reads(client_number);

    //! perform all unfinished reads for the disconnected client
    while (reads != 0)
    {
        char buffer[MAX_MESSAGE_LENGTH];
        ringbuffer_read(current_reader, buffer, client_number);
        reads = reads_list_get_reads(client_number);
    }

    // Remove reader from reads list and from number_of_readers
    reads_list_remove_reader(client_number);
    number_of_readers--;

    // Unlock variables
    pthread_mutex_unlock(&thread_mutex);

    return;
}
