#include <stdio.h>

int main() {
    printf("Hello, World!\n");

    // How to write a shell
    // Read in input

    // read in cmd

    // parse cmd (built-in or user program, whether has file descriptor)

    // if built-in, no fork. search in the built in list to make sure the cmd in built-ins

    // support

    // if user program, fork process

    // for child, run
    // (with execvp)

    // for parent (shell), wait until the child exits.


    // Loop back

    return 0;
}

// built in cd, pwd, exit
// cd with chdir(). Needs the argument from getenv("HOME")

// pwd. Use getcwd()

// exit (call exist);