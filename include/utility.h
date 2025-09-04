#ifndef UTILITY_H
#define UTILITY_H

/**
 * !! DO NOT MODIFY THIS FILE !!
 */

#include <stddef.h>
#include <stdbool.h> // ADD THIS LINE

// Forward declare structs from filesys.h to avoid circular dependency
struct filesystem;
typedef struct filesystem filesystem_t;
typedef enum fs_display_flag fs_display_flag_t;
typedef unsigned int dblock_index_t;


size_t calculate_index_dblock_amount(size_t file_size);

size_t calculate_necessary_dblock_amount(size_t file_size);

dblock_index_t *cast_dblock_ptr(void *addr);

void display_filesystem(filesystem_t *fs, fs_display_flag_t flag);

// ADD THIS FUNCTION PROTOTYPE
bool split_path(const char *path, char *parent_path, char *child_name);

#endif