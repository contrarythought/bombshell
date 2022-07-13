#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
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

void execute_cmd(char *cmd)
{
    // cmd = command + path
    char *saved_cmd = strdup(cmd);
    if (!saved_cmd)
        err(errno);

    char *command = NULL;
    char *path = NULL;

    command = strsep(&cmd, " ");
    if (!command)
        return;

    // execute built-in commands
    if (!strncmp(command, "exit", 4))
    {
        free(saved_cmd);
        exit(EXIT_SUCCESS);
    }

    else if (!strncmp(command, "cd", 2))
    {
        path = strsep(&cmd, " ");
        if (chdir(path) == -1)
            err(errno);
    }

    else if (!strncmp(command, "mkdir", 5))
    {
        path = strsep(&cmd, "\n");
        if (mkdir(path, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH) == -1)
            err(errno);
    }

    else
    {
        //  path might have mulitple space-separated commands
        char **args = (char **)malloc(sizeof(char *));
        if (!args)
            err(errno);

        // step through each arg and save it
        args[0] = strdup(command);
        int i;
        char *arg = NULL;
        for (i = 1; cmd; i++)
        {
            arg = strsep(&cmd, " ");
            args = (char **)realloc(args, i + 1);
            if (!args)
                err(errno);
            args[i] = strdup(arg);
            if (!args[i])
                err(errno);
        }
        // not 100% sure if this is necessary
        args = (char **)realloc(args, i + 1);
        if (!args)
            err(errno);
        args[i] = NULL;

        // execute command in separate process
        int child = fork();

        if (child < 0)
            err(errno);

        else if (child == 0)
        {
            if (execvp(command, args) == -1)
                err(errno);
        }

        else
        {
            wait(NULL);
            for (i = 0; args[i]; i++)
            {
                free(args[i]);
            }
            free(args);

            free(saved_cmd);
        }
    }
}

// TODO
void process_file(char *file)
{
}

void trim(char **tok)
{
    char *w = (*tok) + strlen(*tok) - 1;
    while (isspace(*w))
        w--;
    w++;
    *w = '\0';

    while ((**tok) == ' ')
        (*tok)++;
}

void process_cmd(char *cmd)
{
    char **cached_cmds = NULL;
    int len = 0;
    char *tok = NULL;

    // cache parallel commands
    if (strstr(cmd, "&"))
    {
        tok = strsep(&cmd, "&\n");

        int i;
        for (i = 0; *cmd; tok = strsep(&cmd, "&\n"), i++)
        {
            trim(&tok);
            cached_cmds = (char **)realloc(cached_cmds, sizeof(char *) * (i + 1));
            if (!cached_cmds)
                err(errno);

            cached_cmds[i] = strdup(tok);
            if (!cached_cmds[i])
                err(errno);
        }
        trim(&tok);
        cached_cmds = (char **)realloc(cached_cmds, sizeof(char *) * (i + 1));
        if (!cached_cmds)
            err(errno);
        cached_cmds[i] = strdup(tok);
        if (!cached_cmds[i])
            err(errno);

        len = i + 1;

        int child;
        for (i = 0; i < len; i++)
        {
            child = fork();
            if (!child)
            {
                execute_cmd(cached_cmds[i]);
                exit(EXIT_SUCCESS);
            }

            else
                wait(NULL);
        }
    }

    else
    {
        trim(&cmd);
        execute_cmd(cmd);
    }

    if (cached_cmds)
    {
        int i;
        for (i = 0; i < len; i++)
        {
            free(cached_cmds[i]);
        }
        free(cached_cmds);
    }
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

            process_cmd(input);
        }
    }

    return EXIT_SUCCESS;
}
