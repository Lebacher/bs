#include "../include/reads_list.h"

#include <pthread.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

//! HINT: when designing the reads_list_element remember to
//! HINT: keep the critical region as small as necessary.
typedef struct reads_list_element
{
    //! HINT: something is missing here
    unsigned int    client_number;
    struct reads_list_element* next;
    struct reads_list_element* previous;
    int reads;
    sem_t semaphore_client;

} reads_list_element_t;


reads_list_element_t* head = NULL;

//! HINT: maybe global synchronization variables are needed
pthread_rwlock_t reads_list_lock = PTHREAD_RWLOCK_INITIALIZER;
//-----------------------------------------------------------------------------

int reads_list_insert_element(unsigned int client_number)
{
    //! HINT: synchronization is needed in this function

    //! create new element
    reads_list_element_t* new_element = malloc(sizeof(reads_list_element_t));
    if(new_element == NULL)
    {
        perror("malloc");
        exit(EXIT_FAILURE);
    }

    new_element->client_number = client_number;
    new_element->next     = NULL;
    new_element->previous = NULL;

    //! insert element into list
    if(head == NULL)
    {
        head = new_element;
        return 0;
    }

    reads_list_element_t* temporary = head;
    while(temporary->next != NULL)
    {
        temporary = temporary->next;
    }
    new_element->previous = temporary;
    temporary->next       = new_element;
    return 0;
}


//-----------------------------------------------------------------------------

sem_t* reads_list_get_reader_semaphore(unsigned int client_number)
{
     reads_list_element_t* temporary = head;
     while (temporary->next != NULL)
    {
        if (temporary->client_number == client_number)
        {
            
        return &temporary->semaphore_client;
        }
        temporary =temporary->next;
    }
    
   
    return NULL; //! Please select a proper return value
}

//-----------------------------------------------------------------------------

void reads_list_increment_all()
{
    //! HINT: synchronization is needed in this function
    pthread_rwlock_rdlock(&reads_list_lock);

    reads_list_element_t* temporary = head;
    while(temporary != NULL)
    {
        temporary->reads++;
        
        temporary = temporary->next;
    }
    pthread_rwlock_unlock(&reads_list_lock);
}

//-----------------------------------------------------------------------------

void reads_list_decrement(unsigned int client_number)
{
    reads_list_element_t* temporary = head;
    pthread_rwlock_unlock(&reads_list_lock);
    while(temporary->client_number != client_number)
    {
        temporary = temporary->next;
    }
    temporary->reads--;
    pthread_rwlock_unlock(&reads_list_lock);
}

//-----------------------------------------------------------------------------

int reads_list_remove_reader(unsigned int client_number)
{
    //! HINT: synchronization is needed in this function
    pthread_rwlock_rdlock(&reads_list_lock);
    //! find element to remove
    reads_list_element_t* temporary = head;
    while(temporary != NULL && temporary->client_number != client_number)
    {
        temporary = temporary->next;
    }
    pthread_rwlock_unlock(&reads_list_lock);

    if(temporary == NULL)
    {
        return -1;
    }
    pthread_rwlock_rdlock(&reads_list_lock);

    //! bend pointers around element
    if(temporary->previous != NULL)
    {
        temporary->previous->next = temporary->next;
    }
    if(temporary->next != NULL)
    {
        temporary->next->previous = temporary->previous;
    }
    if(temporary == head)
    {
        head = temporary->next;
    }
    pthread_rwlock_unlock(&reads_list_lock);

    //! finally delete element
    free(temporary);
    return 0;
}

//-----------------------------------------------------------------------------

int reads_list_get_reads(unsigned int client_number)
{
    int buffer = 0;
    
    (void) client_number; //! Please remove this when you implement this function
    //! please implement this function

    return buffer;
}
