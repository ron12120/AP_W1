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

char command[1024], lastCommand[1024], currentCommand[1024], prompt[1024];
int last_command, fd, amper, rv, piping, i;
int Duplicate_FD, mainProcess, status = 0;
int change_status(char **args);
char current_command[1024];
List variables, commands;
char **CountPIPEPointer;
pid_t ProccesID, cpid;
char *end_if = "fi\n";
char *new_command2;
char *new_command;
char *argv[1024];
char *outfile;
int pipeFD[2];
char *token;
int enter = 0;
char a, b, c;

char **CountPIPE(char **args)
{
    char **CounterPIPE = args;
    while (*CounterPIPE != NULL)
    {
        if (strcmp(*CounterPIPE, "|") == 0)
        {
            return CounterPIPE;
        }

        CounterPIPE++;
    }
    return NULL;
}

int CountARGS(char **args)
{
    char **CounterARGS = args;
    int cnt = 0;
    while (*CounterARGS != NULL)
    {
        CounterARGS++;
        cnt++;
    }
    return cnt;
}

char *safe_strcpy(char *dest, size_t size, char *src)
{
    if (size > 0)
    {
        *dest = '\0';
        return strncat(dest, src, size - 1);
    }
    return dest;
}

int first_index_in_str(char *arr)
{

    int j;
    for (j = 0; i < 1024; i++)
    {
        if (strlen(arr) <= i)
        {
            break;
        }
    }
    return j;
}

void split(char *command)
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
// Task number: 8
void termination_handler(int signum)
{
    if (getpid() == mainProcess)
    {
        printf("\nYou typed Control-C!\n");
        write(0, prompt, strlen(prompt) + 1);
        return;
    }
    else
    {
        fprintf(stderr, "\ncaught signal: %d\n", signum);
        kill(mainProcess, SIGKILL);
        return;
    }
}

int execute(char **args)
{

    rv = -1;
    piping = 0;
    i = CountARGS(args);
    char **CountPIPEPointer = CountPIPE(args);
    // Task number: 9

    if (CountPIPEPointer != NULL)
    {
        *CountPIPEPointer = NULL;
        piping++;
        int result = pipe(pipeFD);
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
            int status = close(pipeFD[1]);
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
            int newFD = dup(pipeFD[0]);
            if (newFD == -1)
            {
                printf("Failed to duplicate file descriptor\n");
                exit(EXIT_FAILURE);
            }
            execute(CountPIPEPointer + 1);
            exit(0);
        }

        Duplicate_FD = dup(STDOUT_FILENO);
        if (Duplicate_FD == -1)
        {
            printf("Failed to duplicate file descriptor\n");
            exit(EXIT_FAILURE);
        }
        dup2(pipeFD[1], STDOUT_FILENO);
    }

    if (args[0] == NULL)
        return 0;

    if (!strcmp(args[i - 1], "&"))
    {
        amper = 1;
        args[i - 1] = NULL;
    }

    int redirect = -1;
    // Task number: 1
    if (i >= 2 && (!strcmp(argv[i - 2], ">") || !strcmp(argv[i - 2], ">>")))
    {
        outfile = argv[i - 1];
        redirect = 1;
    }
    else if (i >= 2 && !strcmp(argv[i - 2], "2>"))
    {
        outfile = argv[i - 1];
        redirect = 2;
    }
    else if (i >= 2 && !strcmp(argv[i - 2], "<"))
    {
        outfile = argv[i - 1];
        redirect = 0;
    }

    // Task number: 2
    if (strcmp(args[0], "prompt") == 0)
    {
        if (!strcmp(argv[1], "="))
        {
            strcpy(prompt, args[2]);
        }

        return 0;
    }
    if (strcmp(args[0], "echo") == 0)
    {
        char **echo_var = args + 1;
        // Task number: 4
        if (strcmp(*echo_var, "$?") == 0)
        {
            printf("%d\n", status);
            return 0;
        }

        while (*echo_var)
        {

            if (echo_var != NULL && echo_var[0][0] == '$')
            {
                // Task number: 3
                Node *node = variables.head;
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
                // Task number: 3
                printf("%s ", echo_var[0]);
            }

            echo_var++;
        }
        printf("\n");
        return 0;
    }

    else
        amper = 0;

    // Task number: 5
    if (strcmp(args[0], "cd") == 0)
    {
        if (chdir(args[1]) != 0)
        {
            printf(" %s: no such directory\n", argv[1]);
        }
        return 0;
    }
    // Task number:  6
    if (strcmp(args[0], "!!") == 0)
    {
        if (commands.size > 0)
        {
            strcpy(currentCommand, lastCommand);
            split(currentCommand);
            execute(argv);
        }
        else
        {
            printf("The command history list is empty");
        }

        return 0;
    }
    // Task number: 10
    if (args[0][0] == '$' && i > 2)
    {
        Var *var = (Var *)malloc(sizeof(Var));
        var->key = malloc((strlen(args[0]) + 1));
        var->value = malloc((strlen(args[2]) + 1));
        strcpy(var->key, args[0]);
        strcpy(var->value, args[2]);
        add(&variables, var);
        return 0;
    }
    // Task number: 11
    if (strcmp(args[0], "read") == 0)
    {
        Var *var = (Var *)malloc(sizeof(Var));
        var->key = malloc(sizeof(char) * (strlen(args[1])));
        var->value = malloc(sizeof(char) * 1024);
        var->key[0] = '$';
        memset(var->value, 0, 1024);
        strcpy(var->key + 1, args[1]);
        fgets(var->value, 1024, stdin);
        var->value[strlen(var->value) - 1] = '\0';
        add(&variables, var);
        return 0;
    }

    ProccesID = fork();

    // Continue task number: 1
    if (ProccesID == 0)
    {

        if (redirect >= 0)
        {
            if (!strcmp(args[i - 2], ">>"))
            {
                if ((fd = open(outfile, O_APPEND | O_CREAT | O_WRONLY, 0660)) < 0)
                {
                    perror("Error: Can't create file");
                    exit(1);
                }
                lseek(fd, 0, SEEK_END);
            }
            else if (!strcmp(args[i - 2], ">") || !strcmp(args[i - 2], "2>"))
            {
                if ((fd = creat(outfile, 0660)) < 0)
                {
                    perror("Error: Can't create file");
                    exit(1);
                }
            }
            else
            {
                fd = open(outfile, O_RDONLY);
            }
            close(redirect);
            dup(fd);
            close(fd);
            args[i - 2] = NULL;
        }
        int status_code = execvp(args[0], args);
        if (status_code == -1)
        {
            printf("Unknown command: %s\n", command);
            exit(1);
        }
    }
    else if (ProccesID == -1)
    {
        printf("Error forking a new process\n");
        exit(1);
    }
    else
    {
        ProccesID = -1;
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
        int newFD = dup(Duplicate_FD);
        if (newFD == -1)
        {
            printf("Failed to duplicate file descriptor\n");
            exit(EXIT_FAILURE);
        }
        int status_pipeFD = close(pipeFD[1]);
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
        rv = execute(args);
        return rv;
    }
}

