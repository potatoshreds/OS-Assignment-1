#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>

// Create a signal handler function
void handler(int sig) {
    if (sig == SIGHUP) {
        printf("Ouch!\n");
    }
    else if (sig == SIGINT) {
        printf("Yeah!\n");
    }
    fflush(stdout);
}

int main(int ac, char *av[]) {
    if (ac != 2) {
        fprintf(stderr, "Usage: %s <n>\n", av[0]); 
        return 1;
    }

    int n = atoi(av[1]);

    if (n < 0) { 
        fprintf(stderr, "n must be >= 0\n"); 
        return 1;
    }
    
    // Register the signal processing functions
    signal(SIGHUP, handler); 
    signal(SIGINT, handler); 

    // Prints the first “n” even numbers
    for (int i = 0; i < n; i++) {
        printf("%d\n", i * 2); // i * 2 can help it print even numbers
        fflush(stdout);
        sleep(5);// Place a sleep(5) after the print statement
    }

    return 0 ;
}

