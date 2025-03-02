#define _XOPEN_SOURCE 700  // Prevent sigaction struct linting error

#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include "pid_set.h"

#define INPUT_LENGTH 2048
#define MAX_ARGS 512

typedef struct {
    char *argv[MAX_ARGS + 1];
    int argc;
    char *input_file;
    char *output_file;
    bool is_bg;
} command_line;

static bool is_foreground_only = false;

// Code adapted from sample parser
command_line *parse_input() {
    char input[INPUT_LENGTH];
    command_line *curr_command = NULL;

    // Get input
    printf(": ");
    fflush(stdout);
    fgets(input, INPUT_LENGTH, stdin);

    curr_command = (command_line *) calloc(1, sizeof(command_line));

    // Tokenize the input
    char *token = strtok(input, " \n");
    while (token) {
        // Background token is not the final word -> reinterpret it as normal text
        if (curr_command->is_bg) {
            curr_command->is_bg = false;
            // Reinterpret "&" as an arg
            curr_command->argv[curr_command->argc++] = strdup("&");
        }

        if (!strcmp(token,"<")) {
            // We assume the redirection operator is followed by another word
            curr_command->input_file = strdup(strtok(NULL," \n"));
        } else if (!strcmp(token,">")) {
            // We assume the redirection operator is followed by another word
            curr_command->output_file = strdup(strtok(NULL," \n"));
        } else if (!strcmp(token,"&")) {
            curr_command->is_bg = true;
        } else {
            curr_command->argv[curr_command->argc++] = strdup(token);
        }
        token = strtok(NULL," \n");
    }
    return curr_command;
}

static void free_command_line(command_line *line) {
    if (line == NULL) return;
    
    for (size_t i = 0; i < line->argc; i++) {
        free(line->argv[i]);
    }

    if (line->input_file != NULL) free(line->input_file);
    if (line->output_file != NULL) free(line->output_file);
    free(line);
}

static void cmd_exit(pid_set *active_child_processes) {
    for (size_t i = 0; i < active_child_processes->size; i++) {
        printf("Killed process %i\n", active_child_processes->array[i]);
        kill(active_child_processes->array[i], SIGTERM);
    }
    exit(0);
}

static void change_dir(command_line *line) {
    if (line->argc == 1) {
        chdir(getenv("HOME"));
    } else {
        chdir(line->argv[1]);
    }
}

static void print_status_2(int status, pid_t background_pid) {
    if (background_pid > 0) printf("background pid %i is done: ", background_pid);
    if (WIFEXITED(status)) {
        printf("exit value %d\n", WEXITSTATUS(status));
    } else {
        printf("terminated by signal %d\n", WTERMSIG(status));
    }
}

static void print_status_1(int status) {
    print_status_2(status, 0);
}

static void handle_SIGTSTP(int signo) {
    char *on_msg = "Entering foreground-only mode (& is now ignored)\n";
    char *off_msg = "Exiting foreground-only mode\n";

    is_foreground_only = !is_foreground_only;
    if (is_foreground_only) {
        write(STDOUT_FILENO, on_msg, 50);
    } else {
        write(STDOUT_FILENO, off_msg, 30);
    }
}

static pid_t fork_exec(command_line *line) {
    pid_t child_pid = fork();
    switch (child_pid) {
        case -1:
            perror("fork failed");
            break;
        case 0: ;
            struct sigaction ignore_action = {0};
            ignore_action.sa_handler = SIG_IGN;
            sigaction(SIGINT, &ignore_action, NULL);

            if (line->is_bg) {
                if (line->input_file == NULL) line->input_file = "/dev/null";
                if (line->output_file == NULL) line->output_file = "/dev/null";
            } else {
                struct sigaction default_action = {0};
                default_action.sa_handler = SIG_DFL;
                sigaction(SIGINT, &default_action, NULL);
            }

            if (line->input_file != NULL) {
                int input_fd = open(line->input_file, O_RDONLY);
                if (input_fd == -1) {
                    printf("cannot open %i for input", input_fd);
                    exit(1);
                }
                dup2(input_fd, STDIN_FILENO);
            }

            if (line->output_file != NULL) {
                int output_fd = open(line->output_file, O_WRONLY | O_TRUNC | O_CREAT, 0664);
                if (output_fd == -1) {
                    printf("cannot open %i for output", output_fd);
                    exit(1);
                }
                dup2(output_fd, STDOUT_FILENO);
            }

            execvp(line->argv[0], line->argv);
            // Child process executes this only if an error occurs
            perror("execv");
            exit(1);
    }
    return child_pid;
}

static void run_external_command(command_line *line, int *foreground_status, pid_set *active_child_processes, bool force_foreground) {
    if (force_foreground) line->is_bg = false;
    pid_t child_pid = fork_exec(line);
    add_pid_set(active_child_processes, child_pid);

    if (line->is_bg) {
        printf("background pid is %d\n", child_pid);
    } else {
        waitpid(child_pid, foreground_status, 0);
        // Print message if process was signaled
        if (WIFSIGNALED(*foreground_status)) print_status_1(*foreground_status);
        remove_pid_set(active_child_processes, child_pid);
    }
}

int main() {
    command_line *line = NULL;
    pid_set active_child_processes;
    init_pid_set(&active_child_processes, 8);
    int foreground_status = 0;

    struct sigaction ignore_action = {0};
    ignore_action.sa_handler = SIG_IGN;
    sigaction(SIGINT, &ignore_action, NULL);

    struct sigaction SIGTSTP_action = {0};
    SIGTSTP_action.sa_handler = handle_SIGTSTP;
    sigaction(SIGTSTP, &SIGTSTP_action, NULL);

    while (true) {
        if (line != NULL) {
            free_command_line(line);
        }

        // Check if any background processes terminated
        for (size_t i = 0; i < active_child_processes.size; i++) {
            int child_status;
            pid_t child_pid;
            child_pid = waitpid(active_child_processes.array[i], &child_status, WNOHANG);
            if (child_pid != 0) {
                print_status_2(child_status, child_pid);
                remove_pid_set(&active_child_processes, child_pid);
            }
        }

        // DEBUG
        char cwd[1024];
        printf("%s | %d", getcwd(cwd, sizeof(cwd)), getpid());

        // Wait for user input
        line = parse_input();

        // Ignore comment lines and blank lines
        if (line->argc == 0 || line->argv[0][0] == '#') continue;


        if (!strcmp(line->argv[0], "exit")) {
            cmd_exit(&active_child_processes);
        } 
        else if (!strcmp(line->argv[0], "cd")) {
            change_dir(line);
        }
        else if (!strcmp(line->argv[0], "status")) {
            print_status_1(foreground_status);
        }
        else {
            run_external_command(line, &foreground_status, &active_child_processes, is_foreground_only);
        }
    }

    free_command_line(line);
    return EXIT_SUCCESS;
}