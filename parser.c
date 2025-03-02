#include "parser.h"

static bool is_foreground_only = false;

// Code adapted from sample parser (https://canvas.oregonstate.edu/courses/1987883/files/109834045)
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
        fflush(stdout);
    } else {
        printf("terminated by signal %d\n", WTERMSIG(status));
        fflush(stdout);
    }
}

static void print_status_1(int status) {
    print_status_2(status, 0);
}

static void toggle_foreground_mode(int signo) {
    char *on_msg = "\nEntering foreground-only mode (& is now ignored)\n";
    char *off_msg = "\nExiting foreground-only mode\n";

    is_foreground_only = !is_foreground_only;
    if (is_foreground_only) {
        write(STDOUT_FILENO, on_msg, 51);
    } else {
        write(STDOUT_FILENO, off_msg, 31);
    }
}

static void sig_ignore(int signo) {
    struct sigaction ignore_action = {0};
    ignore_action.sa_handler = SIG_IGN;
    sigaction(signo, &ignore_action, NULL);
}

static void sig_reset(int signo) {
    struct sigaction default_action = {0};
    default_action.sa_handler = SIG_DFL;
    sigaction(signo, &default_action, NULL);
}

static pid_t fork_exec(command_line *line) {
    pid_t child_pid = fork();
    switch (child_pid) {
        case -1:
            perror("fork failed");
            break;
        case 0: ;
            // All children ignore ^Z
            sig_ignore(SIGTSTP);

            if (line->is_bg) {
                // Background children's input and output is ignored by default
                if (line->input_file == NULL) line->input_file = "/dev/null";
                if (line->output_file == NULL) line->output_file = "/dev/null";
            } else {
                // Foreground child uses default ^C behavior
                sig_reset(SIGINT);
            }

            // Set input
            if (line->input_file != NULL) {
                int input_fd = open(line->input_file, O_RDONLY);
                if (input_fd == -1) {
                    printf("cannot open %s for input\n", line->input_file);
                    fflush(stdout);
                    exit(1);
                }
                dup2(input_fd, STDIN_FILENO);
            }
            // Set output
            if (line->output_file != NULL) {
                int output_fd = open(line->output_file, O_WRONLY | O_TRUNC | O_CREAT, 0664);
                if (output_fd == -1) {
                    printf("cannot open %s for output\n", line->output_file);
                    fflush(stdout);
                    exit(1);
                }
                dup2(output_fd, STDOUT_FILENO);
            }

            execvp(line->argv[0], line->argv);
            // Child process executes this only if an error occurs
            perror(line->argv[0]);
            exit(1);
    }
    return child_pid;
}

static void run_external_command(command_line *line, int *foreground_status, pid_set *active_child_processes, bool force_foreground) {
    // Block ^Z signals while a foreground process is running (or while a background process 
    //  is being started, but the former is more important)
    sigset_t block_set, old_set;
    sigaddset(&block_set, SIGTSTP);
    sigprocmask(SIG_BLOCK, &block_set, &old_set);
    
    if (force_foreground) line->is_bg = false;
    pid_t child_pid = fork_exec(line);
    add_pid_set(active_child_processes, child_pid);

    if (line->is_bg) {
        printf("background pid is %d\n", child_pid);
        fflush(stdout);
    } else {
        waitpid(child_pid, foreground_status, 0);
        // Print message if process was signaled
        if (WIFSIGNALED(*foreground_status)) print_status_1(*foreground_status);
        remove_pid_set(active_child_processes, child_pid);
    }

    // Unblock ^Z signals
    sigprocmask(SIG_SETMASK, &old_set, NULL);
}

static void print_finished_background_processes(pid_set *active_child_processes) {
    for (size_t i = 0; i < active_child_processes->size; i++) {
        int child_status;
        pid_t child_pid;
        child_pid = waitpid(active_child_processes->array[i], &child_status, WNOHANG);
        if (child_pid != 0) {
            print_status_2(child_status, child_pid);
            remove_pid_set(active_child_processes, child_pid);
        }
    }
}

int main() {
    command_line *line = NULL;
    pid_set active_child_processes;
    init_pid_set(&active_child_processes, 8);
    int foreground_status = 0;
    bool prev_foreground_mode = false;

    // Ignore ^C
    sig_ignore(SIGINT);

    // Handle ^Z: Toggle foreground mode
    struct sigaction SIGTSTP_action = {0};
    SIGTSTP_action.sa_handler = toggle_foreground_mode;
    sigaction(SIGTSTP, &SIGTSTP_action, NULL);

    while (true) {
        if (line != NULL) {
            free_command_line(line);
        }

        // If any background processes terminated, notify the user
        print_finished_background_processes(&active_child_processes);

        // Wait for user input
        line = parse_input();

        if (prev_foreground_mode != is_foreground_only) {
            prev_foreground_mode = is_foreground_only;
            continue;
        }

        // Ignore comment lines and blank lines
        if (line->argc == 0 || line->argv[0][0] == '#') continue;

        // Handle commands
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