// filepath: /workspaces/linux-filesystem-sim/src/file_operations.c
#include "filesys.h"
#include "utility.h" // Assuming this is where split_path is declared
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#define DIRECTORY_ENTRY_SIZE (sizeof(inode_index_t) + MAX_FILE_NAME_LEN)

// ----------------------- HELPER PROTOTYPES ----------------------- //
static inode_index_t find_inode_for_path(terminal_context_t *context, const char *path);
static fs_retcode_t add_entry_to_directory(filesystem_t *fs, inode_t *dir_inode, const char *name, inode_index_t entry_inode_index);
static fs_retcode_t remove_entry_from_directory(filesystem_t *fs, inode_t *dir_inode, const char *name);
static inode_index_t find_inode_in_dir(filesystem_t *fs, inode_t *dir_inode, const char *name);

// ----------------------- CORE FUNCTION ----------------------- //
int new_file(terminal_context_t *context, char *path, permission_t perms)
{
    if(context == NULL || path == NULL || strlen(path) == 0){
        REPORT_RETCODE(INVALID_INPUT);
        return -1;
    }

    char parent_path[256];
    char new_name[MAX_FILE_NAME_LEN];
    if (!split_path(path, parent_path, new_name)) {
        REPORT_RETCODE(INVALID_FILENAME);
        return -1;
    }

    inode_index_t parent_inode_idx = find_inode_for_path(context, parent_path);
    if (parent_inode_idx == (inode_index_t)-1) {
        REPORT_RETCODE(DIR_NOT_FOUND);
        return -1;
    }
    inode_t *parent_inode = &context->fs->inodes[parent_inode_idx];
    if (parent_inode->internal.file_type != DIRECTORY) {
        REPORT_RETCODE(INVALID_FILE_TYPE);
        return -1;
    }

    if (find_inode_in_dir(context->fs, parent_inode, new_name) != (inode_index_t)-1) {
        REPORT_RETCODE(FILE_EXIST);
        return -1;
    }

    inode_index_t holder_inode_index;
    if (claim_available_inode(context->fs, &holder_inode_index) != SUCCESS) {
        // claim_available_inode already reports the error
        return -1;
    }

    inode_t* new_inode = &context->fs->inodes[holder_inode_index];
    new_inode->internal.file_type = DATA_FILE;
    new_inode->internal.file_perms = perms;
    strncpy(new_inode->internal.file_name, new_name, MAX_FILE_NAME_LEN);
    new_inode->internal.file_size = 0;
    
    if (add_entry_to_directory(context->fs, parent_inode, new_name, holder_inode_index) != SUCCESS) {
        release_inode(context->fs, new_inode);
        REPORT_RETCODE(SYSTEM_ERROR); // No specific code for directory full
        return -1;
    }

    return 0;
}

int new_directory(terminal_context_t *context, char *path)
{
    if(context == NULL || path == NULL || strlen(path) == 0){
        REPORT_RETCODE(INVALID_INPUT);
        return -1;
    }

    char parent_path[256];
    char new_dir_name[MAX_FILE_NAME_LEN];
    if (!split_path(path, parent_path, new_dir_name)) {
        REPORT_RETCODE(INVALID_FILENAME);
        return -1;
    }

    inode_index_t parent_inode_idx = find_inode_for_path(context, parent_path);
    if (parent_inode_idx == (inode_index_t)-1) {
        REPORT_RETCODE(DIR_NOT_FOUND);
        return -1;
    }
    inode_t *parent_inode = &context->fs->inodes[parent_inode_idx];
    if (parent_inode->internal.file_type != DIRECTORY) {
        REPORT_RETCODE(INVALID_FILE_TYPE);
        return -1;
    }

    if (find_inode_in_dir(context->fs, parent_inode, new_dir_name) != (inode_index_t)-1) {
        REPORT_RETCODE(DIRECTORY_EXIST);
        return -1;
    }

    inode_index_t new_dir_inode_idx;
    if (claim_available_inode(context->fs, &new_dir_inode_idx) != SUCCESS) {
        return -1;
    }

    inode_t *new_dir_inode = &context->fs->inodes[new_dir_inode_idx];
    new_dir_inode->internal.file_type = DIRECTORY;
    new_dir_inode->internal.file_perms = FS_READ | FS_WRITE | FS_EXECUTE;
    strncpy(new_dir_inode->internal.file_name, new_dir_name, MAX_FILE_NAME_LEN);
    new_dir_inode->internal.file_size = 0;

    if (add_entry_to_directory(context->fs, new_dir_inode, ".", new_dir_inode_idx) != SUCCESS ||
        add_entry_to_directory(context->fs, new_dir_inode, "..", parent_inode_idx) != SUCCESS) {
        inode_release_data(context->fs, new_dir_inode);
        release_inode(context->fs, new_dir_inode);
        REPORT_RETCODE(SYSTEM_ERROR);
        return -1;
    }

    if (add_entry_to_directory(context->fs, parent_inode, new_dir_name, new_dir_inode_idx) != SUCCESS) {
        inode_release_data(context->fs, new_dir_inode);
        release_inode(context->fs, new_dir_inode);
        REPORT_RETCODE(SYSTEM_ERROR);
        return -1;
    }

    return 0;
}

