#include "main.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>

#define MAX_COMMAND_LENGTH 255
#define MAX_ARGS 64
#define MAX_JOBS 100

// Function prototypes
void execute_command(char *args[]);
void handle_signals(int signal_number);
void builtin_cd(char *args[]);
void builtin_echo(char *args[]);
void builtin_pwd();
void builtin_export(char *args[]);
void builtin_unset(char *args[]);
void builtin_alias(char *args[]);
void builtin_unalias(char *args[]);
void builtin_jobs();
void builtin_fg(char *args[]);
void builtin_bg(char *args[]);
void builtin_exit();

// Job structure
typedef struct {
    pid_t pid;
    char command[MAX_COMMAND_LENGTH + 1];
} Job;

Job jobs[MAX_JOBS];
int num_jobs = 0;

// Command structure
typedef struct {
    char *name;
    void (*function)(char *args[]);
} Command;

// Array of built-in commands
Command builtins[] = {
        {"cd", builtin_cd},
        {"echo", builtin_echo},
        {"pwd", builtin_pwd},
        {"export", builtin_export},
        {"unset", builtin_unset},
        {"alias", builtin_alias},
        {"unalias", builtin_unalias},
        {"jobs", builtin_jobs},
        {"fg", builtin_fg},
        {"bg", builtin_bg},
        {"exit", builtin_exit},
        {NULL, NULL} // Terminator for the array
};

int main() {
    char input[MAX_COMMAND_LENGTH];
    char *args[MAX_ARGS];

    // Set up signal handlers
    signal(SIGINT, handle_signals);
    signal(SIGTSTP, SIG_IGN); // Ignore Ctrl+Z

    while (1) {
        printf("$ ");
        fflush(stdout);

        if (fgets(input, sizeof(input), stdin) == NULL) {
            break;
        }

        // Tokenize input
        char *token = strtok(input, " \n");
        int i = 0;
        while (token != NULL && i < MAX_ARGS - 1) {
            args[i++] = token;
            token = strtok(NULL, " \n");
        }
        args[i] = NULL;

        // Check if command is a built-in command
        int found = 0;
        for (int j = 0; builtins[j].name != NULL; j++) {
            if (strcmp(args[0], builtins[j].name) == 0) {
                builtins[j].function(args);
                found = 1;
                break;
            }
        }

        if (!found) {
            execute_command(args);
        }
    }

    return 0;
}

void execute_command(char *args[]) {
    pid_t pid = fork();
    if (pid == -1) {
        perror("fork");
    } else if (pid == 0) {
        // Child process
        execvp(args[0], args);
        perror("execvp");
        exit(EXIT_FAILURE);
    } else {
        // Parent process
        int status;
        waitpid(pid, &status, 0);
    }
}

void handle_signals(int signal_number) {
    if (signal_number == SIGINT) {
        printf("\n"); // Print newline after Ctrl+C
    }
}

// Implementations of built-in commands

void builtin_cd(char *args[]) {
    if (args[1] == NULL) {
        fprintf(stderr, "cd: missing argument\n");
    } else {
        if (chdir(args[1]) != 0) {
            perror("cd");
        }
    }
}

void builtin_echo(char *args[]) {
    for (int i = 1; args[i] != NULL; i++) {
        printf("%s ", args[i]);
    }
    printf("\n");
}

void builtin_pwd() {
    char cwd[MAX_COMMAND_LENGTH + 1];
    if (getcwd(cwd, sizeof(cwd)) != NULL) {
        printf("%s\n", cwd);
    } else {
        perror("pwd");
    }
}

void builtin_export(char *args[]) {
    if (args[1] == NULL) {
        fprintf(stderr, "export: missing argument\n");
    } else {
        if (putenv(args[1]) != 0) {
            perror("export");
        }
    }
}

void builtin_unset(char *args[]) {
    if (args[1] == NULL) {
        fprintf(stderr, "unset: missing argument\n");
    } else {
        if (unsetenv(args[1]) != 0) {
            perror("unset");
        }
    }
}

void builtin_alias(char *args[]) {
    if (args[1] == NULL) {
        fprintf(stderr, "alias: missing arguments\n");
        return;
    }

    // Copy the argument to avoid modifying the original string
    char *arg_copy = strdup(args[1]);
    if (arg_copy == NULL) {
        perror("strdup");
        return;
    }

    // Split the argument into alias name and value
    char *equals = strchr(arg_copy, '=');
    if (equals == NULL || equals == arg_copy) {
        fprintf(stderr, "alias: invalid syntax\n");
        free(arg_copy);
        return;
    }

    *equals = '\0'; // Split at the equals sign
    char *alias_name = arg_copy;
    char *alias_value = equals + 1;

    // Store the alias (not implemented in this example)
    printf("Alias: %s=%s\n", alias_name, alias_value);

    free(arg_copy);
}

void builtin_unalias(char *args[]) {
    if (args[1] == NULL) {
        fprintf(stderr, "unalias: missing arguments\n");
        return;
    }

    // Not implemented in this example
    printf("Unalias: %s\n", args[1]);
}

void builtin_jobs() {
    printf("Jobs:\n");
    printf("PID\tCommand\n");
    for (int i = 0; i < num_jobs; i++) {
        printf("%d\t%s\n", jobs[i].pid, jobs[i].command);
    }
}

void builtin_fg(char *args[]) {
    if (args[1] == NULL) {
        printf("Usage: fg <job_id>\n");
        return;
    }

    int job_id = atoi(args[1]);
    if (job_id < 1 || job_id > num_jobs) {
        printf("Invalid job ID.\n");
        return;
    }

    pid_t pid = jobs[job_id - 1].pid;

    // Wait for the job to finish
    int status;
    waitpid(pid, &status, 0);

    // Remove the job from the list
    for (int i = job_id - 1; i < num_jobs - 1; i++) {
        jobs[i] = jobs[i + 1];
    }
    num_jobs--;

    printf("Foreground job %d (%s) terminated.\n", pid, jobs[job_id - 1].command);
}

// Background the specified job
void builtin_bg(char *args[]) {
    if (args[1] == NULL) {
        printf("Usage: bg <job_id>\n");
        return;
    }

    int job_id = atoi(args[1]);
    if (job_id < 1 || job_id > num_jobs) {
        printf("Invalid job ID.\n");
        return;
    }

    pid_t pid = jobs[job_id - 1].pid;
    printf("Background job %d (%s).\n", pid, jobs[job_id - 1].command);
    // Add code to send the job to the background (e.g., SIGCONT)
    if (kill(pid, SIGCONT) == -1) {
        perror("kill");
    }
}

void builtin_exit() {
    printf("Exiting shell...\n");
    exit(EXIT_SUCCESS);
}
