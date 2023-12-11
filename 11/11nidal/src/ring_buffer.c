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
    int reader_counter;

} ring_buffer_element_t;

static ring_buffer_element_t ring_buffer[RINGBUFFER_SIZE];

unsigned int current_writer = 0;

unsigned int number_of_readers = 0;

//! HINT: maybe global synchronization variables are needed
pthread_mutex_t writer_mutex = PTHREAD_MUTEX_INITIALIZER;
//-----------------------------------------------------------------------------

int ringbuffer_write(char* text)
{

    if (pthread_mutex_lock(&writer_mutex) == 0)
    {
        // writing failed
        if (ring_buffer[current_writer % RINGBUFFER_SIZE].reader_counter != 0)
        {
            // FB In diesem Fall sollte kurz gewartet werden und erst wenn dann kein freier Buffer verfügbar ist abgebrochen werden -2P
            pthread_mutex_unlock(&writer_mutex);
            return -1;
        }
        strcpy(ring_buffer[current_writer % RINGBUFFER_SIZE].text, text);

        // reads_list_increment

        ring_buffer[current_writer % RINGBUFFER_SIZE].reader_counter =
            number_of_readers;
        reads_list_increment_all();
        current_writer++;
        if (pthread_mutex_unlock(&writer_mutex) != 0)
        {
            perror("unlock Error\n");   // FB Da perror an euren String ein ": <error message>\n" anfügt, ist der Zeilenumbruch hier ungünstig.
            exit(1);    // FB exit(EXIT_FAILURE) ist äquivalent und aussagekräftiger und verbessert daher das Verständnis des Codes
        }
    }
    else    // FB Euer Code wird übersichtlicher, wenn ihr auf den Fehler prüft, und wenn dessen Behandlung mit einem return/exit abschließt,
            // FB auf das else verzichtet und normal fortfahrt. Dann braucht ihr weniger Einrückungen und es Fehlerbehandlungen sind leichter als solche zu erkennen.
            // FB if (error) { handle error; exit/return} continue with normal behaviour
    {
        perror("lock Error\n"); // FB Da perror an euren String ein ": <error message>\n" anfügt, ist der Zeilenumbruch hier ungünstig.
        exit(1);    // FB exit(EXIT_FAILURE) ist äquivalent und aussagekräftiger und verbessert daher das Verständnis des Codes
    }

    return 0;
}

//-----------------------------------------------------------------------------

void ringbuffer_read(int* current_reader, char* buffer,
                     unsigned int client_number)
{
    // FB Hier fehlt Synchronisation
    int reader = *current_reader;
    //! HINT: Check if thread can read a new element & synchronization will be
    //! needed
    /* ... */
    if (reads_list_get_reads(client_number) == 0)
    {
        // FB In diesem Fall sollte kurz gewartet werden und erst wenn dann kein freier Buffer verfügbar ist abgebrochen werden -2P
        strcpy(buffer, "nack");
        return;
    }
    ring_buffer[reader % RINGBUFFER_SIZE].reader_counter--; // FB Hier können mehrere Threads gleichzeitig dekrementieren. -1P
                                                            // FB Da das Dekrementieren nicht atomar ist, kann es vorkommen,
                                                            // FB dass ein Dekrement verloren geht, was zu einem Deadlock führt.

    // FB Wenn der Thread vor dem strcpy unterbrochen wird, kann der noch nicht gelesene Text überschrieben werden. -1P

    reads_list_decrement(client_number);

    //! Read Element
    strcpy(buffer, (const char*)ring_buffer[reader % RINGBUFFER_SIZE].text);

    //! HINT: notify the writer

    //! Update reader count
    (*current_reader)++;
    return;
}

//-----------------------------------------------------------------------------

int ringbuffer_add_reader(unsigned int client_number)
{
    //! HINT: synchronization is needed in this function
    int new_reader;
    if (pthread_mutex_lock(&writer_mutex) == 0)
    {
        if (reads_list_insert_element(client_number) != 0)
        {
            exit(EXIT_FAILURE);
        }
        new_reader = current_writer;
        number_of_readers++;

        if (pthread_mutex_unlock(&writer_mutex) != 0)
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

    return new_reader;
}

//-----------------------------------------------------------------------------

void ringbuffer_remove_reader(int* current_reader, unsigned int client_number)
{
    //! HINT: synchronization is needed in this function

    int read = reads_list_get_reads(client_number);

    //! perform all unfinished reads for the disconnected client
    while (read != 0)
    {
        char buffer[MAX_MESSAGE_LENGTH];
        ringbuffer_read(current_reader, buffer, client_number);
        read = reads_list_get_reads(client_number);
    }
    // FB Wenn bis zum Lock ein anderer Thread ein write durchführt, kommt es zum Deadlock -1P
    reads_list_remove_reader(client_number);
    if (pthread_mutex_lock(&writer_mutex) == 0)
    {
        number_of_readers--;
        if (pthread_mutex_unlock(&writer_mutex) != 0)
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
    return;
}
