#include "client.h"

int main(void)
{

    // Task .1
    connect_to_server();

    // Task .2
    handshake();

    // Task .6
    start_reader_thread();

    // Task .3 to .5
    send_message();

    //this part should never be reached
    close_connection();
    return EXIT_FAILURE;
}
