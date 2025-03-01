#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

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
        if (!strcmp(token,"<")) {
            curr_command->input_file = strdup(strtok(NULL," \n"));
        } else if (!strcmp(token,">")) {
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
    for (size_t i = 0; i < line->argc; i++) {
        free(line->argv[i]);
    }
    free(line->input_file);
    free(line->output_file);
    free(line);
}

// Code adapted from sample parser
int main() {
    struct command_line *curr_command = NULL;
    while (true) {
        //TODO: Remove
        if (curr_command != NULL) {
            printf("ARGS:\n");
            for (size_t i = 0; i < curr_command->argc; i++) {
                printf(" > %s\n", curr_command->argv[i]);
            }
            printf("IN: %s\n", curr_command->input_file);
            printf("OUT: %s\n", curr_command->output_file);
            printf("BG: %i\n", curr_command->is_bg);
            free_command_line(curr_command);
        }
        curr_command = parse_input();
    }
    free(curr_command);
    return EXIT_SUCCESS;
}