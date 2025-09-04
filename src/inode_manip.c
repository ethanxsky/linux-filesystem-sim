#include "filesys.h"

#include <string.h>
#include <assert.h>

#include "utility.h"
#include "debug.h"

#define INDIRECT_DBLOCK_INDEX_COUNT (DATA_BLOCK_SIZE / sizeof(dblock_index_t) - 1)
#define INDIRECT_DBLOCK_MAX_DATA_SIZE ( DATA_BLOCK_SIZE * INDIRECT_DBLOCK_INDEX_COUNT )

#define NEXT_INDIRECT_INDEX_OFFSET (DATA_BLOCK_SIZE - sizeof(dblock_index_t))
#define MIN(a, b) ((a) < (b) ? (a) : (b))
// ----------------------- UTILITY FUNCTION ----------------------- //

void fill_dblock(filesystem_t *fs, inode_t *inode, void* data, dblock_index_t dblockindex, size_t n, size_t *dataoffset){

    if(*dataoffset == 0){
        size_t initial_offset = inode->internal.file_size % DATA_BLOCK_SIZE;
        for(int i = 0; i < DATA_BLOCK_SIZE - (int)initial_offset; i++){
            if((*dataoffset < DATA_BLOCK_SIZE - initial_offset) && *dataoffset < n){
                fs->dblocks[dblockindex * DATA_BLOCK_SIZE + initial_offset + i] = *((byte*)data + (*dataoffset));
                (*dataoffset)++;
            }
        }
    }
    else{
        for(int i = 0; i < DATA_BLOCK_SIZE; i++){
            if(*dataoffset < n){
                fs->dblocks[dblockindex * DATA_BLOCK_SIZE + i] = *((byte*)data + (*dataoffset));
                (*dataoffset)++;

            }
            
        }
    }


    return;        
}

// ----------------------- CORE FUNCTION ----------------------- //




