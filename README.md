# SimpleFS

## Simple Filesystem Interface Development

This project will follow the IIT [lab document on filesystems](https://www.cse.iitb.ac.in/~mythili/os/labs/lab-simplefs/simplefs.pdf).

## File Operations Implementation

### simplefs_create(char *filename)

This function creates a new file with the specified name:

1. Validates the filename length (≤ 8 characters)
2. Checks if a file with the same name already exists
3. Allocates a new inode using `simplefs_allocInode()`
4. Initializes the inode with the filename, zero file size, and empty direct block pointers
5. Returns the inode index on success, -1 on failure

### simplefs_delete(char *filename)

This function removes a file from the file system:

1. Searches for the inode with the matching filename
2. Frees all allocated data blocks by calling `simplefs_freeDataBlock()` for each direct block
3. Frees the inode by calling `simplefs_freeInode()`

### simplefs_open(char *filename)

This function opens a file and returns a file handle:

1. Finds the inode with the matching filename
2. Allocates a free file handle from the `file_handle_array`
3. Initializes the file handle with the inode number and zero offset
4. Returns the file handle index on success, -1 on failure

### simplefs_close(int file_handle)

This function closes an open file:

1. Validates the file handle
2. Resets the file handle's inode number and offset to indicate it's free

### simplefs_read(int file_handle, char *buf, int nbytes)

This function reads data from a file:

1. Validates the file handle and checks if the file is open
2. Calculates the starting block and offset within that block
3. Ensures the read operation won't go beyond the file size
4. Reads data across multiple blocks if necessary, copying into the buffer
5. Returns 0 on success, -1 on failure

### simplefs_write(int file_handle, char *buf, int nbytes)

This function writes data to a file:

1. Validates the file handle and checks if the file is open
2. Calculates the starting block and offset within that block
3. Ensures the write operation won't exceed maximum file size
4. For each block needed:
   - Allocates a new data block if necessary
   - Reads existing data block if it's already allocated
   - Copies data from buffer to the block
   - Writes the block back to disk
5. Updates the file size if necessary
6. Returns 0 on success, -1 on failure

### simplefs_seek(int file_handle, int nseek)

This function adjusts the current offset in an open file:

1. Validates the file handle and checks if the file is open
2. Calculates the new offset
3. Ensures the new offset is within file boundaries
4. Updates the offset in the file handle
5. Returns 0 on success, -1 on failure

## Usage

1. Build testcases:
   ```bash
   make
   ```
2. Run a specific testcase:
   ```bash
   ./testcase0
   ```
3. Clean up files
   ```bash
   make clean
   ```
