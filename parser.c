#include <signal.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include "pid_set.h"

#define INPUT_LENGTH 2048
#define MAX_ARGS 512

struct command_line {
    char *argv[MAX_ARGS + 1];
    int argc;
    char *input_file;
    char *output_file;
    bool is_bg;
};

// Code adapted from sample parser
struct command_line *parse_input() {
    char input[INPUT_LENGTH];
    struct command_line *curr_command = NULL;

    // Get input
    printf(": ");
    fflush(stdout);
    fgets(input, INPUT_LENGTH, stdin);

    curr_command = (struct command_line *) calloc(1, sizeof(struct command_line));

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

static void free_command_line(struct command_line *line) {
    if (line == NULL) return;
    
    for (size_t i = 0; i < line->argc; i++) {
        free(line->argv[i]);
    }

    if (line->input_file != NULL) free(line->input_file);
    if (line->output_file != NULL) free(line->output_file);
    free(line);
}

// Code adapted from sample parser
int main() {
    struct command_line *curr_command = NULL;
    pid_set active_child_processes;
    int foreground_status = 0;
    init_pid_set(&active_child_processes, 8);

    while (true) {
        // DEBUG
        char cwd[1024];
        printf("CWD: %s", getcwd(cwd, sizeof(cwd)));

        // Wait for user input
        curr_command = parse_input();

        // Ignore comment lines and blank lines
        if (curr_command->argc == 0 || curr_command->argv[0][0] == '#') continue;

        // `exit` command
        if (!strcmp(curr_command->argv[0], "exit")) {
            for (size_t i = 0; i < active_child_processes.size; i++) {
                kill(active_child_processes.array[i], SIGTERM);
            }
            exit(0);
        }

        // `cd` command
        if (!strcmp(curr_command->argv[0], "cd")) {
            if (curr_command->argc == 1) {
                chdir(getenv("HOME"));
            } else {
                chdir(curr_command->argv[1]);
            }
        }

        // `status` command
        if (!strcmp(curr_command->argv[0], "status")) {
            if (WIFEXITED(foreground_status)) {
                printf("exit value %d\n", WEXITSTATUS(foreground_status));
            } else {
                printf("terminated by signal %d\n", WTERMSIG(foreground_status));
            }
        }

        // DEBUG
        if (curr_command != NULL) {
            printf("=============\n");
            printf("ARGS (%i):\n", curr_command->argc);
            for (size_t i = 0; i < curr_command->argc; i++) {
                printf(" %s", curr_command->argv[i]);
            }
            printf("\nIN: %s, ", curr_command->input_file);
            printf("OUT: %s, ", curr_command->output_file);
            printf("BG: %i\n", curr_command->is_bg);
            printf("=============\n");

            free_command_line(curr_command);
        }
    }

    free_command_line(curr_command);
    return EXIT_SUCCESS;
}