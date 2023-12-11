#include "reads_list.h"
#include "pthread.h"
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

typedef struct reads_list_element
{
    // Set readslist element variables
    int counter;
    sem_t semaphore;
    unsigned int client_number;
    struct reads_list_element* next;
    struct reads_list_element* previous;

} reads_list_element_t;

reads_list_element_t* head = NULL;
// Lock for the double linked list
pthread_rwlock_t list_lock = PTHREAD_RWLOCK_INITIALIZER;

//-----------------------------------------------------------------------------

int reads_list_insert_element(unsigned int client_number)
{
    //! create new element
    reads_list_element_t* new_element = malloc(sizeof(reads_list_element_t));
    if (new_element == NULL)
    {
        perror("malloc");
        exit(EXIT_FAILURE);
    }

    // Lock thread
    pthread_rwlock_wrlock(&list_lock);

    // Set default values
    new_element->client_number = client_number;
    new_element->counter       = 0;

    if (sem_init(&new_element->semaphore, 0, 1) < 0)
    {
        perror("sem_init");
        exit(EXIT_FAILURE);
    }

    //! insert element into list
    if (head == NULL)
    {
        new_element->next     = NULL;
        new_element->previous = NULL;
        head                  = new_element;

        // Unlock before returning
        pthread_rwlock_unlock(&list_lock);
        return 0;
    }

    reads_list_element_t* temporary = head;
    while (temporary->next != NULL)
    {
        temporary = temporary->next;
    }
    new_element->next     = NULL;
    new_element->previous = temporary;
    temporary->next       = new_element;

    // Unlock
    pthread_rwlock_unlock(&list_lock);

    return 0;
}

//-----------------------------------------------------------------------------

sem_t* reads_list_get_reader_semaphore(unsigned int client_number)
{
    // Lock thread
    pthread_rwlock_wrlock(&list_lock);

    reads_list_element_t* elem = head;

    // search for the client
    while (elem->client_number != client_number && elem == NULL)
    {
        elem = elem->next;
    }

    // Unlock thread again
    pthread_rwlock_unlock(&list_lock);

    // Dont decrement if there is no element
    if (elem == NULL)
    {
        return NULL;
    }
    return &elem->semaphore;
}

//-----------------------------------------------------------------------------

void reads_list_increment_all()
{
    // Lock thread
    pthread_rwlock_wrlock(&list_lock);

    reads_list_element_t* temporary = head;
    while (temporary != NULL)
    {
        // Lock to increade the counter
        sem_wait(&temporary->semaphore);
        temporary->counter++;
        sem_post(&temporary->semaphore);
        // Switch to next list element
        temporary = temporary->next;
    }
    // Unlock thread
    pthread_rwlock_unlock(&list_lock);
}

//-----------------------------------------------------------------------------

void reads_list_decrement(unsigned int client_number)
{
    // Lock thread
    pthread_rwlock_wrlock(&list_lock);

    reads_list_element_t* elem = head;

    // search for the client
    while (elem->client_number != client_number && elem != NULL)
    {
        elem = elem->next;
    }

    // Unlock thread again
    pthread_rwlock_unlock(&list_lock);

    // Dont decrement if there is no element
    if (elem == NULL)
    {
        return;
    }

    // decrement the counter once possible
    sem_wait(&elem->semaphore);
    elem->counter--;
    sem_post(&elem->semaphore);
}

//-----------------------------------------------------------------------------

int reads_list_remove_reader(unsigned int client_number)
{
    // Lock thread
    pthread_rwlock_wrlock(&list_lock);

    //! find element to remove
    reads_list_element_t* temporary = head;
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
    if (temporary == head)
    {
        head = temporary->next;
    }

    // Unlock thread
    pthread_rwlock_unlock(&list_lock);
    
    free(temporary);
    return 0;
}

//-----------------------------------------------------------------------------

int reads_list_get_reads(unsigned int client_number)
{
    int buffer = 0;

    // Lock thread
    pthread_rwlock_wrlock(&list_lock);

    reads_list_element_t* temp = head;
    // iterate the list until the element with the proper client_number is found
    // or until the list's end
    while (temp != NULL && temp->client_number != client_number)
    {
        temp = temp->next;
    }
    // Unlock thread
    pthread_rwlock_unlock(&list_lock);
    if (temp == NULL)
    {
        return 0;
    }

    // Get buffer after semaphose is unlocked
    sem_wait(&temp->semaphore);
    buffer = temp->counter;
    sem_post(&temp->semaphore);

    return buffer;
}
