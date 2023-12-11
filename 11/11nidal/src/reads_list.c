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
    sem_t read_list;
    unsigned int client_number;
    struct reads_list_element* next;
    struct reads_list_element* previous;

} reads_list_element_t;

reads_list_element_t* head = NULL;
pthread_rwlock_t list_lock = PTHREAD_RWLOCK_INITIALIZER;

//! HINT: maybe global synchronization variables are needed

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
    sem_init(&(new_element->read_list), 1, 0);  // FB Hier wäre pshared = 0 richtig, da die Semaphore nur zwischen Threads, nicht aber zwischen Prozessen geteilt wird.

    if (pthread_rwlock_wrlock(&list_lock) == 0)
    {
        //! insert element into list
        if (head == NULL)
        {
            new_element->previous = NULL;
            head                  = new_element;
        }
        else
        {
            reads_list_element_t* temporary = head;
            while (temporary->next != NULL)
            {
                temporary = temporary->next;
            }
            new_element->previous = temporary;
            temporary->next       = new_element;
        }
        new_element->next = NULL;
        if (pthread_rwlock_unlock(&list_lock) != 0)
        {
            perror("unlock Error\n");   // FB Da perror an euren String ein ": <error message>\n" anfügt, ist der Zeilenumbruch hier ungünstig.
            exit(1);    // FB exit(EXIT_FAILURE) ist äquivalent und aussagekräftiger und verbessert daher das Verständnis des Codes
        }
    }
    else
    {
        perror("lock Error\n"); // FB Da perror an euren String ein ": <error message>\n" anfügt, ist der Zeilenumbruch hier ungünstig.
        exit(1);    // FB exit(EXIT_FAILURE) ist äquivalent und aussagekräftiger und verbessert daher das Verständnis des Codes
    }
    return 0;
}

//-----------------------------------------------------------------------------

sem_t* reads_list_get_reader_semaphore(unsigned int client_number)
{
    if (pthread_rwlock_rdlock(&list_lock) == 0)
    {
        reads_list_element_t* temporary = head;

        while (temporary != NULL && temporary->client_number != client_number)
        {
            temporary = temporary->next;
        }
        // FB Ihr könnt den kritischen Abschnitt bereits hier beenden.
        //
        if (temporary == NULL)
        {
            pthread_rwlock_unlock(&list_lock);
            return NULL;
        }
        if (pthread_rwlock_unlock(&list_lock) != 0)
        {
            perror("unlock Error\n");   // FB Da perror an euren String ein ": <error message>\n" anfügt, ist der Zeilenumbruch hier ungünstig.
            exit(1);    // FB exit(EXIT_FAILURE) ist äquivalent und aussagekräftiger und verbessert daher das Verständnis des Codes
        }

        return &temporary->read_list;
    }
    else
    {
        perror("lock Error\n"); // FB Da perror an euren String ein ": <error message>\n" anfügt, ist der Zeilenumbruch hier ungünstig.
        exit(1);    // FB exit(EXIT_FAILURE) ist äquivalent und aussagekräftiger und verbessert daher das Verständnis des Codes
    }
    return NULL;
}

//-----------------------------------------------------------------------------

void reads_list_increment_all()
{
    //! HINT: synchronization is needed in this function
    if (pthread_rwlock_wrlock(&list_lock) == 0) // FB Hier reicht ein readlock, da sem_post bereits synchronisiert ist
    {
        reads_list_element_t* temporary = head;

        while (temporary != NULL)
        {
            sem_post(&temporary->read_list);
            temporary = temporary->next;
        }
        if (pthread_rwlock_unlock(&list_lock) != 0)
        {
            perror("unlock Error\n");   // FB Da perror an euren String ein ": <error message>\n" anfügt, ist der Zeilenumbruch hier ungünstig.
            exit(1);    // FB exit(EXIT_FAILURE) ist äquivalent und aussagekräftiger und verbessert daher das Verständnis des Codes
        }
    }
    else
    {
        perror("lock Error\n"); // FB Da perror an euren String ein ": <error message>\n" anfügt, ist der Zeilenumbruch hier ungünstig.
        exit(1);    // FB exit(EXIT_FAILURE) ist äquivalent und aussagekräftiger und verbessert daher das Verständnis des Codes
    }
}

