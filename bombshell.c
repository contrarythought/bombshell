#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/syscall.h>

#define START_PATH "/bin"

void err(int e)
{
    switch (e)
    {
    case EACCES:
        printf("search permission is denied for one of the components of path\n");
        break;
    case EFAULT:
        printf("path points outside your accessible address space\n");
        break;
    case EIO:
        printf("An I/O error occurred\n");
        break;
    case ELOOP:
        printf("too many symbolic links were encountered in resolving path\n");
        break;
    case ENAMETOOLONG:
        printf("path is too long\n");
        break;
    case ENOENT:
        printf("the directory specified in path does not exist\n");
        break;
    case ENOMEM:
        printf("insufficient kernel memory was available\n");
        break;
    case ENOTDIR:
        printf("a component of path is not a directory");
        break;
    case EBADF:
        printf("pathname is relative but dirfd is neither AT_FDCWD nor a valid file descriptor");
        break;
    case EDQUOT:
        printf("the user's quota of disk blocks or inodes on the filesystem has been exhausted\n");
        break;
    case EEXIST:
        printf("pathname already exists (not necessarily as a directory). This includes the case where pathname is a symbolic link, dangling or not.\n");
        break;
    case EINVAL:
        printf("the final component (\"basename\") of the new directory's pathname is invalid (e.g., it contains characters not permitted by the underlying filesystem.\n");
        break;
    case EMLINK:
        printf("The number of links to the parent directory would exceed LINK_MAX.\n");
        break;
    case ENOSPC:
        printf("the device containing pathname has no room for the new directory.\n");
        break;
    case EPERM:
        printf("the filesystem containing pathname does not support the creation of directories.\n");
        break;
    case EROFS:
        printf("pathname refers to a file on a read-only filesystem.\n");
        break;
    }
}
// TODO
void process_file(char *file)
{
}

int main(int argc, char *argv[])
{
    // process batch commands via file ex-stdin
    if (argc == 2)
        process_file(argv[1]);

    // process stdin commands
    else
    {
        char *input = NULL;
        size_t len = 0;
        ssize_t nread = 0;
        char cwd[100];

        while (1)
        {
            getcwd(cwd, sizeof(cwd));
            printf("bombshell:~%s$ ", cwd);

            if ((nread = getline(&input, &len, stdin)) == -1)
            {
                fprintf(stderr, "err: failed to read input");

                free(input);

                exit(EXIT_FAILURE);
            }

            // save full input
            char *save_input = strdup(input);

            // parse command - NOTE: strsep modifies input by updating input to point past the token returned
            char *command = strsep(&input, " ");

            char *path = NULL;

            // process built-in commands
            if (!strncmp(command, "exit\n", strlen("exit\n")))
                exit(EXIT_SUCCESS);

            else if (!strncmp(command, "cd", strlen("cd")))
            {
                // get location to cd into
                path = strsep(&input, "\n");
                int e = chdir(path);
                if (e == -1)
                    err(errno);
            }

            else if (!strncmp(command, "mkdir", strlen("mkdir")))
            {
                path = strsep(&input, "\n");
                int e = mkdir(path, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
                if (e == -1)
                    err(errno);
            }

            // create new process to execute non built-in command
            else
            {
                path = strsep(&input, "\n");
                char **args = NULL;
                char *tok = NULL;
                args = (char **)malloc(sizeof(char *));
                if (!args)
                    err(errno);

                // tokenize path
                if (path)
                {
                    args[0] = strdup(command);
                    if (!args[0])
                        err(errno);

                    int i;
                    for (i = 1; path; i++)
                    {
                        tok = strsep(&path, " ");
                        args = (char **)realloc(args, sizeof(char *) * (i + 1));
                        args[i] = strdup(tok);
                        if (!args)
                            err(errno);
                    }
                    args[i] = NULL;
                }

                else
                {
                    // cut off \n if no args exist
                    command[strlen(command) - 1] = '\0';
                    args[0] = strdup(command);
                    if (!args[0])
                        err(errno);
                }

                int child = fork();

                if (child < 0)
                    err(errno);

                else if (child == 0)
                {
                    if (execvp(args[0], args) == -1)
                    {
                        printf("Failed to execute\n");
                    }
                }

                else
                {
                    wait(NULL);

                    int i;
                    for (i = 0; args[i]; i++)
                    {
                        free(args[i]);
                    }
                    
                    free(args);
                }
            }

            free(save_input);
        }
    }

    return EXIT_SUCCESS;
}
