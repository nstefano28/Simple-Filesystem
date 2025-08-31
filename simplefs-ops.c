#include "simplefs-ops.h"
extern struct filehandle_t file_handle_array[MAX_OPEN_FILES]; // Array for storing opened files

int simplefs_create(char *filename){
    /*
	    Create file with name `filename` from disk
	*/

	if (strlen(filename) > MAX_NAME_STRLEN) {
        return -1;
    }

    // Check if file already exists
    struct inode_t inode;
    for (int i = 0; i < NUM_INODES; i++) {
        simplefs_readInode(i, &inode);
        if (inode.status == INODE_IN_USE && strcmp(inode.name, filename) == 0) {
            // File already exists
            return -1;
        }
    }

	int inode_index = simplefs_allocInode();
    // No free inodes available
    if (inode_index == -1) {
        return -1;
    }

	simplefs_readInode(inode_index, &inode);
    inode.status = INODE_IN_USE;
    strncpy(inode.name, filename, MAX_NAME_STRLEN);
    inode.file_size = 0;
    for (int i = 0; i < MAX_FILE_SIZE; i++) {
        inode.direct_blocks[i] = -1;
    }

    // Write the inode back to disk
    simplefs_writeInode(inode_index, &inode);

    return inode_index;

}


void simplefs_delete(char *filename){
    /*
	    delete file with name `filename` from disk
	*/

	struct inode_t inode;
    int inode_index = -1;

    // Find the inode with the given filename
    for (int i = 0; i < NUM_INODES; i++) {
        simplefs_readInode(i, &inode);
        if (inode.status == INODE_IN_USE && strcmp(inode.name, filename) == 0) {
            inode_index = i;
            break;
        }
    }

    if (inode_index == -1) {
        // File not found
        return;
    }

    // Free all data blocks used by the file
    for (int i = 0; i < MAX_FILE_SIZE; i++) {
        if (inode.direct_blocks[i] != -1) {
            simplefs_freeDataBlock(inode.direct_blocks[i]);
        }
    }

    // Free the inode
    simplefs_freeInode(inode_index);
}

int simplefs_open(char *filename){
    /*
	    open file with name `filename`
	*/
    struct inode_t inode;
    int inode_index = -1;

    // Find the inode with the given filename
    for (int i = 0; i < NUM_INODES; i++) {
        simplefs_readInode(i, &inode);
        if (inode.status == INODE_IN_USE && strcmp(inode.name, filename) == 0) {
            inode_index = i;
            break;
        }
    }

    if (inode_index == -1) {
        // File not found
        return -1;
    }

    // Find an unused file handle
    int file_handle = -1;
    for (int i = 0; i < MAX_OPEN_FILES; i++) {
        if (file_handle_array[i].inode_number == -1) {
            file_handle = i;
            break;
        }
    }

    if (file_handle == -1) {
        // No free file handles
        return -1;
    }

    // Initialize the file handle
    file_handle_array[file_handle].inode_number = inode_index;
    file_handle_array[file_handle].offset = 0;

    return file_handle;
}

void simplefs_close(int file_handle){
    /*
	    close file pointed by `file_handle`
	*/
	if (file_handle < 0 || file_handle >= MAX_OPEN_FILES) {
        return;
    }

    // Reset the file handle
    file_handle_array[file_handle].inode_number = -1;
    file_handle_array[file_handle].offset = 0;

}

int simplefs_read(int file_handle, char *buf, int nbytes){
    /*
	    read `nbytes` of data into `buf` from file pointed by `file_handle` starting at current offset
	*/
    if (file_handle < 0 || file_handle >= MAX_OPEN_FILES) {
        return -1;
    }

    int inode_index = file_handle_array[file_handle].inode_number;
    if (inode_index == -1) {
        // File not open
        return -1;
    }

    int offset = file_handle_array[file_handle].offset;
    struct inode_t inode;
    simplefs_readInode(inode_index, &inode);

    // Check if read would go beyond the file size
    if (offset + nbytes > inode.file_size) {
        return -1;
    }

    // Calculate starting block and offset within that block
    int start_block = offset / BLOCKSIZE;
    int start_offset = offset % BLOCKSIZE;
    int bytes_read = 0;
    char block_buf[BLOCKSIZE];

    // Read data across multiple blocks if necessary
    while (bytes_read < nbytes) {
        int current_block = start_block + (start_offset + bytes_read) / BLOCKSIZE;
        int current_offset = (start_offset + bytes_read) % BLOCKSIZE;
        int bytes_to_read = BLOCKSIZE - current_offset;
        
        if (bytes_to_read > nbytes - bytes_read) {
            bytes_to_read = nbytes - bytes_read;
        }

        // Check if the block is allocated
        if (current_block >= MAX_FILE_SIZE || inode.direct_blocks[current_block] == -1) {
            return -1;
        }

        // Read the block
        simplefs_readDataBlock(inode.direct_blocks[current_block], block_buf);
        
        // Copy data from the block to the buffer
        memcpy(buf + bytes_read, block_buf + current_offset, bytes_to_read);
        bytes_read += bytes_to_read;
    }

    return 0;
}