//-----------------------------------------------------------------------------

void reads_list_decrement(unsigned int client_number)
{
    if (pthread_rwlock_wrlock(&list_lock) == 0)
    {
        reads_list_element_t* temporary = head;
        while (temporary != NULL && temporary->client_number != client_number)
        {
            temporary = temporary->next;
        }

        if (pthread_rwlock_unlock(&list_lock) != 0)
        {
            perror("unlock Error\n");   // FB Da perror an euren String ein ": <error message>\n" anfügt, ist der Zeilenumbruch hier ungünstig.
            exit(1);    // FB exit(EXIT_FAILURE) ist äquivalent und aussagekräftiger und verbessert daher das Verständnis des Codes
        }
        if (reads_list_get_reads(client_number) == 0)   // FB Einfacher: sem_getvalue(&temporary->read_list) == 0
        {
            return;
        }

        sem_wait(&temporary->read_list);
    }
    else
    {
        perror("lock Error\n"); // FB Da perror an euren String ein ": <error message>\n" anfügt, ist der Zeilenumbruch hier ungünstig.
        exit(1);    // FB exit(EXIT_FAILURE) ist äquivalent und aussagekräftiger und verbessert daher das Verständnis des Codes
    }
}

//-----------------------------------------------------------------------------

int reads_list_remove_reader(unsigned int client_number)
{
    //! HINT: synchronization is needed in this function

    //! find element to remove
    if (pthread_rwlock_wrlock(&list_lock) == 0)
    {
        // FB Für die Iteration reicht ein Readlock, das Writelock wird erst für das Verändern der Pointer benötigt.
        reads_list_element_t* temporary = head;
        while (temporary != NULL && temporary->client_number != client_number)
        {
            temporary = temporary->next;
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
        //! finally delete element
        // FB Hier hättet ihr die Semaphore freigeben müssen.
        free(temporary);    // FB Das free kann außerhalb des Writelocks durchgeführt werden -1P
        if (pthread_rwlock_unlock(&list_lock) != 0)
        {
            perror("unlock Error\n");   // FB Da perror an euren String ein ": <error message>\n" anfügt, ist der Zeilenumbruch hier ungünstig.
            exit(1);    // FB exit(EXIT_FAILURE) ist äquivalent und aussagekräftiger und verbessert daher das Verständnis des Codes
        }
    }
    else
    {
        perror("lock Error\n"); // FB Da perror an euren String ein ": <error message>\n" anfügt, ist der Zeilenumbruch hier ungünstig.
        exit(1);    // FB exit(EXIT_FAILURE) ist äquivalent und aussagekräftiger und verbessert daher das Verständnis des Codes
    }

    return 0;
}

//-----------------------------------------------------------------------------

int reads_list_get_reads(unsigned int client_number)
{
    int buffer;
    if (pthread_rwlock_rdlock(&list_lock) == 0)
    {
        reads_list_element_t* temporary = head;
        while (temporary != NULL && temporary->client_number != client_number)
        {
            temporary = temporary->next;
        }
        buffer = -1;
        sem_getvalue(&temporary->read_list, &buffer);
        if (pthread_rwlock_unlock(&list_lock) != 0)
        {
            perror("unlock Error\n");   // FB Da perror an euren String ein ": <error message>\n" anfügt, ist der Zeilenumbruch hier ungünstig.
            exit(1);    // FB exit(EXIT_FAILURE) ist äquivalent und aussagekräftiger und verbessert daher das Verständnis des Codes
        }
    }
    else
    {
        perror("lock Error\n"); // FB Da perror an euren String ein ": <error message>\n" anfügt, ist der Zeilenumbruch hier ungünstig.
        exit(1);    // FB exit(EXIT_FAILURE) ist äquivalent und aussagekräftiger und verbessert daher das Verständnis des Codes
    }
    return buffer;
}
