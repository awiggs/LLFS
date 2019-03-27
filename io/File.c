// FS Assignment, CSC 360
// Andrew Wiggins, V00817291

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "FS.h"

/*****************************************************/

// Functions

int open_fs(char *fs_path)
{
	// Open vdisk
	vdisk = fopen(fs_path, "r+");

	if (vdisk == NULL) {
		fprintf(stderr, "Error opening file system!\n");
		return 1;
	}

	return 0;

}

void close_fs(void)
{
	// Close vdisk
	int retval = fclose(vdisk);

	if (retval == 0) {
		printf("File system closed successfully...\n");
	} else {
		fprintf(stderr, "File system could not be closed!\n");
	}
}

inode* get_inode(int inode_num)
{
	inode* ret;
	int block_number, block_offset, inode_offset;
	char buffer[BLOCK_SIZE];

	// Ensure requested inode number is valid
	if (inode_num >= MAX_FILES) {
		printf("Requested invalid inode -- returning NULL\n");
		return NULL;
	}

	// Open the disk
	if (open_fs(VDISK_PATH) != 0) {
		return NULL;
	}

	// Get block number that contains desired inode
	block_number = 3 + (inode_num / (BLOCK_SIZE / INODE_SIZE));
	block_offset = (block_number * BLOCK_SIZE); // + ((block_number - 1) * BLOCK_SIZE);
	
	// Move cursor to top of inode block
	fseek(vdisk, block_offset, SEEK_SET);

	// Read block containing inode into buffer
	fread(buffer, BLOCK_SIZE, 1, vdisk);

	// Calculate inode offset
	inode_offset = inode_num % (BLOCK_SIZE / INODE_SIZE);

	// Create a return inode
	ret = (inode *)malloc(INODE_SIZE);

	// Grab bytes from disk
	memcpy(ret, &buffer[inode_offset * INODE_SIZE], INODE_SIZE);	

	// Close the disk
	close_fs();

	printf("Returned inode number: %d\n", ret->inode_num);
	
	return ret;
}

int write_inode(inode* node)
{
	int inode_num, ;

	// Get inode num to write back to disk
	inode_num = node->inode_num;

	// Open the disk
	if (open_fs(VDISK_PATH) != 0) {
		return NULL;
	}

	// Get block number to write inode back to
	block_number


	return 0;
}

int write_fs()
{
	return 0;
}

int read_fs()
{
	return 0;
}

int init_root()
{
	inode *root_inode = get_inode(0);
	root_inode->filesize = 1;
	root_inode->directory_flag = 1;

	return 0;
}

int init_fs(char *fs_path, int num_blocks)
{
	printf("Formatting disk...\n");
	superblock *sb;

	// Open disk
	vdisk = fopen(fs_path, "wb+");

	if (vdisk == NULL) {
		fprintf(stderr, "Error opening initial vdisk!\n");
		return 1;
	}

	// Create Superblock
	sb = (superblock *)malloc(sizeof(superblock));
	sb->magic = MAGIC_NUM;
	sb->block_count = num_blocks;
	sb->max_files = MAX_FILES;
	sb->first_free_inode = 0;
	sb->first_free_block = 11;
	sb->current_files = 0;

	// Create block vector mapping
	// Helper files found in separate header
	int free_blocks[128]; // 512 bytes, int = 4 bytes, a[i] = 32 bit flags
	int inode_map[128];

	// Fill free blocks
	// 0 = unavailable, 1 = available
	int i;
	for (i = 0; i < 4096; i++) {
		// Fill inode map with 1s
		if (i < 112) {
			SetBit(inode_map, i);
		} else {
			ClearBit(inode_map, i);
		}

		// First 10 blocks aren't available
		if (i < 10) {
			ClearBit(free_blocks, i);
		} else {
			SetBit(free_blocks, i);
		}
	}

	// Write the superblock and free block map to disk
	fwrite(sb, 1, sizeof(superblock), vdisk);
	fwrite(free_blocks, 1, sizeof(free_blocks), vdisk);
	fwrite(inode_map, 1, sizeof(inode_map), vdisk);

	// Create inode blocks with inodes
	int j;
	for (j = 0; j < 112; j++) {
		// Create inode block
		inode* node = (inode *)malloc(INODE_SIZE);

		// Fill inode with default info
		node->inode_num = j;
		node->filesize = 0;
		node->directory_flag = 0;
		
		// Write inode block to disk
		fwrite(node, 1, INODE_SIZE, vdisk);

		// Free inode memory
		free(node);
	}

	// Clean up
	free(sb);
	fclose(vdisk);
	printf("Formatted disk. Exiting.\n");
	return 0;
}

/*****************************************************/

int test_fs()
{
	// Use this for testing the file system
	return 0;
}

/*****************************************************/

int main()
{
	char *FS_PATH = VDISK_PATH;

	// Initialize vdisk
	int start = init_fs(FS_PATH, NUM_BLOCKS);

	if (start == 1) {
		fprintf(stderr, "Error starting file system!\n");
	}

	init_root();

	// Open vdisk for use
//	open_fs(FS_PATH);

	// Do stuff
//	test_fs();

	// Close vdisk
//	close_fs();

	return 0;
}