int remove_file(terminal_context_t *context, char *path)
{
    if(context == NULL || path == NULL){
        REPORT_RETCODE(INVALID_INPUT);
        return -1;
    }

    char parent_path[256];
    char file_to_remove[MAX_FILE_NAME_LEN];
    if (!split_path(path, parent_path, file_to_remove)) {
        REPORT_RETCODE(INVALID_FILENAME);
        return -1;
    }

    inode_index_t file_inode_idx = find_inode_for_path(context, path);
    if (file_inode_idx == (inode_index_t)-1) {
        REPORT_RETCODE(FILE_NOT_FOUND);
        return -1;
    }
    inode_t *file_inode = &context->fs->inodes[file_inode_idx];
    if (file_inode->internal.file_type != DATA_FILE) {
        REPORT_RETCODE(INVALID_FILE_TYPE);
        return -1;
    }

    inode_index_t parent_inode_idx = find_inode_for_path(context, parent_path);
    inode_t *parent_inode = &context->fs->inodes[parent_inode_idx];

    if (remove_entry_from_directory(context->fs, parent_inode, file_to_remove) != SUCCESS) {
        REPORT_RETCODE(FILE_NOT_FOUND);
        return -1;
    }

    inode_release_data(context->fs, file_inode);
    release_inode(context->fs, file_inode);

    return 0;
}

int remove_directory(terminal_context_t *context, char *path)
{
    if(context == NULL || path == NULL){
        REPORT_RETCODE(INVALID_INPUT);
        return -1;
    }

    inode_index_t dir_inode_idx = find_inode_for_path(context, path);
    if (dir_inode_idx == (inode_index_t)-1) {
        REPORT_RETCODE(DIR_NOT_FOUND);
        return -1;
    }
    inode_t *dir_inode = &context->fs->inodes[dir_inode_idx];
    if (dir_inode == context->working_directory) {
        REPORT_RETCODE(ATTEMPT_DELETE_CWD);
        return -1;
    }
    if (dir_inode->internal.file_type != DIRECTORY) {
        REPORT_RETCODE(INVALID_FILE_TYPE);
        return -1;
    }

    if (dir_inode->internal.file_size > (2 * DIRECTORY_ENTRY_SIZE)) {
        REPORT_RETCODE(DIR_NOT_EMPTY);
        return -1;
    }

    char parent_path[256];
    char dir_to_remove[MAX_FILE_NAME_LEN];
    if (!split_path(path, parent_path, dir_to_remove)) {
        REPORT_RETCODE(INVALID_FILENAME);
        return -1;
    }
    inode_index_t parent_inode_idx = find_inode_for_path(context, parent_path);
    inode_t *parent_inode = &context->fs->inodes[parent_inode_idx];

    if (remove_entry_from_directory(context->fs, parent_inode, dir_to_remove) != SUCCESS) {
        REPORT_RETCODE(DIR_NOT_FOUND);
        return -1;
    }

    inode_release_data(context->fs, dir_inode);
    release_inode(context->fs, dir_inode);

    return 0;
}

int change_directory(terminal_context_t *context, char *path)
{
    if(context == NULL || path == NULL){
        REPORT_RETCODE(INVALID_INPUT);
        return -1;
    }
    
    inode_index_t dest_inode_idx = find_inode_for_path(context, path);

    if (dest_inode_idx == (inode_index_t)-1) {
        REPORT_RETCODE(DIR_NOT_FOUND);
        return -1;
    }

    inode_t *dest_inode = &context->fs->inodes[dest_inode_idx];
    if (dest_inode->internal.file_type != DIRECTORY) {
        REPORT_RETCODE(INVALID_FILE_TYPE);
        return -1;
    }

    context->working_directory = dest_inode;

    return 0;
}