fs_retcode_t inode_write_data(filesystem_t *fs, inode_t *inode, void *data, size_t n)
{

    if(fs == NULL || inode == NULL){
        return INVALID_INPUT;
    }

    //PRINTING INODES DIRECT DATA ARRAY BEFORE WRITING

    //printf("\nTHE INODE DIRECT DATA BEFORE WRITING IS ");
    // for(int i = 0; i < INODE_DIRECT_BLOCK_COUNT; i++){
    //     printf("%d, ", inode->internal.direct_data[i]);
    // }
    // printf("\n");

    //display_filesystem(fs, 7);
    size_t avail_dblock = available_dblocks(fs);
    size_t neces_dblock = calculate_necessary_dblock_amount(n); //necessary dblocks for size n
    size_t initial_dblock_amount = calculate_necessary_dblock_amount(inode->internal.file_size);
    size_t index_dblock_amount = calculate_index_dblock_amount(n + inode->internal.file_size);
    dblock_index_t starting_dblock = inode->internal.file_size / DATA_BLOCK_SIZE; //DOES NOT MEAN THE INDEX OF THE DBLOCK, MEANS THE FIRST,SECOND,THIRD, ETC DBLOCK IN TOTAL

    
    //printf("\n THE AMOUNT OF INDEX DBLOCKS IS %lu \n", index_dblock_amount);

    if(avail_dblock < neces_dblock){
        return INSUFFICIENT_DBLOCKS;
    }

    size_t dataoffset = 0;
    //printf("\nTHE STARTING DBLOCK HELD IN STARTING_DBLOCK IS %d", starting_dblock);

    size_t dblockallocatedcounter = 0;

    //WRITE TO DIRECT BLOCKS ONLY
    if(index_dblock_amount == 0){
        for(size_t i = 0; i < neces_dblock; i++){
            //printf("\nCLAIMING %lu\n", i +1);
            claim_available_dblock(fs, &inode->internal.direct_data[initial_dblock_amount + i]);
        }

        for(int i = starting_dblock; i < INODE_DIRECT_BLOCK_COUNT; i++){
            fill_dblock(fs, inode, data, inode->internal.direct_data[i], n, &dataoffset);
        }
        inode->internal.file_size += n;

    } //DIRECT BLOCK END
    //INDIRECT BLOCK WRITING
    else{
        //printf("\n\n\n\n\n\n\n\n\n");
        //printf("\nTHE NECESSARY TOTAL AMOUNT OF DBLOCKS IS %lu\n", neces_dblock + index_dblock_amount);
        //CHECK FOR DIRECTBLOCKS FIRST
        int pre_allocated_index_block_in_inode = calculate_index_dblock_amount(inode->internal.file_size);
        int pre_allocated_direct_dblock_in_inode = calculate_necessary_dblock_amount(inode->internal.file_size) - pre_allocated_index_block_in_inode;
        int direct_dblock_TBA = INODE_DIRECT_BLOCK_COUNT - pre_allocated_direct_dblock_in_inode;

        if(direct_dblock_TBA > 0){
            //printf("\nHAVE TO ALLOCATE DIRECT DBLOCKS BEFORE INDIRECT DBLOCKS\n");
            for(size_t i = 0; i < INODE_DIRECT_BLOCK_COUNT; i++){
                if(claim_available_dblock(fs, &inode->internal.direct_data[starting_dblock + i]) == SUCCESS){
                    //printf("\nALLOCATED %lu DIRECT BLOCKS\n", i + 1); 
                    //printf("    THIS IS A TEST MESSAGE      THE VALUE HELD AT INODE->INTERNAL.DIRECT_DATA[INDEX] IS %d\n", inode->internal.direct_data[starting_dblock + i] );

                    direct_dblock_TBA--;
                    dblockallocatedcounter++;
                }
                if(direct_dblock_TBA == 0){
                    break;
                }
            }

            for(size_t i = starting_dblock; i < INODE_DIRECT_BLOCK_COUNT; i++){
            fill_dblock(fs, inode, data, inode->internal.direct_data[i], n, &dataoffset);
            }
        }

        //printf("\nTHE NUMBER OF DBLOCKS ALLOCATED SO FAR IS %lu\n", dblockallocatedcounter);
        //int indirect_direct_dblockTBA = neces_dblock - dblockallocatedcounter;
        //printf("\nTHE INDEX DBLOCKS TO BE ALLOCATED IS %lu\n", index_dblock_amount);
        //printf("\nTHE INDIRECT DIRECT DBLOCKS TO BE ALLOCATED IS %d\n", indirect_direct_dblockTBA);



        if (pre_allocated_index_block_in_inode == 0) {
            if (claim_available_dblock(fs, &inode->internal.indirect_dblock) != SUCCESS) {
                return INSUFFICIENT_DBLOCKS;
            }
        
            dblock_index_t current_index_block = inode->internal.indirect_dblock;
            dblock_index_t offset = 0;
        
            while (dataoffset < n) {
                for (int i = offset; i < 15; i++) {
                    dblock_index_t dblock_index_from_indirect = 0;
        
                    if (claim_available_dblock(fs, &dblock_index_from_indirect) != SUCCESS) {
                        return INSUFFICIENT_DBLOCKS;
                    }
        
                    memcpy(&fs->dblocks[current_index_block * DATA_BLOCK_SIZE + i * sizeof(dblock_index_t)], 
                           &dblock_index_from_indirect, sizeof(dblock_index_t));
        
                    fill_dblock(fs, inode, data, dblock_index_from_indirect, n, &dataoffset);
        
                    if (dataoffset >= n) {
                        break;
                    }
                }
        
                if (dataoffset < n) {
                    dblock_index_t next_index_block = 0;
        
                    if (claim_available_dblock(fs, &next_index_block) != SUCCESS) {
                        return INSUFFICIENT_DBLOCKS;
                    }
        
                    memcpy(&fs->dblocks[current_index_block * DATA_BLOCK_SIZE + 60], 
                           &next_index_block, sizeof(dblock_index_t));
        
                    current_index_block = next_index_block;
                    offset = 0;
                }
            }
        
            inode->internal.file_size += n;
            return SUCCESS;
        }
        else{
            dblock_index_t index_block = inode->internal.indirect_dblock;
            dblock_index_t current_index_block = index_block;
            dblock_index_t offset = (starting_dblock - INODE_DIRECT_BLOCK_COUNT) % 15;

            for (int i = 1; i < pre_allocated_index_block_in_inode; i++) {
                memcpy(&current_index_block, &fs->dblocks[current_index_block * DATA_BLOCK_SIZE + 60], sizeof(dblock_index_t));
            }

            //temp_offset = offset * sizeof(dblock_index_t);
            dblockallocatedcounter = 0;

            while (dataoffset < n) {
                for (int i = offset; i < 15; i++) {
                    dblock_index_t dblock_index_from_indirect = 0;

                    memcpy(&dblock_index_from_indirect, &fs->dblocks[current_index_block * DATA_BLOCK_SIZE + i * 4], sizeof(dblock_index_t));
                    if (dblock_index_from_indirect == 0) {
                        if (claim_available_dblock(fs, &dblock_index_from_indirect) != SUCCESS) {
                            return INSUFFICIENT_DBLOCKS;
                        }

                        memcpy(&fs->dblocks[current_index_block * DATA_BLOCK_SIZE + i * 4], &dblock_index_from_indirect, sizeof(dblock_index_t));
                    }

                    fill_dblock(fs, inode, data, dblock_index_from_indirect, n, &dataoffset);

                    if (dataoffset >= n) {
                        break;
                    }
                }

                if (dataoffset < n) {
                    dblock_index_t next_index_block = 0;
                    memcpy(&next_index_block, &fs->dblocks[current_index_block * DATA_BLOCK_SIZE + 60], sizeof(dblock_index_t));

                    if (next_index_block == 0) {
                        if (claim_available_dblock(fs, &next_index_block) != SUCCESS) {
                            return INSUFFICIENT_DBLOCKS;
                        }

                        memcpy(&fs->dblocks[current_index_block * DATA_BLOCK_SIZE + 60], &next_index_block, sizeof(dblock_index_t));
                    }

                    current_index_block = next_index_block;
                    offset = 0; 
                }
            }

            inode->internal.file_size += n;

            return SUCCESS;
        }
        
    }

    return SUCCESS;

}





