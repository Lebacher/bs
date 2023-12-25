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

pthread_mutex_t ring_buffer_mutex = PTHREAD_MUTEX_INITIALIZER;
sem_t* ring_buffer_semaphore;
//-----------------------------------------------------------------------------

void ringbuffer_initialize(void)
{
    //! HINT: maybe some additional initialization is needed
    for (int i = 0; i < RINGBUFFER_SIZE; i++)
    {
        ring_buffer[i].reader_counter = 0;
    }
	
    ring_buffer_semaphore = malloc(sizeof(sem_t));
    if (ring_buffer_semaphore == NULL)
    {
        perror("malloc failed.");
        exit(EXIT_FAILURE);
    }
	
    if (sem_init(ring_buffer_semaphore, 0, RINGBUFFER_SIZE) < 0)
    {
        perror("sem_init failed.");
        exit(EXIT_FAILURE);
    }
}

//-----------------------------------------------------------------------------

int ringbuffer_write(char* text)
{
    //! HINTS: Check if thread can write a new element, and synchronization will be needed
    
    if (sem_trywait(ring_buffer_semaphore) == 0)
    {
        //! Write element
		strcpy(ring_buffer[current_writer % RINGBUFFER_SIZE].text, text);

        if (pthread_mutex_lock(&ring_buffer_mutex) != 0) 
		{
			perror("pthread_mutex_lock failed.");
			exit(EXIT_FAILURE);
		}
		
        ring_buffer[current_writer % RINGBUFFER_SIZE].reader_counter = number_of_readers;
		
        if (pthread_mutex_unlock(&ring_buffer_mutex) != 0) 
		{
			perror("pthread_mutex_unlock failed.");
			exit(EXIT_FAILURE);
		}

        reads_list_increment_all();

		if (pthread_mutex_lock(&ring_buffer_mutex) != 0) 
		{
			perror("pthread_mutex_lock failed.");
			exit(EXIT_FAILURE);
		}

        current_writer++;
		
		if (pthread_mutex_unlock(&ring_buffer_mutex) != 0) 
		{
			perror("pthread_mutex_unlock failed.");
			exit(EXIT_FAILURE);
		}


        return 0;
    }
    else 
	{
		return -1;
	}
}

//-----------------------------------------------------------------------------

void ringbuffer_read(int* current_reader, char* buffer, unsigned int client_number)
{

    int reader = *current_reader;
    //! HINT: Check if thread can read a new element & synchronization will be needed
    
    if (reads_list_get_reads(client_number) == 0) 
    {
        strcpy(buffer, "nack");
    }
    else
    {
		int old_reader_counter = ring_buffer[reader % RINGBUFFER_SIZE].reader_counter;
		
        if (pthread_mutex_lock(&ring_buffer_mutex) != 0) 
		{
			perror("pthread_mutex_lock failed.");
			exit(EXIT_FAILURE);
		}
		
        ring_buffer[reader % RINGBUFFER_SIZE].reader_counter--;
        
        if (pthread_mutex_unlock(&ring_buffer_mutex) != 0) 
		{
			perror("pthread_mutex_unlock failed.");
			exit(EXIT_FAILURE);
		}
		
		reads_list_decrement(client_number);
		
		//! Read Element
        strcpy(buffer, (const char*)ring_buffer[reader % RINGBUFFER_SIZE].text);
		
		//! HINT: notify the writer
		int new_reader_counter = ring_buffer[reader % RINGBUFFER_SIZE].reader_counter;
		
		if (old_reader_counter == 1 && new_reader_counter == 0)
		{
			if (sem_post(ring_buffer_semaphore) == -1) 
			{
				perror("sem_post failed.");
				exit(EXIT_FAILURE);
			}
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

    if(reads_list_insert_element(client_number) != 0)
    {
        exit(EXIT_FAILURE);
    }

	if (pthread_mutex_lock(&ring_buffer_mutex) != 0) 
	{
        perror("pthread_mutex_lock failed.");
        exit(EXIT_FAILURE);
    }
	
    number_of_readers++;
    int new_reader = current_writer;
    
	if (pthread_mutex_unlock(&ring_buffer_mutex) != 0) 
	{
        perror("pthread_mutex_unlock failed.");
        exit(EXIT_FAILURE);
    }

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
            
			if (pthread_mutex_lock(&ring_buffer_mutex) != 0) 
			{
				perror("pthread_mutex_lock failed.");
				exit(EXIT_FAILURE);
			}
			
            number_of_readers--;
            
			if (pthread_mutex_unlock(&ring_buffer_mutex) != 0) 
			{
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
