#include "main.h"

int main() {
    printf("Launching user threads demo\n");
    int exit = demo();
    if (!exit) {
        printf("Demo successfully exited\n");
    } else {
        printf("Demo exited with error code %d\n", exit);
    }
}