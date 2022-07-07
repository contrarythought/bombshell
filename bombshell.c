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
void execute_cmd(char *cmd)
{
    char *saved_cmd = strdup(cmd);
    if (!saved_cmd)
        err(errno);

    char *command = NULL;
    char *path = NULL;

    command = strsep(&cmd, " \n");
    if (!command)
        return;

    // execute built-in commands
    if (!strncmp(command, "exit\n", 5))
    {
        free(saved_cmd);
        exit(EXIT_SUCCESS);
    }

    else if (!strncmp(command, "cd", 2))
    {
        path = strsep(&cmd, " \n");
        if (chdir(path) == -1)
            err(errno);
    }

    else if (!strncmp(command, "mkdir", 5))
    {
        path = strsep(&cmd, " \n");
        if (mkdir(path, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH) == -1)
            err(errno);
    }
}

// TODO
void process_file(char *file)
{
}

// TODO
void process_cmd(char *cmd)
{
    char **cached_cmds = NULL;
    pid_t *processes = NULL;

    char *tok = NULL;

    // cache parallel commands
    if (strstr(cmd, "&"))
    {
        tok = strsep(&cmd, "&");

        int i;
        for (i = 0; cmd; i++, tok = strsep(&cmd, "&"))
        {
            cached_cmds = (char **)realloc(cached_cmds, i + 1);
            if (!cached_cmds)
                err(errno);

            cached_cmds[i] = strdup(tok);
            if (!cached_cmds[i])
                err(errno);
        }

        // run the processes in parallel
        processes = (pid_t *)malloc(sizeof(pid_t) * i);
        if (!processes)
            err(errno);

        int j;
        for (j = 0; j < i; j++)
        {
            processes[j] = fork();
            if (processes[j] < 0)
                err(errno); // TODO - update err()

            else if (processes[j] == 0)
                break;
        }

        // process[j] breaks out of loop and executes cmd[j]
        if (processes[j] == 0)
        {
            execute_cmd(cached_cmds[j]);
            exit(EXIT_SUCCESS);
        }

        wait(NULL);
    }

    else
    {
        // TODO
    }

    if (cached_cmds)
    {
        int i;
        for (i = 0; cached_cmds[i]; i++)
        {
            free(cached_cmds[i]);
        }
        free(cached_cmds);
    }

    if (processes)
        free(processes);
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
            printf("in orig while\n");
            getcwd(cwd, sizeof(cwd));
            printf("bombshell:~%s$ ", cwd);

            if ((nread = getline(&input, &len, stdin)) == -1)
            {
                fprintf(stderr, "err: failed to read input");

                free(input);

                exit(EXIT_FAILURE);
            }

            process_cmd(input);

            /*
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


                        if (strncmp(tok, "&", 1))
                        {

                        }
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
            */
        } // end while
    }

    return EXIT_SUCCESS;
}