int list(terminal_context_t *context, char *path)
{
    if(context == NULL){
        REPORT_RETCODE(INVALID_INPUT);
        return -1;
    }

    inode_t *dir_inode;
    if (path == NULL || strlen(path) == 0) {
        dir_inode = context->working_directory;
    } else {
        inode_index_t dir_inode_idx = find_inode_for_path(context, path);
        if (dir_inode_idx == (inode_index_t)-1) {
            REPORT_RETCODE(DIR_NOT_FOUND);
            return -1;
        }
        dir_inode = &context->fs->inodes[dir_inode_idx];
    }

    if (dir_inode->internal.file_type != DIRECTORY) {
        REPORT_RETCODE(INVALID_FILE_TYPE);
        return -1;
    }

    size_t num_entries = dir_inode->internal.file_size / DIRECTORY_ENTRY_SIZE;
    if (num_entries == 0) return 0;

    byte *buffer = malloc(dir_inode->internal.file_size);
    if (buffer == NULL) {
        REPORT_RETCODE(SYSTEM_ERROR);
        return -1;
    }
    size_t bytes_read;
    inode_read_data(context->fs, dir_inode, 0, buffer, dir_inode->internal.file_size, &bytes_read);

    for (size_t i = 0; i < num_entries; ++i) {
        byte *entry_ptr = buffer + i * DIRECTORY_ENTRY_SIZE;
        inode_index_t entry_inode_idx;
        memcpy(&entry_inode_idx, entry_ptr, sizeof(inode_index_t));
        
        if (entry_inode_idx != (inode_index_t)-1) {
            char *name = (char *)(entry_ptr + sizeof(inode_index_t));
            printf("%s\n", name);
        }
    }
    free(buffer);
    return 0;
}

char *get_path_string(terminal_context_t *context)
{
    REPORT_RETCODE(NOT_IMPLEMENTED);
    return NULL;
}

int tree(terminal_context_t *context, char *path)
{
    REPORT_RETCODE(NOT_IMPLEMENTED);
    return -1;
}

//Part 2
void new_terminal(filesystem_t *fs, terminal_context_t *term)
{
    if (fs == NULL || term == NULL) {
        return;
    }
    term->fs = fs;
    term->working_directory = &fs->inodes[0]; // Start at root
}

fs_file_t fs_open(terminal_context_t *context, char *path)
{
    if (context == NULL || path == NULL) {
        REPORT_RETCODE(INVALID_INPUT);
        return NULL;
    }

    inode_index_t file_inode_idx = find_inode_for_path(context, path);
    if (file_inode_idx == (inode_index_t)-1) {
        REPORT_RETCODE(FILE_NOT_FOUND);
        return NULL;
    }

    inode_t *file_inode = &context->fs->inodes[file_inode_idx];
    if (file_inode->internal.file_type != DATA_FILE) {
        REPORT_RETCODE(INVALID_FILE_TYPE);
        return NULL;
    }

    fs_file_t file = (fs_file_t)malloc(sizeof(struct fs_file));
    if (file == NULL) {
        REPORT_RETCODE(SYSTEM_ERROR);
        return NULL;
    }

    file->fs = context->fs;
    file->inode = file_inode;
    file->offset = 0;

    return file;
}

void fs_close(fs_file_t file)
{
    if (file != NULL) {
        free(file);
    }
}

size_t fs_read(fs_file_t file, void *buffer, size_t n)
{
    if(file == NULL || buffer == NULL){
        return 0;
    }

    size_t bytes_read = 0;
    if (inode_read_data(file->fs, file->inode, file->offset, buffer, n, &bytes_read) != SUCCESS) {
        return 0;
    }
    file->offset += bytes_read;

    return bytes_read;
}

size_t fs_write(fs_file_t file, void *buffer, size_t n)
{
    if(file == NULL || buffer == NULL || n == 0){
        return 0;
    }

    if (inode_modify_data(file->fs, file->inode, file->offset, buffer, n) != SUCCESS) {
        return 0;
    }
    
    file->offset += n;

    return n;
}

int fs_seek(fs_file_t file, seek_mode_t seek_mode, int offset)
{

    if(file == NULL || file->inode == NULL){
        return -1;
    }

    size_t file_size = file->inode->internal.file_size;
    long long new_offset;

    switch(seek_mode) {
        case FS_SEEK_START:
            new_offset = offset;
            break;
        case FS_SEEK_CURRENT:
            new_offset = (long long)file->offset + offset;
            break;
        case FS_SEEK_END:
            new_offset = (long long)file_size + offset;
            break;
        default:
            return -1;
    }

    if (new_offset < 0 || new_offset > (long long)file_size) {
        return -1;
    }

    file->offset = (size_t)new_offset;

    return 0;
}

