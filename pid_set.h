#include <sys/types.h>
#include <stdlib.h>
#include <stdbool.h>

typedef struct {
    pid_t *array;
    size_t capacity;
    size_t size;
} pid_set;

void init_pid_set(pid_set *set, size_t initial_capacity);
bool add_pid_set(pid_set *set, pid_t pid);
bool remove_pid_set(pid_set *set, pid_t pid);
bool free_pid_set(pid_set *set);