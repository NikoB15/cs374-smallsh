#include <sys/types.h>
#include <stdlib.h>

typedef struct {
    pid_t *array;
    size_t capacity;
    size_t size;
} pid_set;

int init_pid_set(pid_set *set, size_t initial_capacity);
int add_pid_set(pid_set *set, pid_t pid);
int remove_pid_set(pid_set *set, pid_t pid);
int free_pid_set(pid_set *set);