// ----------------------- HELPER IMPLEMENTATIONS ----------------------- //
static inode_index_t find_inode_in_dir(filesystem_t *fs, inode_t *dir_inode, const char *name) {
    if (dir_inode->internal.file_type != DIRECTORY || dir_inode->internal.file_size == 0) {
        return (inode_index_t)-1;
    }

    size_t num_entries = dir_inode->internal.file_size / DIRECTORY_ENTRY_SIZE;
    byte *dir_data = malloc(dir_inode->internal.file_size);
    if (dir_data == NULL) return (inode_index_t)-1;
    
    size_t bytes_read;
    inode_read_data(fs, dir_inode, 0, dir_data, dir_inode->internal.file_size, &bytes_read);

    inode_index_t found_idx = (inode_index_t)-1;
    for (size_t i = 0; i < num_entries; ++i) {
        byte *entry_ptr = dir_data + i * DIRECTORY_ENTRY_SIZE;
        inode_index_t entry_inode_idx;
        memcpy(&entry_inode_idx, entry_ptr, sizeof(inode_index_t));
        char *entry_name = (char *)(entry_ptr + sizeof(inode_index_t));

        if (entry_inode_idx != (inode_index_t)-1 && strcmp(entry_name, name) == 0) {
            found_idx = entry_inode_idx;
            break;
        }
    }

    free(dir_data);
    return found_idx;
}

static inode_index_t find_inode_for_path(terminal_context_t *context, const char *path) {
    inode_t *start_node;
    const char *path_segment = path;

    if (path[0] == '/') {
        start_node = &context->fs->inodes[0];
        path_segment++;
    } else {
        start_node = context->working_directory;
    }

    if (*path_segment == '\0' || strcmp(path, "/") == 0) {
        return start_node - context->fs->inodes;
    }

    inode_index_t current_inode_idx = start_node - context->fs->inodes;
    char current_name[MAX_FILE_NAME_LEN];
    const char *next_slash;

    while (*path_segment != '\0') {
        inode_t *current_inode = &context->fs->inodes[current_inode_idx];
        if (current_inode->internal.file_type != DIRECTORY) {
            return (inode_index_t)-1;
        }

        next_slash = strchr(path_segment, '/');
        size_t name_len;
        if (next_slash != NULL) {
            name_len = next_slash - path_segment;
            strncpy(current_name, path_segment, name_len);
            current_name[name_len] = '\0';
            path_segment = next_slash + 1;
        } else {
            strcpy(current_name, path_segment);
            path_segment += strlen(path_segment);
        }
        
        if (strlen(current_name) == 0) continue;

        inode_index_t next_inode_idx = find_inode_in_dir(context->fs, current_inode, current_name);
        if (next_inode_idx == (inode_index_t)-1) {
            return (inode_index_t)-1;
        }
        current_inode_idx = next_inode_idx;
    }

    return current_inode_idx;
}

static fs_retcode_t add_entry_to_directory(filesystem_t *fs, inode_t *dir_inode, const char *name, inode_index_t entry_inode_index) {
    byte new_entry[DIRECTORY_ENTRY_SIZE] = {0};
    memcpy(new_entry, &entry_inode_index, sizeof(inode_index_t));
    strncpy((char *)(new_entry + sizeof(inode_index_t)), name, MAX_FILE_NAME_LEN);
    
    return inode_write_data(fs, dir_inode, new_entry, DIRECTORY_ENTRY_SIZE);
}

static fs_retcode_t remove_entry_from_directory(filesystem_t *fs, inode_t *dir_inode, const char *name) {
    size_t dir_size = dir_inode->internal.file_size;
    if (dir_size == 0) return NOT_FOUND;

    byte *dir_data = malloc(dir_size);
    if (dir_data == NULL) return SYSTEM_ERROR;
    size_t bytes_read;
    inode_read_data(fs, dir_inode, 0, dir_data, dir_size, &bytes_read);

    size_t num_entries = dir_size / DIRECTORY_ENTRY_SIZE;
    for (size_t i = 0; i < num_entries; ++i) {
        byte *entry_ptr = dir_data + i * DIRECTORY_ENTRY_SIZE;
        char *entry_name = (char *)(entry_ptr + sizeof(inode_index_t));
        
        if (strcmp(entry_name, name) == 0) {
            inode_index_t unassigned = (inode_index_t)-1;
            memcpy(entry_ptr, &unassigned, sizeof(inode_index_t));
            inode_modify_data(fs, dir_inode, i * DIRECTORY_ENTRY_SIZE, entry_ptr, DIRECTORY_ENTRY_SIZE);
            free(dir_data);
            return SUCCESS;
        }
    }

    free(dir_data);
    return NOT_FOUND;
}