fs_retcode_t inode_read_data(filesystem_t *fs, inode_t *inode, size_t offset, void *buffer, size_t n, size_t *bytes_read)
{
    if(fs == NULL || inode == NULL || bytes_read == NULL){
        return INVALID_INPUT;
    }

    *bytes_read = 0;

    if(offset + n < DATA_BLOCK_SIZE * 4){
        //printf("\nThe offset is %lu\n", offset);
        inode_index_t starting_block;
        if(offset == 0){
            starting_block = 0;
        }
        else{
            starting_block = offset / DATA_BLOCK_SIZE;
        }

        //printf("\nTHE STARTING BLOCK SHOULD BE %d\n", starting_block);
        inode_index_t dblock_index_holder = 0;
        byte *temp_ptr = (byte*) buffer;

        for(int i = starting_block; i < INODE_DIRECT_BLOCK_COUNT; i++){
            for(int x = 0; x < DATA_BLOCK_SIZE; x++){
                if(i == starting_block){
                    dblock_index_holder = inode->internal.direct_data[i];
                    temp_ptr[*bytes_read] = fs->dblocks[dblock_index_holder * DATA_BLOCK_SIZE + (offset % DATA_BLOCK_SIZE) + x];
                    (*bytes_read)++;
                    if((*bytes_read) == n || (*bytes_read) >= inode->internal.file_size){
                        return SUCCESS;
                    }
                }
                else{
                    dblock_index_holder = inode->internal.direct_data[i];
                    temp_ptr[*bytes_read] = fs->dblocks[dblock_index_holder * DATA_BLOCK_SIZE + x];
                    (*bytes_read)++;
                    if((*bytes_read) == n || (*bytes_read) >= inode->internal.file_size){
                        return SUCCESS;
                    }
                }
            }
        }
        
    }
    else{
        //printf("\nREAD SIZE MORE THAN DIRECT DATA\n");
        dblock_index_t starting_block = 0;
        dblock_index_t dblock_index_holder = 0;
        byte *temp_ptr = (byte*) buffer;
        
        if(offset < DATA_BLOCK_SIZE * 4){
            //printf("\nSTARTING READING FROM  DIRECT BLOCKS WHEN OFFSET AND READ MORE THAN DIRECT BUT OFFSET LESS TAN DIRECT\n");

            if(offset == 0){
                starting_block = 0;
            }
            else if(offset < DATA_BLOCK_SIZE * 4){
                starting_block = offset / DATA_BLOCK_SIZE;
            }

            for(size_t i = starting_block; i < INODE_DIRECT_BLOCK_COUNT; i++){
                for(int x = 0; x < DATA_BLOCK_SIZE; x++){
                    if(i == starting_block){
                        dblock_index_holder = inode->internal.direct_data[i];
                        temp_ptr[*bytes_read] = fs->dblocks[dblock_index_holder * DATA_BLOCK_SIZE + (offset % DATA_BLOCK_SIZE) + x];
                        (*bytes_read)++;
                        if((*bytes_read) == n || (*bytes_read) >= inode->internal.file_size - offset){
                            return SUCCESS;
                        }
                    }
                    else{
                        dblock_index_holder = inode->internal.direct_data[i];
                        temp_ptr[*bytes_read] = fs->dblocks[dblock_index_holder * DATA_BLOCK_SIZE + x];
                        (*bytes_read)++;
                        if((*bytes_read) == n || (*bytes_read) >= inode->internal.file_size - offset){
                            return SUCCESS;
                        }
                    }
                }
            }

            dblock_index_t neces_index_dblock = calculate_index_dblock_amount(n + offset);
            dblock_index_t indirect_block_holder = 0;
            dblock_index_holder = 0;

            for(size_t i = 0; i < neces_index_dblock; i++){
                if(i == 0){
                    indirect_block_holder = inode->internal.indirect_dblock;
                    
                    for(int indirectoffset = 0; indirectoffset < 15; indirectoffset++){
                        memcpy(&dblock_index_holder, &fs->dblocks[indirect_block_holder * DATA_BLOCK_SIZE + (indirectoffset * 4)], sizeof(dblock_index_t));
                        for(int withinblockoffset = 0; withinblockoffset < DATA_BLOCK_SIZE; withinblockoffset++){
                            temp_ptr[*bytes_read] = fs->dblocks[dblock_index_holder * DATA_BLOCK_SIZE + withinblockoffset];
                            (*bytes_read)++;
                            
                            if((*bytes_read) == n || (*bytes_read) >= inode->internal.file_size - offset){
                                return SUCCESS;
                            }
                        }
                    }
                }
                else{
                    dblock_index_t temp_dblock = indirect_block_holder;
                    memcpy(&indirect_block_holder, &fs->dblocks[temp_dblock * DATA_BLOCK_SIZE + 60], sizeof(dblock_index_t));

                    for(int indirectoffset = 0; indirectoffset < 15; indirectoffset++){
                        memcpy(&dblock_index_holder, &fs->dblocks[indirect_block_holder * DATA_BLOCK_SIZE + (indirectoffset * 4)], sizeof(dblock_index_t));
                        for(int withinblockoffset = 0; withinblockoffset < DATA_BLOCK_SIZE; withinblockoffset++){
                            temp_ptr[*bytes_read] = fs->dblocks[dblock_index_holder * DATA_BLOCK_SIZE + withinblockoffset];
                            (*bytes_read)++;

                            if((*bytes_read) == n || (*bytes_read) >= inode->internal.file_size - offset){
                                return SUCCESS;
                            }
                        }
                    }

                }
            }

        }
        else{
            //printf("\nSTARTING READING FROM AFTER DIRECT BLOCKS WHEN OFFSET AND READ MORE THAN DIRECT\n");

            size_t adjusted_offset = offset - (DATA_BLOCK_SIZE * INODE_DIRECT_BLOCK_COUNT);
            size_t indirect_block_index = adjusted_offset / DATA_BLOCK_SIZE;
            size_t offset_within_block = adjusted_offset % DATA_BLOCK_SIZE;

            dblock_index_t indirect_block_holder = inode->internal.indirect_dblock;
            dblock_index_t dblock_index_holder = 0;
            byte *temp_ptr = (byte*) buffer;


            while (*bytes_read < n && offset < inode->internal.file_size) {
                if (indirect_block_index >= 15) {
                    dblock_index_t temp_dblock = indirect_block_holder;
                    memcpy(&indirect_block_holder, &fs->dblocks[temp_dblock * DATA_BLOCK_SIZE + 60], sizeof(dblock_index_t));
                    indirect_block_index -= 15;
                }

                memcpy(&dblock_index_holder, &fs->dblocks[indirect_block_holder * DATA_BLOCK_SIZE + (indirect_block_index * 4)], sizeof(dblock_index_t));

                for (int i = offset_within_block; i < DATA_BLOCK_SIZE; i++) {
                    temp_ptr[*bytes_read] = fs->dblocks[dblock_index_holder * DATA_BLOCK_SIZE + i];
                    (*bytes_read)++;
                    offset++;  

                    if (*bytes_read == n || offset >= inode->internal.file_size) {
                        return SUCCESS;
                    }
                }

                offset_within_block = 0;
                indirect_block_index++;
            }

        }


    }


    return SUCCESS;
    
}

