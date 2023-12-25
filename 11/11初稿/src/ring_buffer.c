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
	
	pthread_mutex_t element_mutex;

} ring_buffer_element_t;


static ring_buffer_element_t ring_buffer[RINGBUFFER_SIZE];

unsigned int current_writer = 0;

unsigned int number_of_readers = 0;

//! HINT: maybe global synchronization variables are needed

pthread_cond_t condition = PTHREAD_COND_INITIALIZER;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

//-----------------------------------------------------------------------------

void ringbuffer_initialize(void)
{
    //! HINT: maybe some additional initialization is needed
    for (int i = 0; i < RINGBUFFER_SIZE; i++)
    {
        ring_buffer[i].reader_counter = 0;

        if (pthread_mutex_init(&ring_buffer[i].element_mutex, NULL) != 0) {
            perror("pthread_mutex_init failed.");
            exit(EXIT_FAILURE);
        }
    }
}

//-----------------------------------------------------------------------------

int ringbuffer_write(char* text)
{
    //! HINTS: Check if thread can read a new element, and synchronization will be needed
    /* ... */

    if (pthread_mutex_lock
            (&ring_buffer[current_writer % RINGBUFFER_SIZE].element_mutex) != 0) {
        perror("pthread_rwlock_lock failed.");
        exit(EXIT_FAILURE);
    }

    if (ring_buffer[current_writer % RINGBUFFER_SIZE].reader_counter > 0) {
        struct timespec abs_timeout;
        clock_gettime(CLOCK_REALTIME, &abs_timeout);
        abs_timeout.tv_sec += 1;

        int result = pthread_cond_timedwait(&condition,
                     &ring_buffer[current_writer % RINGBUFFER_SIZE].element_mutex, &abs_timeout);

        if (result != 0) {
            printf("timeout.");

            if (pthread_mutex_unlock
               (&ring_buffer[current_writer % RINGBUFFER_SIZE].element_mutex) != 0) {
                 perror("pthread_mutex_unlock failed.");
                 exit(EXIT_FAILURE);
            }

                return -1;

        }
	}

    if (pthread_mutex_unlock
                    (&ring_buffer[current_writer % RINGBUFFER_SIZE].element_mutex) != 0) {
        perror("pthread_mutex_unlock failed.");
        exit(EXIT_FAILURE);
    }

    if (pthread_mutex_lock(&mutex) != 0) {
        perror("pthread_mutex_lock failed.");
        exit(EXIT_FAILURE);
    }

    //! Write element
        
    strcpy(ring_buffer[current_writer % RINGBUFFER_SIZE].text, text);

    ring_buffer[current_writer % RINGBUFFER_SIZE].reader_counter = number_of_readers;

    reads_list_increment_all();

    current_writer++;

    if (pthread_mutex_unlock(&mutex) != 0) {
        perror("pthread_mutex_unlock failed.");
        exit(EXIT_FAILURE);
    }

    return 0;
    
}

//-----------------------------------------------------------------------------

void ringbuffer_read(int* current_reader, char* buffer, unsigned int client_number)
{

    int reader = *current_reader;
    //! HINT: Check if thread can read a new element & synchronization will be needed
    /* ... */
    if (reads_list_get_reads(client_number) == 0) {
        strcpy(buffer, "nack");
        return;
    }

    if (pthread_mutex_lock(&ring_buffer[reader % RINGBUFFER_SIZE].element_mutex) != 0) {
        perror("pthread_mutex_lock failed.");
        exit(EXIT_FAILURE);
    }

    if (ring_buffer[reader % RINGBUFFER_SIZE].reader_counter == 0){
        if (pthread_mutex_unlock(&ring_buffer[reader % RINGBUFFER_SIZE].element_mutex) != 0) {
			perror("pthread_mutex_unlock failed.");
			exit(EXIT_FAILURE);
		}
		
        printf("Unreadable message");
        strcpy(buffer, "nack");
    }
    else {
        ring_buffer[reader % RINGBUFFER_SIZE].reader_counter--;

        reads_list_decrement(client_number);

        //! Read Element
        
        strcpy(buffer, (const char*)ring_buffer[reader % RINGBUFFER_SIZE].text);

        //! HINT: notify the writer
        
        if (ring_buffer[reader % RINGBUFFER_SIZE].reader_counter == 0){
            pthread_cond_signal(&condition);
        }

        if (pthread_mutex_unlock(&ring_buffer[reader % RINGBUFFER_SIZE].element_mutex) != 0) {
            perror("pthread_mutex_unlock failed.");
            exit(EXIT_FAILURE);
        }

        //! Update reader count
        
        (*current_reader)++;
    }

    return;
}

//-----------------------------------------------------------------------------

int ringbuffer_add_reader(unsigned int client_number)
{
    //! HINT: synchronization is needed in this function
    ringbuffer_initialize();
	
    if (pthread_mutex_lock(&mutex) != 0) {
        perror("pthread_mutex_lock failed.");
        exit(EXIT_FAILURE);
    }

    if(reads_list_insert_element(client_number) != 0)
    {
        exit(EXIT_FAILURE);
    }

    number_of_readers++;
    int new_reader = current_writer;

    if (pthread_mutex_unlock(&mutex) != 0) {
        perror("pthread_mutex_unlock failed.");
        exit(EXIT_FAILURE);
    }

    return new_reader;
}

//-----------------------------------------------------------------------------

void ringbuffer_remove_reader(int* current_reader, unsigned int client_number)
{
    //! HINT: synchronization is needed in this function
    
    if (pthread_mutex_lock(&mutex) != 0) {
        perror("pthread_mutex_lock failed.");
        exit(EXIT_FAILURE);
    }

    while(1)
    {
        int reads = reads_list_get_reads(client_number);

        if(reads == 0)
        {
            reads_list_remove_reader(client_number);
            number_of_readers--;

            if (pthread_mutex_unlock(&mutex) != 0) {
                perror("pthread_mutex_unlock failed.");
                exit(EXIT_FAILURE);
            }

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