int main()
{
    mainProcess = getpid();
    signal(SIGINT, termination_handler);
    strcpy(prompt, "hello: ");
    last_command = 0;
    char *prevCommand = malloc(sizeof(char) * 1024);
    struct termios old_terminal, new_terminal;
    tcgetattr(STDIN_FILENO, &old_terminal);

    while (1)
    {
        memset(command, 0, sizeof(command));
        new_terminal = old_terminal;
        new_terminal.c_lflag &= (ECHOE | ~ICANON);
        tcsetattr(STDIN_FILENO, TCSANOW, &new_terminal);
        printf("%s", prompt);
        i = 1;
        c = getchar();
        if (c == 127 || c == '\b')
        {
            if (i < strlen(prompt) + 1)
            {
                printf("\b\b\b   \b");
            }
            continue;
        }
        else if (c == '\033')
        {
            // Task number: 12
            printf("\33[2K");
            a = getchar();
            b = getchar();
            switch (b)
            {
            case 'A':
                if (commands.size == 0)
                {
                    break;
                }
                else if (last_command > 0)
                {
                    last_command--;
                }
                printf("\b\b\b\b");
                prevCommand = (char *)get_command(&commands, last_command);
                printf("%s", (char *)get_command(&commands, last_command));
                break;
            case 'B':
                if (commands.size == 0 || last_command >= commands.size - 1)
                {
                    if (last_command == commands.size - 1)
                    {
                        last_command++;
                    }
                    break;
                }
                else
                {
                    last_command++;
                }
                printf("\b\b\b\b");
                prevCommand = (char *)get_command(&commands, last_command);
                printf("%s", (char *)get_command(&commands, last_command));
                break;
            }
            command[0] = c;
            continue;
        }

        else if (c == '\n')
        {
            tcsetattr(STDIN_FILENO, TCSANOW, &old_terminal);
            if (commands.size > 0)
            {
                prevCommand = (char *)get_command(&commands, last_command);
                new_command2 = malloc(sizeof(char) * strlen(prevCommand));
                prevCommand[strlen(prevCommand)] = ' ';
                strcpy(new_command2, prevCommand);
                add(&commands, new_command2);
                last_command = commands.size;
                strcpy(command, new_command2);
                split(command);
                status = change_status(argv);
            }
            continue;
        }
        else
        {
            tcsetattr(STDIN_FILENO, TCSANOW, &old_terminal);
            memset(command, 0, 1024);
            command[0] = c;
            char b;
            int Flag = 0;
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
            // Task number: 13
            if_flag = 1;
            while (1)
            {
                fgets(current_command, 1024, stdin);
                strcat(command, current_command);
                command[strlen(command) - 1] = '\n';
                if (!strcmp(current_command, end_if))
                    break;
            }
            char *commandcurr = "bash";
            char *argument_list[] = {"bash", "-c", command, NULL};
            if (fork() == 0)
            {
                int status_code = execvp(commandcurr, argument_list);
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
            // Task number: 7
            exit(0);
        }

        if (strcmp(command, "!!"))
        {
            tcsetattr(STDIN_FILENO, TCSANOW, &old_terminal);
            strcpy(lastCommand, command);
        }

        new_command = malloc(sizeof(char) * strlen(command));
        strcpy(new_command, command);
        add(&commands, new_command);
        last_command = commands.size;
        if (!if_flag)
        {
            split(command);
            status = change_status(argv);
        }
        else
        {
            continue;
        }
    }
}