fs_retcode_t inode_modify_data(filesystem_t *fs, inode_t *inode, size_t offset, void *buffer, size_t n)
{


    if(fs == NULL || inode == NULL || offset > inode->internal.file_size){
        return INVALID_INPUT;
    }

    size_t initial_indexblocks = calculate_index_dblock_amount(inode->internal.file_size);
    size_t initial_totalblocks = calculate_necessary_dblock_amount(inode->internal.file_size);
    size_t initial_dblocks = initial_totalblocks - initial_indexblocks;

    size_t avail_dblock = available_dblocks(fs);
    size_t neces_dblock = calculate_necessary_dblock_amount(n);
    size_t neces_indexblock = calculate_index_dblock_amount(n + offset);

    if(avail_dblock < neces_dblock){
        //printf("\nTHE AVAILABLE BLOCKS IS %lu AND THE NECESSARY BLOCKS IS %lu", avail_dblock, neces_dblock);
        return INSUFFICIENT_DBLOCKS;
    }



    //printf("\nTHE INITIAL DBLOCKS IS %lu\n", initial_dblocks);

    //display_filesystem(fs, 3);
    
    //printf("\n\n\n\n\n\n");
    //printf("\nThe initial size is %lu\n", inode->internal.file_size);
    //printf("\nThe size of data to read is %lu and the offset is %lu\n", n, offset);

    size_t bytes_read = 0;
    byte *temp_ptr = buffer;
    dblock_index_t starting_block = 0;
    int starting_byte = 0;


    if(offset + n < (DATA_BLOCK_SIZE * 4)){
        //printf("\nAAAAAAAH\n\n\n\n\n\n");

        if(offset == 0){
            starting_block = 0;
        }
        else{
            starting_block = offset / DATA_BLOCK_SIZE;
            starting_byte = offset % DATA_BLOCK_SIZE;
        }


        for(size_t i = starting_block; i < INODE_DIRECT_BLOCK_COUNT; i++){
            //printf("\nTHE VALUE OF I IS %lu\n", i);

            //printf("\n HERE %lu\n", i);

            if(i > initial_dblocks - 1){
                claim_available_dblock(fs, &inode->internal.direct_data[i]);
                //printf("\nNEED TO CLAIM NEW DIRECT BLOCK\n");
            }


            if(i == starting_block){
                dblock_index_t dblock_index_holder = inode->internal.direct_data[i];
                for(int x = starting_byte; x < DATA_BLOCK_SIZE; x++){
                    fs->dblocks[dblock_index_holder * DATA_BLOCK_SIZE + x] = temp_ptr[bytes_read];
                    bytes_read++;
                    if(bytes_read + offset > inode->internal.file_size){
                        inode->internal.file_size++;
                        //printf("\n\n\nADDED TO FILE SIZE\n\n\n");
                    }

                    if(bytes_read == n){
                        //printf("\nThe new size is %lu\n\n\n\n\n\n", inode->internal.file_size);
                        //display_filesystem(fs, 3);
                        return SUCCESS;
                    }
                }
            }
            else{
                dblock_index_t dblock_index_holder = inode->internal.direct_data[i];
                for(int x = 0; x < DATA_BLOCK_SIZE; x++){
                    fs->dblocks[dblock_index_holder * DATA_BLOCK_SIZE + x] = temp_ptr[bytes_read];
                    bytes_read++;

                    if(bytes_read + offset > inode->internal.file_size){
                        inode->internal.file_size++;
                        //printf("\n\n\nADDED TO FILE SIZE\n\n\n");

                    }

                    if(bytes_read == n){
                        //printf("\nThe new size is %lu\n\n\n\n\n\n", inode->internal.file_size);

                        //display_filesystem(fs, 3);
                        return SUCCESS;
                    }

                }

            }
            
        }
    }
    else{
        //printf("\nHAVE TO MODIFY MORE THAN DIRECT DATA\n");
        if(offset < DATA_BLOCK_SIZE * 4){
            //printf("\nAAAHA\n");
            if(offset == 0){
                starting_block = 0;
            }
            else if(offset < DATA_BLOCK_SIZE * 4){
                starting_block = offset / DATA_BLOCK_SIZE;
                starting_byte = offset % DATA_BLOCK_SIZE;
            }
            //printf("\nTHE STARTING BLOCK IS %d\n", starting_block);
            //display_filesystem(fs, 3);
            //printf("\n\n\n\n");
            // for(int z = 0; z < 4; z++){
            //     printf("%d, ", inode->internal.direct_data[z]);
            // }
            for(size_t i = starting_block; i < INODE_DIRECT_BLOCK_COUNT; i++){
                //printf("\nTHE DBLOCK BEING WRITTEN IS %d\n", inode->internal.direct_data[i]);
                if(i > initial_dblocks - 1){
                    claim_available_dblock(fs, &inode->internal.direct_data[i]);
                    //printf("\nNEED TO CLAIM NEW DIRECT BLOCK\n");
                    //printf("\nTHE NEW DBLOCK BEING WRITTEN IS %d\n", inode->internal.direct_data[i]);

                }

                if(i == starting_block){
                    dblock_index_t dblock_index_holder = inode->internal.direct_data[i];
                    for(int x = starting_byte; x < DATA_BLOCK_SIZE; x++){
                        fs->dblocks[dblock_index_holder * DATA_BLOCK_SIZE + x] = temp_ptr[bytes_read];
                        bytes_read++;
                        if(bytes_read + offset > inode->internal.file_size){
                            inode->internal.file_size++;
                            //printf("\n\n\nADDED TO FILE SIZE\n\n\n");
                        }

                        if(bytes_read == n){
                            //printf("\nThe new size is %lu\n\n\n\n\n\n", inode->internal.file_size);
                            //display_filesystem(fs, 3);
                            return SUCCESS;
                        }
                    }
                }
                else{
                    dblock_index_t dblock_index_holder = inode->internal.direct_data[i];
                    for(int x = 0; x < DATA_BLOCK_SIZE; x++){
                        fs->dblocks[dblock_index_holder * DATA_BLOCK_SIZE + x] = temp_ptr[bytes_read];
                        bytes_read++;

                        if(bytes_read + offset > inode->internal.file_size){
                            inode->internal.file_size++;
                            //printf("\n\n\nADDED TO FILE SIZE\n\n\n");

                        }

                        if(bytes_read == n){
                            //printf("\nThe new size is %lu\n\n\n\n\n\n", inode->internal.file_size);

                            //display_filesystem(fs, 3);
                            return SUCCESS;
                        }

                    }
                }
            }
            dblock_index_t dblock_index_holder = 0;
            dblock_index_t indirect_block_holder = 0;

            if(initial_indexblocks == 0){
                claim_available_dblock(fs, &inode->internal.indirect_dblock);
            }

            //printf("\nThe index block is %d\n", inode->internal.indirect_dblock);

            for(size_t y = 0; y < neces_indexblock; y++){
                if(y == 0){
                    indirect_block_holder = inode->internal.indirect_dblock;
                    for(int indirectoffset = 0; indirectoffset < 15; indirectoffset++){
                        if((bytes_read + offset) / DATA_BLOCK_SIZE > initial_dblocks - 4 ){
                            claim_available_dblock(fs, cast_dblock_ptr(&fs->dblocks[indirect_block_holder * DATA_BLOCK_SIZE + (indirectoffset * 4)]));
                            memcpy(&dblock_index_holder, &fs->dblocks[indirect_block_holder * DATA_BLOCK_SIZE + (indirectoffset * 4)], sizeof(dblock_index_t));
                        }
                        else{
                            memcpy(&dblock_index_holder, &fs->dblocks[indirect_block_holder * DATA_BLOCK_SIZE + (indirectoffset * 4)], sizeof(dblock_index_t));
                        }

                        for(int offset_within_block = 0; offset_within_block < DATA_BLOCK_SIZE; offset_within_block++){
                            fs->dblocks[dblock_index_holder * DATA_BLOCK_SIZE + offset_within_block] = temp_ptr[bytes_read];
                            bytes_read++;

                            if(bytes_read + offset > inode->internal.file_size){
                                inode->internal.file_size++;
                                //printf("\n\n\nADDED TO FILE SIZE\n\n\n");
                            }
    
                            if(bytes_read == n){
                                //printf("\nThe new size is %lu\n\n\n\n\n\n", inode->internal.file_size);
                                //display_filesystem(fs, 3);
                                return SUCCESS;
                            }
                        }
                    }
                }
                else{
                    dblock_index_t temp_dblock = indirect_block_holder;
                    memcpy(&indirect_block_holder, &fs->dblocks[temp_dblock * DATA_BLOCK_SIZE + 60], sizeof(dblock_index_t));

                    for(int indirectoffset = 0; indirectoffset < 15; indirectoffset++){
                        if((bytes_read + offset) / DATA_BLOCK_SIZE > initial_dblocks - 4){
                            claim_available_dblock(fs, cast_dblock_ptr(&fs->dblocks[indirect_block_holder * DATA_BLOCK_SIZE + (indirectoffset * 4)]));
                            memcpy(&dblock_index_holder, &fs->dblocks[indirect_block_holder * DATA_BLOCK_SIZE + (indirectoffset * 4)], sizeof(dblock_index_t));

                        }
                        else{
                            memcpy(&dblock_index_holder, &fs->dblocks[indirect_block_holder * DATA_BLOCK_SIZE + (indirectoffset * 4)], sizeof(dblock_index_t));
                        }

                        for(int offset_within_block = 0; offset_within_block < DATA_BLOCK_SIZE; offset_within_block++){
                            fs->dblocks[dblock_index_holder * DATA_BLOCK_SIZE + offset_within_block] = temp_ptr[bytes_read];
                            bytes_read++;

                            if(bytes_read + offset > inode->internal.file_size){
                                inode->internal.file_size++;
                                //printf("\n\n\nADDED TO FILE SIZE\n\n\n");
                            }
    
                            if(bytes_read == n){
                                
                                //printf("\nThe new size is %lu\n\n\n\n\n\n", inode->internal.file_size);
                                //display_filesystem(fs, 3);
                                return SUCCESS;
                            }
                        }
                    }
                }
            }
            

        }
        else {
            //printf("\\nHERE\n");
            size_t block_index = offset / DATA_BLOCK_SIZE; 
            size_t byte_offset = offset % DATA_BLOCK_SIZE;
        
            size_t indirect_logical_index = block_index - INODE_DIRECT_BLOCK_COUNT; 
        
            dblock_index_t indirect_block = inode->internal.indirect_dblock;
        
            if (indirect_block == 0) {
                if (claim_available_dblock(fs, &inode->internal.indirect_dblock) != SUCCESS) {
                    return INSUFFICIENT_DBLOCKS;
                }
                indirect_block = inode->internal.indirect_dblock;
            }
        
            dblock_index_t dblock_index_holder = 0;
            byte *temp_ptr = buffer;
            size_t bytes_read = 0;
        
            size_t indirect_block_num = indirect_logical_index / 15; 
            size_t indirect_offset = indirect_logical_index % 15;    
        
            for (size_t i = 0; i < indirect_block_num; i++) {
                dblock_index_t next_indirect;
                memcpy(&next_indirect, &fs->dblocks[indirect_block * DATA_BLOCK_SIZE + 60], sizeof(dblock_index_t));
        
                if (next_indirect == 0) {
                    if (claim_available_dblock(fs, cast_dblock_ptr(&fs->dblocks[indirect_block * DATA_BLOCK_SIZE + 60])) != SUCCESS) {
                        return INSUFFICIENT_DBLOCKS;
                    }
                    memcpy(&next_indirect, &fs->dblocks[indirect_block * DATA_BLOCK_SIZE + 60], sizeof(dblock_index_t));
                }
        
                indirect_block = next_indirect;
            }
        
            while (bytes_read < n) {
                dblock_index_t *dblock_ptr = cast_dblock_ptr(&fs->dblocks[indirect_block * DATA_BLOCK_SIZE + (indirect_offset * 4)]);
        
                if (*dblock_ptr == 0) {
                    if (claim_available_dblock(fs, dblock_ptr) != SUCCESS) {
                        return INSUFFICIENT_DBLOCKS;
                    }
                }
        
                dblock_index_holder = *dblock_ptr;
        
                size_t start = (bytes_read == 0) ? byte_offset : 0;
        
                for (size_t i = start; i < DATA_BLOCK_SIZE; i++) {
                    fs->dblocks[dblock_index_holder * DATA_BLOCK_SIZE + i] = temp_ptr[bytes_read];
                    bytes_read++;
        
                    if (bytes_read + offset > inode->internal.file_size) {
                        inode->internal.file_size++;
                    }
        
                    if (bytes_read == n) {
                        return SUCCESS;
                    }
                }
        
                indirect_offset++;
        
                if (indirect_offset == 15) {
                    dblock_index_t next_indirect;
                    memcpy(&next_indirect, &fs->dblocks[indirect_block * DATA_BLOCK_SIZE + 60], sizeof(dblock_index_t));
        
                    if (next_indirect == 0) {
                        if (claim_available_dblock(fs, cast_dblock_ptr(&fs->dblocks[indirect_block * DATA_BLOCK_SIZE + 60])) != SUCCESS) {
                            return INSUFFICIENT_DBLOCKS;
                        }
                        memcpy(&next_indirect, &fs->dblocks[indirect_block * DATA_BLOCK_SIZE + 60], sizeof(dblock_index_t));
                    }
        
                    indirect_block = next_indirect;
                    indirect_offset = 0;
                }
            }
        
            return SUCCESS;
        }
        

    }


    return SUCCESS;

    //check to see if the input is valid

    //calculate the final filesize and verify there are enough blocks to support it
    //use calculate_necessary_dblock_amount and available_dblocks


    //Write to existing data in your inode

    //For the new data, call "inode_write_data" and return
}

