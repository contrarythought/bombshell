#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

// TODO
void process_file(char *file) {

}

int main(int argc, char *argv[]) 
{

    // process batch commands via file ex-stdin
    if (argc == 2) {

        process_file(argv[1]);

    } 

    // process stdin commands
    else {

        char *input = NULL;
        size_t len = 0;
        ssize_t nread = 0;

        while(1) {

            printf("bombshell> ");

            if ((nread = getline(&input, &len, stdin)) == -1) {

                fprintf(stderr, "err: failed to read input");

                free(input);

                exit(EXIT_FAILURE);

            }

            // parse command - NOTE: strsep modifies input by updating input to point past the token returned
            char *command = strsep(&input, " ");

            // path - setting it to the input modified above
            char *path = NULL;

            // process built-in commands
            if (!strncmp(command, "exit\n", strlen("exit\n"))) {

                exit(EXIT_SUCCESS);

            } 

            // TODO
            else if (!strncmp(command, "cd", strlen("cd"))) {

                // get location to cd into
                path = strsep(&input, "\n");

            } 

        }

    }

    return EXIT_SUCCESS;
}