int simplefs_write(int file_handle, char *buf, int nbytes){
    /*
	    write `nbytes` of data from `buf` to file pointed by `file_handle` starting at current offset
	*/
    if (file_handle < 0 || file_handle >= MAX_OPEN_FILES) {
        return -1;
    }

    int inode_index = file_handle_array[file_handle].inode_number;
    if (inode_index == -1) {
        // File not open
        return -1;
    }

    int offset = file_handle_array[file_handle].offset;
    struct inode_t inode;
    simplefs_readInode(inode_index, &inode);

    // Check if write would exceed maximum file size
    if (offset + nbytes > MAX_FILE_SIZE * BLOCKSIZE) {
        return -1;
    }

    // Calculate starting block and offset within that block
    int start_block = offset / BLOCKSIZE; // Integer division truncates
    int start_offset = offset % BLOCKSIZE;
    int bytes_written = 0;
    char block_buf[BLOCKSIZE];
    
    // Track newly allocated blocks to free them in case of failure
    int new_blocks[MAX_FILE_SIZE];
    int num_new_blocks = 0;

    // Write data across multiple blocks if necessary
    while (bytes_written < nbytes) {

        // Calculate the current block and the offset within the block
        int current_block = start_block + (start_offset + bytes_written) / BLOCKSIZE;
        int current_offset = (start_offset + bytes_written) % BLOCKSIZE;
 
        // Determine how many bytes to write in the current block
        int bytes_to_write = BLOCKSIZE - current_offset;
        
        // If there are more bytes remaining than the space in the block, adjust the write size
        if (bytes_to_write > nbytes - bytes_written) {
            bytes_to_write = nbytes - bytes_written;
        }

        // Allocate block if necessary
        if (inode.direct_blocks[current_block] == -1) {
            int block_num = simplefs_allocDataBlock();
            if (block_num == -1) {
                // No free data blocks
                // Free all newly allocated blocks
                for (int i = 0; i < num_new_blocks; i++) {
                    simplefs_freeDataBlock(new_blocks[i]);
                }
                return -1;
            }

            // Increment number of new blocks and track
            new_blocks[num_new_blocks] = block_num;
            num_new_blocks++;
            inode.direct_blocks[current_block] = block_num;
            
            
            // Initialize the block with zeroes
            memset(block_buf, 0, BLOCKSIZE);
        } else {
            // Read existing block
            simplefs_readDataBlock(inode.direct_blocks[current_block], block_buf);
        }
        
        // Copy data from the buffer to the block
        memcpy(block_buf + current_offset, buf + bytes_written, bytes_to_write);
        
        // Write the block back to disk
        simplefs_writeDataBlock(inode.direct_blocks[current_block], block_buf);
        
        bytes_written += bytes_to_write;
    }

    // Update file size if necessary
    if (offset + nbytes > inode.file_size) {
        inode.file_size = offset + nbytes;
    }

    // Write the updated inode back to disk
    simplefs_writeInode(inode_index, &inode);

    return 0;
}


int simplefs_seek(int file_handle, int nseek){
    /*
	   increase `file_handle` offset by `nseek`
	*/
    if (file_handle < 0 || file_handle >= MAX_OPEN_FILES) {
        return -1;
    }

    int inode_index = file_handle_array[file_handle].inode_number;
    if (inode_index == -1) {
        // File not open
        return -1;
    }

    struct inode_t inode;
    simplefs_readInode(inode_index, &inode);

    // Calculate new offset
    int new_offset = file_handle_array[file_handle].offset + nseek;
    
    // Ensure new offset is within file boundaries
    if (new_offset < 0 || new_offset > inode.file_size) {
        return -1;
    }

    // Update offset
    file_handle_array[file_handle].offset = new_offset;
    
    return 0;
}