fs_retcode_t inode_shrink_data(filesystem_t *fs, inode_t *inode, size_t new_size)
{
    if(fs == NULL || inode == NULL || new_size > inode->internal.file_size){
        return INVALID_INPUT;
    }
    if(new_size == 0){
        inode_release_data(fs, inode);
        inode->internal.file_size = 0;
        return SUCCESS;
    }

    display_filesystem(fs, 3);
     size_t initial_dblocks = calculate_necessary_dblock_amount(inode->internal.file_size) - calculate_index_dblock_amount(inode->internal.file_size) - INODE_DIRECT_BLOCK_COUNT;
     //size_t neces_dblocks_for_new_size = calculate_necessary_dblock_amount(new_size) - INODE_DIRECT_BLOCK_COUNT;
     size_t neces_dblocks_for_removal = (inode->internal.file_size - new_size) / DATA_BLOCK_SIZE;


     printf("Initial number of data blocks: %zu\n", initial_dblocks);
     //printf("Neces dblocks for new size: %lu\n", neces_dblocks_for_new_size);

     printf("Blocks to remove: %lu\n", neces_dblocks_for_removal);

     printf("New size: %zu\n", new_size);

     dblock_index_t starting_block = inode->internal.file_size / DATA_BLOCK_SIZE;
     dblock_index_t last_block = new_size / DATA_BLOCK_SIZE;

     if(inode->internal.file_size <= DATA_BLOCK_SIZE * 4){
        //tracker_for_first_block_removed = inode->internal.file_size % DATA_BLOCK_SIZE;
        //bytes_removed += tracker_for_first_block_removed;
        for(int i = starting_block; i >= 0; i--){
            dblock_index_t dblock_index_holder = inode->internal.direct_data[i];
            //release_dblock(fs, &fs->dblocks[dblock_index_holder * 64]);
            if(i == (int)last_block){

                if(new_size % DATA_BLOCK_SIZE == 0){
                    release_dblock(fs, &fs->dblocks[dblock_index_holder * 64]);
                }

                inode->internal.file_size = new_size;
                printf("\n\n\n");
                display_filesystem(fs, 3);
                return SUCCESS;
            }
            else{
                release_dblock(fs, &fs->dblocks[dblock_index_holder * 64]);

            }   

        }

     }
     else{
        //dblock_index_t starting_block_within_index = new_size / DATA_BLOCK_SIZE - INODE_DIRECT_BLOCK_COUNT;
        //dblock_index_t starting_index_block = calculate_index_dblock_amount(new_size);
        dblock_index_t indirect_block_holder = inode->internal.indirect_dblock;
        //dblock_index_t direct_blocks_removed = 0;

        if(new_size < DATA_BLOCK_SIZE * 4){
            printf("\nShrinking from indirect into direct\n");
        }
        else{
            printf("\nNEEDED2\n");
            dblock_index_t starting_block_within_index = (new_size / DATA_BLOCK_SIZE - INODE_DIRECT_BLOCK_COUNT) % 15;
            printf("\nThe starting block within index is %d\n", starting_block_within_index);
            size_t starting_index_block = calculate_index_dblock_amount(new_size);
            printf("\nThe staritng index block is %lu\n", starting_index_block);
            dblock_index_t starting_index_holder = 0;
            int starting_byte_within_starting_block = new_size % DATA_BLOCK_SIZE;
            printf("\nThe starting byte is: %d\n", starting_byte_within_starting_block);

            printf("\nIndirect block holder at beginning is %d\n", indirect_block_holder);

            for(size_t i = 0; i < starting_index_block - 1; i++){
                dblock_index_t temp_block = indirect_block_holder;
                memcpy(&indirect_block_holder, &fs->dblocks[temp_block * DATA_BLOCK_SIZE + 60], sizeof(dblock_index_t));
            }
            starting_index_holder = indirect_block_holder;
            printf("\nThe starting indirect block is %d\n", indirect_block_holder);


            for(size_t i = 0; i < neces_dblocks_for_removal; i++){

                if(indirect_block_holder == starting_index_holder && i %16 == 0 && starting_byte_within_starting_block == 0 && starting_block_within_index == 0){
                    dblock_index_t temp_block = indirect_block_holder;
                    release_dblock(fs, &fs->dblocks[temp_block * DATA_BLOCK_SIZE]);
                    memcpy(&indirect_block_holder, &fs->dblocks[temp_block * DATA_BLOCK_SIZE + 60], sizeof(dblock_index_t));
                }
                
                if(i != 0 && i % 16 == 0){
                    dblock_index_t temp_block = indirect_block_holder;
                    memcpy(&indirect_block_holder, &fs->dblocks[temp_block * DATA_BLOCK_SIZE + 60], sizeof(dblock_index_t));
                }

                dblock_index_t dblock_index_holder = 0;
                if(indirect_block_holder == starting_index_holder){
                    dblock_index_t temp_block = 0;
                    printf("\ni mod 16 is: %lu", i%16);
                    memcpy(&temp_block, &fs->dblocks[(indirect_block_holder * DATA_BLOCK_SIZE) + ((starting_block_within_index  + (i % 16))* 4)], sizeof(dblock_index_t));
                    printf("\nthe indirect block is: %d", indirect_block_holder);
                    printf("\nthe temp block index is: %d\n", temp_block);
                    dblock_index_holder = temp_block;
                    //memcpy(&dblock_index_holder, &temp_block, sizeof(dblock_index_t));
                    printf("\nthe dblock index is: %d\n", dblock_index_holder);
                }
                else{
                    printf("\n2222the indirect block is: %d", indirect_block_holder);
                    dblock_index_t temp_block = 0;
                    memcpy(&temp_block, &fs->dblocks[indirect_block_holder * DATA_BLOCK_SIZE +  ((i % 16) * 4)], sizeof(dblock_index_t));
                    dblock_index_holder = temp_block;
                    printf("\n222the temp block index is: %d\n", dblock_index_holder);

                }

                if(i == 0){
                    if(starting_byte_within_starting_block == 0){
                        printf("\nReleasing %d\n", dblock_index_holder);
                        release_dblock(fs, &fs->dblocks[dblock_index_holder * DATA_BLOCK_SIZE]);

                    }
                }
                else{
                    printf("\n222Releasing %d\n", dblock_index_holder);

                    release_dblock(fs, &fs->dblocks[dblock_index_holder * DATA_BLOCK_SIZE]);
                    
                }

                if(i + 1 == neces_dblocks_for_removal){
                    if(indirect_block_holder != starting_index_holder){
                        printf("\n222Releasing indirect block %d\n", indirect_block_holder);
                        release_dblock(fs, &fs->dblocks[indirect_block_holder * DATA_BLOCK_SIZE]);
                        
                    }
                }

            }

            inode ->internal.file_size = new_size;

        }


     }


    display_filesystem(fs, 3);
    return SUCCESS;
    
    //check to see if inputs are in valid range

    //Calculate how many blocks to remove

    //helper function to free all indirect blocks

    //remove the remaining direct dblocks

    //update filesize and return
}

