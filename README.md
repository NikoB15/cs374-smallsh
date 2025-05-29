# SMALLSH
Niko Bransfield  
Oregon State University  
CS374 Portfolio Project  
Completed 2025-03-02

## Overview
Simple Linux shell written in C.

## Program Functionality
- Provides a prompt for running commands
  - Syntax: `command [arg1 arg2 ...] [< input_file] [> output_file] [&]`
- Handles blank lines and comments (lines beginning with `#`)
- Three builtin commands:
  - `exit`: Exits SMALLSH.
  - `cd [directory]`: Changes the working directory.
    - `directory` can be an absolute or relative path.
    - If `directory` is omitted, this command changes to the directory specified by the HOME environment variable
  - `status`: Prints the exit status or terminating signal of the last foreground process run by SMALLSH.
- Support for external commands by creating new processes using the `exec()` family of functions
- Support for input and output redirection using `<` and `>` operators
- Support for running commands as background processes using `&` operator
- Custom handlers for `SIGINT` and `SIGTSTP`
  - Pressing `Ctrl+C` stops the current foreground process (if one is active).
  - Pressing `Ctrl+Z` toggles foreground-only mode.
