#include "reads_list.h"
#include "pthread.h"
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

//! HINT: when designing the reads_list_element remember to
//! HINT: keep the critical region as small as necessary.
typedef struct reads_list_element
{
    sem_t sem_client; //init einer Semaphore
    unsigned int    client_number;
    struct reads_list_element* next;
    struct reads_list_element* previous;

} reads_list_element_t;


reads_list_element_t* head = NULL;

//RW Lock Variable
pthread_rwlock_t lock = PTHREAD_RWLOCK_INITIALIZER;

//! HINT: maybe global synchronization variables are needed

//-----------------------------------------------------------------------------

int reads_list_insert_element(unsigned int client_number)
{
    //! create new element
    reads_list_element_t* new_element = malloc(sizeof(reads_list_element_t));
    if(new_element == NULL)
    {
        perror("malloc");
        exit(EXIT_FAILURE);
    }

    new_element->client_number = client_number;

    if(sem_init(&(new_element->sem_client), 0, 0) == -1) //init einr Semaphore
        {
            perror("ERROR no initialize of the semaphore");
            exit(EXIT_FAILURE);
        }

        pthread_rwlock_wrlock(&lock); //lock

    //! insert element into list
    if(head == NULL)
    {
        new_element->next     = NULL;
        new_element->previous = NULL;
        head                  = new_element;
        return 0;
    }

    reads_list_element_t* temporary = head;
    while(temporary->next != NULL)
    {
        temporary = temporary->next;
    }
    new_element->next     = NULL;
    new_element->previous = temporary;
    temporary->next       = new_element;

    pthread_rwlock_unlock(&lock); //unlock

    return 0;
}

//-----------------------------------------------------------------------------

sem_t* reads_list_get_reader_semaphore(unsigned int client_number)
{
    pthread_rwlock_rdlock(&lock); //lock
    reads_list_element_t* temporary = head;
    sem_t *return_value = NULL;

    while(temporary != NULL && temporary->client_number!= client_number)
    {
        temporary= temporary->next;
    }

    if(temporary != NULL)
    {
        return_value = &(temporary->sem_client);
    }

    pthread_rwlock_unlock(&lock); //unlock

    return return_value;
}

//-----------------------------------------------------------------------------

void reads_list_increment_all()
{
    pthread_rwlock_wrlock(&lock); //lock

    reads_list_element_t* temporary = head;
    while(temporary != NULL)
    {
        if(sem_post(&(temporary->sem_client)) != 0)
        {
            perror("ERROR sem_post");
        }
        temporary = temporary->next;
    }

    pthread_rwlock_unlock(&lock); //unlock
}

//-----------------------------------------------------------------------------

void reads_list_decrement(unsigned int client_number)
{
    (void) client_number; //! Please remove this when you implement this function
    //! please implement this function
}

//-----------------------------------------------------------------------------

int reads_list_remove_reader(unsigned int client_number)
{
    pthread_rwlock_rdlock(&lock); //lock

    //! find element to remove
    reads_list_element_t* temporary = head;
    while(temporary != NULL && temporary->client_number != client_number)
    {
        temporary = temporary->next;
    }

    pthread_rwlock_unlock(&lock); //unlock

    if(temporary == NULL)
    {
        return -1;
    }

    pthread_rwlock_rdlock(&lock); //lock

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

    pthread_rwlock_unlock(&lock); //unlock

    //! finally delete element
    free(temporary);
    return 0;
}

//-----------------------------------------------------------------------------

int reads_list_get_reads(unsigned int client_number)
{
    int buffer = 0;
    
    sem_t *this_sem = reads_list_get_reader_semaphore(client_number);
    sem_getvalue(this_sem, &buffer);

    return buffer;
}
