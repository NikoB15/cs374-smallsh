#define _XOPEN_SOURCE 700  // Prevent sigaction struct linting error

#include <fcntl.h>
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

typedef struct {
    char *argv[MAX_ARGS + 1];
    int argc;
    char *input_file;
    char *output_file;
    bool is_bg;
} command_line;

command_line *parse_input();