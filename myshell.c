#include <sys/stat.h>
#include <sys/wait.h>
#include <fcntl.h>
#include "stdio.h"
#include "errno.h"
#include "stdlib.h"
#include "unistd.h"
#include <string.h>
#include <signal.h>
#include "linkedlist.h"
#include <termios.h>
#include <unistd.h>

#define MAX_COMMAND 1024

char command[MAX_COMMAND], last_command[MAX_COMMAND], current_command[MAX_COMMAND], prompt[MAX_COMMAND], current_command[MAX_COMMAND];
int command_counter, file_desc, amp, rv, piping, i, duplicate_fd, main_process, status = 0, pipe_fd[2], enter = 0;
List vars, comms;
pid_t pid, cpid;
char* end_if = "fi\n", *new_command, *new_command2, *output_file, *token;
char *argv[MAX_COMMAND];
char a, b, c;

int change_status(char **args);

int count_args(char **args)
{
    char **counter_args = args;
    int cnt = 0;
    for (cnt; *counter_args != NULL; cnt++)
    {
        counter_args++;
    }
    return cnt;
}

char **count_pipes(char **args)
{
    char **counter_pipes = args;
    while (*counter_pipes != NULL)
    {
        if (strcmp(*counter_pipes, "|") == 0)
        {
            return counter_pipes;
        }

        counter_pipes++;
    }
    return NULL;
}

int first_index_in_str(char *arr)
{
    int j;
    for (j = 0; i < MAX_COMMAND; i++)
    {
        if (strlen(arr) <= i)
        {
            break;
        }
    }
    return j;
}

char *safe_strcpy(char *dst, size_t size, char *src)
{
    if (size > 0)
    {
        *dst = '\0';
        return strncat(dst, src, size - 1);
    }
    return dst;
}

void termination_handler(int signum)
{
    if (getpid() == main_process)
    {
        printf("\n");
        printf("You typed Control-C!");
        printf("\n");
        write(0, prompt, strlen(prompt) + 1);
        return;
    }
    else
    {
        fprintf(stderr, "\ncaught signal: %d\n", signum);
        kill(main_process, SIGKILL);
        return;
    }
}

void split_command(char *command)
{
    char *part_of_command = strtok(command, " ");
    int i = 0;
    while (part_of_command != NULL)
    {
        argv[i] = part_of_command;
        part_of_command = strtok(NULL, " ");
        i++;
    }
    argv[i] = NULL;
}

