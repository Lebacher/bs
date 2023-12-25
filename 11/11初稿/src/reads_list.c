#include "reads_list.h"
#include "pthread.h"
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
	
	sem_t reads;

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

    if (sem_init(&(new_element->reads), 0, 0) == -1) {
        perror("Sem_init failed.");
        exit(EXIT_FAILURE);
    }

    if (pthread_rwlock_wrlock(&reads_list_lock) != 0) {
        perror("Pthread_rwlock_wrlock failed.");
        exit(EXIT_FAILURE);
    }

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

    if (pthread_rwlock_unlock(&reads_list_lock) != 0) {
        perror("pthread_rwlock_unlock failed.");
        exit(EXIT_FAILURE);
    }

    return 0;
}

//-----------------------------------------------------------------------------

sem_t* reads_list_get_reader_semaphore(unsigned int client_number)
{
    //(void) client_number; //! Please remove this when you implement this function
                       
    //! please implement this function
    

    if (pthread_rwlock_rdlock(&reads_list_lock) != 0) {
        perror("pthread_rwlock_rdlock failed.");
        exit(EXIT_FAILURE);
    }

    reads_list_element_t* temporary = head;
    while (temporary->client_number != client_number && temporary != NULL) {
        temporary = temporary->next;
    }

    if (pthread_rwlock_unlock(&reads_list_lock) != 0) {
        perror("pthread_rwlock_unlock failed.");
        exit(EXIT_FAILURE);
    }

    if (temporary == NULL) {
        return NULL;
    }

    return &temporary->reads;
    //return NULL; //! Please select a proper return value
                 
}

//-----------------------------------------------------------------------------

void reads_list_increment_all()
{
    //! HINT: synchronization is needed in this function
    

    if (pthread_rwlock_rdlock(&reads_list_lock) != 0) {
        perror("Pthread_rwlock_rdlock failed.");
        exit(EXIT_FAILURE);
    }

    reads_list_element_t* temporary = head;
    while(temporary != NULL)
    {
        if (sem_post(&temporary->reads) != 0)
        {
            perror("Sem_post failed.");
            exit(EXIT_FAILURE);
        }
        temporary = temporary->next;
    }

    if (pthread_rwlock_unlock(&reads_list_lock) != 0) {
        perror("pthread_rwlock_unlock failed.");
        exit(EXIT_FAILURE);
    }
}

//-----------------------------------------------------------------------------

void reads_list_decrement(unsigned int client_number)
{
    //(void) client_number; //! Please remove this when you implement this function
                            
    //! please implement this function
    

    if (pthread_rwlock_wrlock(&reads_list_lock) != 0) {
        perror("pthread_rwlock_wrlock failed.");
        exit(EXIT_FAILURE);
    }

    reads_list_element_t* temporary = head;
    while (temporary != NULL && temporary->client_number != client_number)
    {
        temporary = temporary->next;
    }

    if (pthread_rwlock_unlock(&reads_list_lock) != 0) {
        perror("pthread_rwlock_unlock failed.");
        exit(EXIT_FAILURE);
    }
	
	if (reads_list_get_reads(client_number) == 0) {
        return;
    }

    if (temporary != NULL) {
        sem_wait(&temporary->reads);
    }
}
//-----------------------------------------------------------------------------

int reads_list_remove_reader(unsigned int client_number)
{
    //! HINT: synchronization is needed in this function
    

    if (pthread_rwlock_rdlock(&reads_list_lock) != 0) {
        perror("pthread_rwlock_rdlock failed.");
        exit(EXIT_FAILURE);
    }
    //! find element to remove
    
    reads_list_element_t* temporary = head;
    while(temporary != NULL && temporary->client_number != client_number)
    {
        temporary = temporary->next;
    }

    if (pthread_rwlock_unlock(&reads_list_lock) != 0) {
        perror("pthread_rwlock_unlock failed.");
        exit(EXIT_FAILURE);
    }

    if(temporary == NULL)
    {
        return -1;
    }

    if (pthread_rwlock_rdlock(&reads_list_lock) != 0) {
        perror("pthread_rwlock_rdlock failed.");
        exit(EXIT_FAILURE);
    }
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

    if (pthread_rwlock_unlock(&reads_list_lock) != 0) {
        perror("pthread_rwlock_unlock failed.");
        exit(EXIT_FAILURE);
    }

    //! finally delete element
    
    free(temporary);
    return 0;
}

//-----------------------------------------------------------------------------

int reads_list_get_reads(unsigned int client_number)
{
    //(void) client_number; //! Please remove this when you implement this function
    
    //! please implement this function
    
    int buffer = 0;
	
    if (pthread_rwlock_rdlock(&reads_list_lock) != 0) {
        perror("pthread_rwlock_rdlock failed.");
        exit(EXIT_FAILURE);
    }

    if (sem_getvalue(reads_list_get_reader_semaphore(client_number),
                     &buffer) != 0){
        perror("Sem_getvalue failed.");
        exit(EXIT_FAILURE);
    }

    if (pthread_rwlock_unlock(&reads_list_lock) != 0) {
        perror("pthread_rwlock_unlock failed.");
        exit(EXIT_FAILURE);
    }

    return buffer;
}
