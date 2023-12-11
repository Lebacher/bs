#include <termios.h>

#include "client.h"
#include "utils.h"

#include <fcntl.h>
#include <sys/ioctl.h>

char* prompt_user_input(char* buffer, size_t size)
{
    //    enable_input_echo();
    prompt_print();

    // get user input
    clear_user_input();
    char* user_input = fgets(buffer, size, stdin);
    prompt_clear();
    fflush(stdout);
    //    disable_input_echo();
    if(user_input == NULL)
    {
        if(feof(stdin))
        {
            printf("closing client\n");
            close_connection();
            exit(EXIT_SUCCESS);
        }
        else
        {
            fprintf(stderr, "ERROR: fgets failed");
            exit(EXIT_FAILURE);
        }
    }

    // ensure user input is in any case nullterminated
    user_input[size - 1] = '\0';

    return user_input;
}

//-------------------------------------------------------------

void prompt_print()
{
    printf(CLEARLINE PROMPT);
    fflush(stdout);
}

//-------------------------------------------------------------

void prompt_clear()
{
    printf(CURSORUP1 CLEAREOL);
}

//-------------------------------------------------------------

void prompt_error()
{
    printf(ERROR_MESSAGE);
    getchar();
    prompt_clear();
}

//-------------------------------------------------------------

uint16_t get_port_number() {
    u_int16_t port = 0;
    const char* username = getenv("USER");
    if (username == NULL)
    {
        return 31026;
    }
    char c;
    while ((c = *username) != '\0') {
        port = (port << 1) + port +(u_int16_t)c;
        username++;
    }
    return 31026 + (port % 4096);
}

//-------------------------------------------------------------

void signal_handler(int sig)
{
    // close all threads and the socket by receiving a signal
    close_connection();
    printf("\n");
    exit(sig);
}

//-------------------------------------------------------------

void initialize_signal_handler(void)
{
    signal(SIGINT, signal_handler);  // No  2 interrupt
    signal(3, signal_handler);       // No  3 quit
    signal(SIGILL, signal_handler);  // No  4 ungültige anweisung
    signal(SIGABRT, signal_handler); // No  6 abnormale prozessterminierung
    signal(SIGFPE, signal_handler);  // No  8 floating point exception
    signal(SIGSEGV, signal_handler); // No 11 segfault
    signal(SIGTERM, signal_handler); // No 15 terminierung
}

//-------------------------------------------------------------

void exit_client(int exit_code)
{
    close_connection();
    exit(exit_code);
}

//-------------------------------------------------------------

void clear_user_input()
{
    int fd = open(ttyname(STDIN_FILENO), O_RDONLY);
    ioctl(fd, TCFLSH, TCIFLUSH);
    close(fd);
}

//-------------------------------------------------------------

void print_reply(char* message)
{
    printf(CLEARLINE "%s\n", message);
    prompt_print();
}
