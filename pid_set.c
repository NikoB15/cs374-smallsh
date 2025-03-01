#include "pid_set.h"

int init_pid_set(pid_set *set, size_t initial_capacity) {
    set->array = malloc(initial_capacity * sizeof(pid_t));
    set->capacity = initial_capacity;
    set->size = 0;
    return 1;
}

int add_pid_set(pid_set *set, pid_t pid) {
    // Check whether element is already present
    for (size_t i = 0; i < set->size; i++) {
        if (set->array[i] == pid) return 0;
    }
    
    // Add element
    set->size++;
    if (set->size == set->capacity) {
        set->capacity *= 2;
        set->array = realloc(set->array, set->capacity * sizeof(pid_t));
    }
    set->array[set->size-1] = pid;
    return 1;
}

int remove_pid_set(pid_set *set, pid_t pid) {
    // Try to find element
    for (size_t i = 0; i < set->size; i++) {
        if (set->array[i] != pid) continue;

        // Remove element
        set->array[i] = set->array[set->size-1];
        set->size--;
        return 1;
    }

    // Element does not exist
    return 0;
}

int free_pid_set(pid_set *set) {
    if (set == NULL) return 0;

    free(set->array);
    set->array = NULL;
    set->capacity = 0;
    set->size = 0;
    return 1;
}