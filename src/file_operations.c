#include "filesys.h"
#include "debug.h"
#include "utility.h"

#include <string.h>

#define DIRECTORY_ENTRY_SIZE (sizeof(inode_index_t) + MAX_FILE_NAME_LEN)
#define DIRECTORY_ENTRIES_PER_DATABLOCK (DATA_BLOCK_SIZE / DIRECTORY_ENTRY_SIZE)

// ----------------------- CORE FUNCTION ----------------------- //
int new_file(terminal_context_t *context, char *path, permission_t perms)
{
    if(context == NULL || path == NULL){
        return 0;
    }
    if(context->fs->available_inode == 0){
        REPORT_RETCODE(INODE_UNAVAILABLE);
        return -1;
    }
    return 0;
}

int new_directory(terminal_context_t *context, char *path)
{
    if(context == NULL || path == NULL){
        return 0;
    }
    return -1;
}

int remove_file(terminal_context_t *context, char *path)
{
    if(context == NULL || path == NULL){
        return 0;
    }
    return -1;
}

// we can only delete a directory if it is empty!!
int remove_directory(terminal_context_t *context, char *path)
{
    if(context == NULL || path == NULL){
        return 0;
    }
    return -1;
}

int change_directory(terminal_context_t *context, char *path)
{
    if(context == NULL || path == NULL){
        return 0;
    }
    return -2;
}

int list(terminal_context_t *context, char *path)
{
    if(context == NULL || path == NULL){
        return 0;
    }
    return -2;
}

char *get_path_string(terminal_context_t *context)
{
    if(context == NULL){
        return 0;
    }

    return NULL;
}

int tree(terminal_context_t *context, char *path)
{
    if(context == NULL || path == NULL){
        return 0;
    }
    return -2;
}

//Part 2
void new_terminal(filesystem_t *fs, terminal_context_t *term)
{
    if(fs == NULL || term == NULL){
        return;
    }
    term->fs = fs;
    term->working_directory = &fs->inodes[0];
}

fs_file_t fs_open(terminal_context_t *context, char *path)
{
    (void) context;
    (void) path;

    //confirm path exists, leads to a file
    //allocate space for the file, assign its fs and inode. Set offset to 0.
    //return file

    return (fs_file_t) 0;
}

void fs_close(fs_file_t file)
{
    if(file == NULL){
        return;
    }
    
    free(file);

    
}

size_t fs_read(fs_file_t file, void *buffer, size_t n)
{
    if(file == NULL){
        return 0;
    }

    inode_t *inode = file->inode;
    size_t bytes_read = 0;

    inode_read_data(file->fs, inode, file->offset, buffer, n, &bytes_read);
    if(file->offset + n > file->inode->internal.file_size){
        file->offset = file->inode->internal.file_size;
    }
    else{
        file->offset += n;
    }

    return bytes_read;
}

size_t fs_write(fs_file_t file, void *buffer, size_t n)
{
    if(file == NULL){
        return 0;
    }

    inode_t *inode = file->inode;
    size_t bytes_written = 0;

    inode_modify_data(file->fs, inode, file->offset, buffer, n);
    bytes_written = n;
    if(file->offset + n > file->inode->internal.file_size){
        file->inode->internal.file_size = file->offset + n;
        file->offset = file->inode->internal.file_size;

    }
    else{
        file->offset += n;
    }

    return bytes_written;
}

int fs_seek(fs_file_t file, seek_mode_t seek_mode, int offset)
{

    if(file == NULL || seek_mode < 0 || seek_mode > 2){
        return -1;
    }
    
    if(file->inode == NULL){
        return -1;
    }

    size_t file_system_total_size = file->inode->internal.file_size;

    

    if(seek_mode == FS_SEEK_START){
        file->offset = offset; 
        if(file->offset > file_system_total_size){
            file->offset = file_system_total_size;
            return 0;
        }
        if(file->offset < file_system_total_size - file_system_total_size){
            return -1;
        }
        return 0;
    }
    else if(seek_mode == FS_SEEK_CURRENT){
        file->offset += offset;
        if(file->offset > file_system_total_size){
            file->offset = file_system_total_size;
            return 0;
        }
        return 0;
        
    }
    else if(seek_mode == FS_SEEK_END){
        file->offset = file_system_total_size + offset;
        if(file->offset < file_system_total_size - file_system_total_size){
            return -1;
        }
        if(file->offset > file_system_total_size){
            file->offset = file_system_total_size;
            return 0;
        }
        return 0;
    }


    return 0;
    
    // Updates the offset stored in file based on the mode seek_mode and the offset.
    // Returns -1 on failure and 0 on a successful seek operations.
    // If the final offset is less than 0, this is a failed operation. No changes should be made to file, i.e. the state of file before the function call must be equal to its state after the function call.
    // If the final offset is greater than the file size, set it to the file size. This is still an succesful seek operation.
    // If the seek mode is FS_SEEK_START, the offset is the offset from the beginning of the file.
    // If the seek mode is FS_SEEK_CURRENT, the offset is the offset from the current offset stored in file.
    // If the seek mode is FS_SEEK_END, the offset is the offset from the end of the file.
}