// make new_size to 0
fs_retcode_t inode_release_data(filesystem_t *fs, inode_t *inode)
{
    if(fs == NULL || inode == NULL){
        return INVALID_INPUT;
    }

    //printf("\nTHE SIZE OF THE INODE IS %lu\n", inode->internal.file_size);
    size_t total_dblock = calculate_necessary_dblock_amount(inode->internal.file_size);
    size_t index_dblock_amount = calculate_index_dblock_amount(inode->internal.file_size);
    size_t direct_dblock_amount = total_dblock - index_dblock_amount;
    //printf("\nTHE TOTAL DBLOCKS TO RELEASE IS %lu   THE INDEX DBLOCK AMOUNT IS %lu  AND THE DIRECT DBLOCK AMOUNT IS %lu\n", total_dblock, index_dblock_amount, direct_dblock_amount);

    // printf("\nTHE INODE DIRECT DATA BEFORE DEALLOCATING IS ");
    // for(int i = 0; i < INODE_DIRECT_BLOCK_COUNT; i++){
    //     printf("%d, ", inode->internal.direct_data[i]);
    // }
    // printf("\n");

    for(int i = 0; i < INODE_DIRECT_BLOCK_COUNT; i++){
        //printf("\nTHERE ARE %lu DBLOCKS LEFT", direct_dblock_amount);
        dblock_index_t byte_start = (inode->internal.direct_data[i]) * DATA_BLOCK_SIZE;
        //printf("\nRELEASING THE DLOCK %d\n", inode->internal.direct_data[i]);
        release_dblock(fs, &fs->dblocks[byte_start]);
        direct_dblock_amount--;
        if(direct_dblock_amount == 0){
            inode->internal.file_size = 0;
            // printf("\nTHE INODE DIRECT DATA AFTER DEALLOCATING IS ");
            // for(int i = 0; i < INODE_DIRECT_BLOCK_COUNT; i++){
            //     printf("%d, ", inode->internal.direct_data[i]);
            // }
            return SUCCESS; 
        }
    }

    // printf("\nTHE INODE DIRECT DATA AFTER DEALLOCATING IS ");
    // for(int i = 0; i < INODE_DIRECT_BLOCK_COUNT; i++){
    //     printf("%d, ", inode->internal.direct_data[i]);
    // }

    // printf("\n");

    // printf("\nTHERE ARE %lu DBLOCKS LEFT", direct_dblock_amount);

    // printf("\nTHE FIRST INDEX DLBOCK SHOULD BE %d\n", inode->internal.indirect_dblock);
    dblock_index_t indirect_dblock_bytestart = (inode->internal.indirect_dblock) * DATA_BLOCK_SIZE;
    // printf("\nTHE INDIRECT BYTE START IS %d AND THE INDIRECT DBLOCK INDEX SHOULD BE %d\n", indirect_dblock_bytestart, inode->internal.indirect_dblock);
    size_t temp_offset = 0;
    dblock_index_t dblock_index_holder = 0;

    


    for(size_t i = 0; i < index_dblock_amount; i++){        
        temp_offset = 0;
        //printf("\nRELEASING INDEX DBLOCK NUMBER %lu\n", i + 1);
        for(size_t x = 0; x < 15; x++){
            //printf("\nI AM RELEASING THE %lu DBLOCK FROM INDEX DBLOCK %lu",x + 1, i + 1);

            memcpy(&dblock_index_holder, &fs->dblocks[indirect_dblock_bytestart + temp_offset], sizeof(dblock_index_t));
            //printf("\nTHE DBLOCK INDEX BEING RELASED IS %d\n", dblock_index_holder); 
            release_dblock(fs, &fs->dblocks[dblock_index_holder * DATA_BLOCK_SIZE]);
            direct_dblock_amount--;
            //printf("\nTHERE ARE %lu DBLOCKS LEFT", direct_dblock_amount);
            if(direct_dblock_amount == 0){
                release_dblock(fs, &fs->dblocks[indirect_dblock_bytestart]);
                inode->internal.file_size = 0;
                return SUCCESS; 
            }


            temp_offset += 4;
            //printf("\nTHE TEMP OFFSET IS NOW %lu\n", temp_offset);

        }
        dblock_index_t temp_byte_holder = indirect_dblock_bytestart;
        memcpy(&indirect_dblock_bytestart, &fs->dblocks[indirect_dblock_bytestart + temp_offset], sizeof(dblock_index_t));
        indirect_dblock_bytestart *= DATA_BLOCK_SIZE;
        
        //printf("\nFREEING THE INDEX BLOCK HOLDING THE VALUE OF %d AND HOLDING VALUE OF %d\n", inode->internal.indirect_dblock, temp_byte_holder/DATA_BLOCK_SIZE);
        release_dblock(fs, &fs->dblocks[temp_byte_holder]);
        

        //printf("\nTHE DLBOCK THAT THE 16TH DBLOCK POINTS TO IS %d\n", indirect_dblock_bytestart/DATA_BLOCK_SIZE);
    }

    return SUCCESS;
    //shrink to size 0
}
