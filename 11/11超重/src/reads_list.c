#include "../include/reads_list.h"

#include <errno.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

//! HINT: when designing the reads_list_element remember to
//! HINT: keep the critical region as small as necessary.
typedef struct reads_list_element
{
    //! HINT: something is missing here
    unsigned int client_number;
    sem_t* to_read;
    struct reads_list_element* next;
    struct reads_list_element* previous;

} reads_list_element_t;

reads_list_element_t* head = NULL;

//! HINT: maybe global synchronization variables are needed

static pthread_mutex_t reads_list_mutex = PTHREAD_MUTEX_INITIALIZER;

//-----------------------------------------------------------------------------

int reads_list_insert_element(unsigned int client_number)
{
    //! HINT: synchronization is needed in this function

    //! create new element

    reads_list_element_t* new_element = malloc(sizeof(reads_list_element_t));
    if (new_element == NULL)
    {
        perror("malloc");
        exit(EXIT_FAILURE);
    }

    new_element->client_number = client_number;
    new_element->to_read       = malloc(sizeof(sem_t));
    if (new_element->to_read == NULL)
    {
        perror("malloc");
        exit(-1);
    }
    if (sem_init(new_element->to_read, 0, 0) < 0)
    {
        perror("ERROR sem_init");
        exit(-1);
    }
    new_element->next     = NULL;
    new_element->previous = NULL;

    //! insert element into list
    pthread_mutex_lock(&reads_list_mutex);
    if (head == NULL)
    {
        head = new_element;
        pthread_mutex_unlock(&reads_list_mutex);
        return 0;
    }

    reads_list_element_t* temporary = head;
    pthread_mutex_unlock(&reads_list_mutex);
    while (temporary->next != NULL)
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
    // (void) client_number; //! Please remove this when you implement this
    // function
    //! please implement this function

    pthread_mutex_lock(&reads_list_mutex);
    reads_list_element_t* temporary = head;
    pthread_mutex_unlock(&reads_list_mutex);
    while (temporary->client_number != client_number)
    {
        temporary = temporary->next;
    }
    return temporary->to_read;

    // return NULL; //! Please select a proper return value
}

//-----------------------------------------------------------------------------

void reads_list_increment_all()
{
    //! HINT: synchronization is needed in this function

    reads_list_element_t* temporary = head;
    while (temporary != NULL)
    {
        if (sem_post(temporary->to_read) < 0)
        {
            perror("ERROR sem_post");
            exit(-1);
        }
        temporary = temporary->next;
    }
}

//-----------------------------------------------------------------------------

void reads_list_decrement(unsigned int client_number)
{
    // (void) client_number; //! Please remove this when you implement this
    // function
    //! please implement this function

    sem_trywait(reads_list_get_reader_semaphore(client_number));
}

//-----------------------------------------------------------------------------

int reads_list_remove_reader(unsigned int client_number)
{
    //! HINT: synchronization is needed in this function

    //! find element to remove
    pthread_mutex_lock(&reads_list_mutex);
    reads_list_element_t* temporary = head;
    pthread_mutex_unlock(&reads_list_mutex);
    while (temporary != NULL && temporary->client_number != client_number)
    {
        temporary = temporary->next;
    }

    if (temporary == NULL)
    {
        return -1;
    }

    //! bend pointers around element
    if (temporary->previous != NULL)
    {
        temporary->previous->next = temporary->next;
    }
    if (temporary->next != NULL)
    {
        temporary->next->previous = temporary->previous;
    }
    pthread_mutex_lock(&reads_list_mutex);
    if (temporary == head)
    {
        head = temporary->next;
    }
    pthread_mutex_unlock(&reads_list_mutex);

    //! finally delete element
    free(temporary);
    return 0;
}

//-----------------------------------------------------------------------------

int reads_list_get_reads(unsigned int client_number)
{
    int buffer = 0;

    // (void) client_number; //! Please remove this when you implement this
    // function
    //! please implement this function

    if (sem_getvalue(reads_list_get_reader_semaphore(client_number), &buffer) <
        0)
    {
        perror("ERROR getvalue");
        exit(-1);
    }

    return buffer;
}