int run_execute(char **args)
{
    rv = -1;
    piping = 0;
    i = count_args(args);
    char **count_pipes_pointer = count_pipes(args);
    if (count_pipes_pointer != NULL)
    {
        *count_pipes_pointer = NULL;
        piping++;
        int result = pipe(pipe_fd);
        if (result == -1)
        {
            printf("Failed to create pipe\n");
            exit(EXIT_FAILURE);
        }
        cpid = fork();
        if (cpid == -1)
        {
            perror("Error occurred: Failed to create new process\n");
            exit(EXIT_FAILURE);
        }
        else if (cpid == 0)
        {
            int status = close(pipe_fd[1]);
            if (status == -1)
            {
                printf("Failed to close file descriptor\n");
                exit(EXIT_FAILURE);
            }
            int status_file = close(0);
            if (status_file == -1)
            {
                printf("Failed to close standard input file descriptor\n");
                exit(EXIT_FAILURE);
            }
            int new_fd = dup(pipe_fd[0]);
            if (new_fd == -1)
            {
                printf("Failed to duplicate file descriptor\n");
                exit(EXIT_FAILURE);
            }
            run_execute(count_pipes_pointer + 1);
            exit(0);
        }
        duplicate_fd = dup(STDOUT_FILENO);
        if (duplicate_fd == -1)
        {
            printf("Failed to duplicate file descriptor\n");
            exit(EXIT_FAILURE);
        }
        dup2(pipe_fd[1], STDOUT_FILENO);
    }
    if (args[0] == NULL)
        return 0;
    if (!strcmp(args[i - 1], "&"))
    {
        amp = 1;
        args[i - 1] = NULL;
    }
    int redirect = -1;
    if (i >= 2 && (!strcmp(argv[i - 2], ">") || !strcmp(argv[i - 2], ">>")))
    {
        output_file = argv[i - 1];
        redirect = 1;
    }
    else if (i >= 2 && !strcmp(argv[i - 2], "2>"))
    {
        output_file = argv[i - 1];
        redirect = 2;
    }
    else if (i >= 2 && !strcmp(argv[i - 2], "<"))
    {
        output_file = argv[i - 1];
        redirect = 0;
    }
    if (strcmp(args[0], "prompt") == 0)
    {
        if (!strcmp(argv[1], "="))
        {
            strcpy(prompt, args[2]);
            strcat(prompt, ": ");
        }
        return 0;
    }
    if (strcmp(args[0], "echo") == 0)
    {
        char **echo_var = args + 1;
        if (strcmp(*echo_var, "$?") == 0)
        {
            printf("%d\n", status);
            return 0;
        }
        while (*echo_var)
        {
            if (echo_var != NULL && echo_var[0][0] == '$')
            {
                Node *node = vars.head;
                char *new_variable = NULL;
                while (node)
                {
                    if (strncmp(((Var *)node->data)->key, *echo_var, strlen(*echo_var)) == 0)
                    {
                        new_variable = ((Var *)node->data)->value;
                    }
                    node = node->next;
                }
                if (new_variable != NULL)
                    printf("%s ", new_variable);
            }
            else
            {
                printf("%s ", echo_var[0]);
            }
            echo_var++;
        }
        printf("\n");
        return 0;
    }
    else
        amp = 0;
    if (strcmp(args[0], "cd") == 0)
    {
        if (chdir(args[1]) != 0)
        {
            printf(" %s: no such directory\n", argv[1]);
        }
        return 0;
    }
    if (strcmp(args[0], "!!") == 0)
    {
        if (comms.size > 0)
        {
            strcpy(current_command, last_command);
            split_command(current_command);
            run_execute(argv);
        }
        else
        {
            printf("The command history list is empty");
        }
        return 0;
    }
    if (args[0][0] == '$' && i > 2)
    {
        Var *var = (Var *)malloc(sizeof(Var));
        var->key = malloc((strlen(args[0]) + 1));
        var->value = malloc((strlen(args[2]) + 1));
        strcpy(var->key, args[0]);
        strcpy(var->value, args[2]);
        add(&vars, var);
        return 0;
    }
    if (strcmp(args[0], "read") == 0)
    {
        Var *var = (Var *)malloc(sizeof(Var));
        var->key = malloc(sizeof(char) * (strlen(args[1])));
        var->value = malloc(sizeof(char) * MAX_COMMAND);
        var->key[0] = '$';
        memset(var->value, 0, MAX_COMMAND);
        strcpy(var->key + 1, args[1]);
        fgets(var->value, MAX_COMMAND, stdin);
        var->value[strlen(var->value) - 1] = '\0';
        add(&vars, var);
        return 0;
    }
    pid = fork();
    if (pid == 0)
    {
        if (redirect >= 0)
        {
            if (!strcmp(args[i - 2], ">>"))
            {
                if ((file_desc = open(output_file, O_APPEND | O_CREAT | O_WRONLY, 0660)) < 0)
                {
                    perror("Error: Can't create file");
                    exit(1);
                }
                lseek(file_desc, 0, SEEK_END);
            }
            else if (!strcmp(args[i - 2], ">") || !strcmp(args[i - 2], "2>"))
            {
                if ((file_desc = creat(output_file, 0660)) < 0)
                {
                    perror("Error: Can't create file");
                    exit(1);
                }
            }
            else
            {
                file_desc = open(output_file, O_RDONLY);
            }
            close(redirect);
            dup(file_desc);
            close(file_desc);
            args[i - 2] = NULL;
        }
        int status_code = execvp(args[0], args);
        if (status_code == -1)
        {
            printf("Unknown command: %s\n", command);
            exit(1);
        }
    }
    else if (pid == -1)
    {
        printf("Error forking a new process\n");
        exit(1);
    }
    else
    {
        pid = -1;
        wait(&status);
        rv = status;
    }
    if (piping != 0)
    {
        int status = close(STDOUT_FILENO);
        if (status == -1)
        {
            printf("Failed to close standard output file descriptor\n");
            exit(EXIT_FAILURE);
        }
        int newFD = dup(duplicate_fd);
        if (newFD == -1)
        {
            printf("Failed to duplicate file descriptor\n");
            exit(EXIT_FAILURE);
        }
        int status_pipeFD = close(pipe_fd[1]);
        if (status_pipeFD == -1)
        {
            printf("Failed to close file descriptor\n");
            exit(EXIT_FAILURE);
        }
        wait(NULL);
    }
    return rv;
}

