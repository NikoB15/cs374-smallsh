#include "pid_set.h"

void init_pid_set(pid_set *set, size_t initial_capacity) {
    set->array = malloc(initial_capacity * sizeof(pid_t));
    set->capacity = initial_capacity;
    set->size = 0;
}

bool add_pid_set(pid_set *set, pid_t pid) {
    // Check whether element is already present
    for (size_t i = 0; i < set->size; i++) {
        if (set->array[i] == pid) return false;
    }
    
    // Add element
    set->size++;
    if (set->size > set->capacity) {
        set->capacity *= 2;
        set->array = realloc(set->array, set->capacity * sizeof(pid_t));
    }
    set->array[set->size-1] = pid;

    return true;
}

bool remove_pid_set(pid_set *set, pid_t pid) {
    // Try to find element
    for (size_t i = 0; i < set->size; i++) {
        if (set->array[i] != pid) continue;

        // Remove element
        set->array[i] = set->array[set->size-1];
        set->size--;
        return true;
    }

    // Element does not exist
    return false;
}

bool free_pid_set(pid_set *set) {
    if (set == NULL) return false;

    free(set->array);
    set->array = NULL;
    set->capacity = 0;
    set->size = 0;
    return true;
}