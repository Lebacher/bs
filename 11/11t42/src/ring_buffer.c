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
    pthread_rwlock_t lock;
    char text[MAX_MESSAGE_LENGTH];
    int            reader_counter;

} ring_buffer_element_t;


static ring_buffer_element_t ring_buffer[RINGBUFFER_SIZE];

unsigned int current_writer = 0;

unsigned int number_of_readers = 0;

//Mutex fuer Writer
pthread_mutex_t lock_writer = PTHREAD_MUTEX_INITIALIZER;

//Mutex fuer Anzahl der Reader
pthread_mutex_t lock_reader_count = PTHREAD_MUTEX_INITIALIZER;

//done

int ringbuffer_write(char* text)
{
    pthread_mutex_lock(&lock_writer); //lock

    ring_buffer_element_t *w_element = &ring_buffer[current_writer % RINGBUFFER_SIZE];

    pthread_rwlock_rdlock(&(w_element->lock)); //lock
    
    int read_c = w_element->reader_counter;
    pthread_rwlock_unlock(&(w_element->lock)); //unlock

    if (read_c != 0)
    {
        pthread_mutex_unlock(&lock_writer); //unlock
        return -1;
    }

    pthread_rwlock_wrlock(&(w_element->lock)); //lock

    //! Write element
    strcpy(ring_buffer[current_writer % RINGBUFFER_SIZE].text, text);

    pthread_mutex_lock(&lock_reader_count); //lock
    ring_buffer[current_writer % RINGBUFFER_SIZE].reader_counter = number_of_readers;
    pthread_mutex_unlock(&lock_reader_count); //unlock

    pthread_rwlock_unlock(&(w_element->lock)); //unlock
    reads_list_increment_all();
    current_writer++;
    pthread_mutex_unlock(&lock_writer); //unlock

    return 0;
}

//-----------------------------------------------------------------------------

void ringbuffer_read(int* current_reader, char* buffer, unsigned int client_number)
{

    int reader = *current_reader;

    ring_buffer_element_t *r_element = &ring_buffer[reader % RINGBUFFER_SIZE];

    pthread_rwlock_rdlock(&r_element->lock); //lock
    int n = r_element->reader_counter <= 0;
    pthread_rwlock_unlock(&r_element->lock); //unlock

    pthread_mutex_lock(&lock_writer); //lock
    n = n || (unsigned int)*current_reader >= current_writer;
    pthread_mutex_unlock(&lock_writer); //unlock

    if(n)
    {
        strcpy(buffer, "nack"); //copied "nack" to buffer
        return;
    }

    pthread_rwlock_wrlock(&r_element->lock); //lock
    ring_buffer[reader % RINGBUFFER_SIZE].reader_counter--;
    pthread_rwlock_unlock(&r_element->lock); //lock


    reads_list_decrement(client_number);

    //! Read Element
    pthread_rwlock_rdlock(&r_element->lock); //lock
    strcpy(buffer, (const char*)ring_buffer[reader % RINGBUFFER_SIZE].text);
    pthread_rwlock_unlock(&r_element->lock); //unlock

    //! HINT: notify the writer

    //! Update reader count
    (*current_reader)++;

    return;
}

//-----------------------------------------------------------------------------

int ringbuffer_add_reader(unsigned int client_number)
{

    if(reads_list_insert_element(client_number) != 0)
    {
        exit(EXIT_FAILURE);
    }

    pthread_mutex_lock(&lock_reader_count); //lock
    number_of_readers++; //erhöhe #Reader
    pthread_mutex_unlock(&lock_reader_count); //unlock


    pthread_mutex_lock(&lock_writer); //lock
    int new_reader = current_writer;
    pthread_mutex_unlock(&lock_writer); //unlock

    return new_reader;
}

//-----------------------------------------------------------------------------

void ringbuffer_remove_reader(int* current_reader, unsigned int client_number)
{
    int reads = reads_list_get_reads(client_number);

    //! perform all unfinished reads for the disconnected client
    while(reads != 0)
    {
        char buffer[MAX_MESSAGE_LENGTH];
        ringbuffer_read(current_reader, buffer, client_number);
        reads = reads_list_get_reads(client_number);
    }

    reads_list_remove_reader(client_number);
    pthread_mutex_lock(&lock_reader_count); //lock
    number_of_readers--; //reduziere #Reader
    pthread_mutex_unlock(&lock_reader_count); //unlock

    return;
}
