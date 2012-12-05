#include <iostream>

#include <pthread.h>

int main(int argc, char **argv) {
    if (argc > 1) {
        printf("Argument %s\n", argv[1]);
    } else {
        printf("Hello World\n");
    }
    return 0;
}