int change_status(char **args)
{
    int rv = -1;
    if (args[0] == NULL)
    {
        rv = 0;
        return rv;
    }
    else
    {
        rv = run_execute(args);
        return rv;
    }
}

int main()
{
    main_process = getpid();
    signal(SIGINT, termination_handler);
    strcpy(prompt, "hello: ");
    command_counter = 0;
    char *prev_command = malloc(sizeof(char) * MAX_COMMAND);
    struct termios old_terminal, new_terminal;
    tcgetattr(STDIN_FILENO, &old_terminal);
    while (1)
    {
        memset(command, 0, sizeof(command));
        new_terminal = old_terminal;
        new_terminal.c_lflag &= (ECHOE | ~ICANON);
        tcsetattr(STDIN_FILENO, TCSANOW, &new_terminal);
        printf("\r");
        printf("%s", prompt);
        i = 1;
        c = getchar();
        if (c == 127 || c == '\b')
        {
            if (i < strlen(prompt) + 1)
            {
                printf("\b\b\b\b");
            }
            continue;
        }
        else if (c == '\033')
        {
            printf("\33[2K");
            a = getchar();
            b = getchar();
            switch (b)
            {
            case 'A':
                if (comms.size == 0)
                {
                    break;
                }
                else if (command_counter > 0)
                {
                    command_counter--;
                }
                printf("\b\b\b\b");
                prev_command = (char *)get_command(&comms, command_counter);
                printf("%s", (char *)get_command(&comms, command_counter));
                break;
            case 'B':
                if (comms.size == 0 || command_counter >= comms.size - 1)
                {
                    if (command_counter == comms.size - 1)
                    {
                        command_counter++;
                    }
                    break;
                }
                else
                {
                    command_counter++;
                }
                printf("\b\b\b\b");
                prev_command = (char *)get_command(&comms, command_counter);
                printf("%s", (char *)get_command(&comms, command_counter));
                break;
            }
            command[0] = c;
            continue;
        }

        else if (c == '\n')
        {
            tcsetattr(STDIN_FILENO, TCSANOW, &old_terminal);
            if (comms.size > 0)
            {
                prev_command = (char *)get_command(&comms, command_counter);
                new_command2 = malloc(sizeof(char) * strlen(prev_command));
                prev_command[strlen(prev_command)] = ' ';
                strcpy(new_command2, prev_command);
                add(&comms, new_command2);
                command_counter = comms.size;
                strcpy(command, new_command2);
                split_command(command);
                status = change_status(argv);
            }
            continue;
        }
        else
        {
            tcsetattr(STDIN_FILENO, TCSANOW, &old_terminal);
            memset(command, 0, MAX_COMMAND);
            command[0] = c;
            char b;
            i = 1;
            while ((b = getchar()) != '\n')
            {
                if (b == 127 || b == '\b')
                {
                    printf("\b \b");
                    i--;
                }
                else
                {
                    command[i] = b;
                    i++;
                };
            }
            command[i] = b;
            i++;
            command[strlen(command)] = '\0';
            i = 1;
        }

        int if_flag = 0;
        if (!strncmp(command, "if", 2))
        {
            if_flag = 1;
            while (1)
            {
                fgets(current_command, MAX_COMMAND, stdin);
                strcat(command, current_command);
                command[strlen(command) - 1] = '\n';
                if (!strcmp(current_command, end_if))
                    break;
            }
            char *command_curr = "bash";
            char *argument_list[] = {"bash", "-c", command, NULL};
            if (fork() == 0)
            {
                int status_code = execvp(command_curr, argument_list);
                printf("bash has taken control of this child process. This won't execute unless it terminates abnormally!\n");
                if (status_code == -1)
                {
                    printf("Terminated Incorrectly\n");
                    return 1;
                }
            }
            wait(&status);
            strcat(command, current_command);
        }
        command[strlen(command) - 1] = '\0';

        if (!strcmp(command, "quit"))
        {
            exit(0);
        }
        if (strcmp(command, "!!"))
        {
            tcsetattr(STDIN_FILENO, TCSANOW, &old_terminal);
            strcpy(last_command, command);
        }
        new_command = malloc(sizeof(char) * strlen(command));
        strcpy(new_command, command);
        add(&comms, new_command);
        command_counter = comms.size;
        if (!if_flag)
        {
            split_command(command);
            status = change_status(argv);
        }
        else
        {
            continue;
        }
    }
}
