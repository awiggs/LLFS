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

superblock* get_superblock()
{
	int block_offset;
	superblock* sb;

	// Set up superblock struct
	sb = (superblock *)malloc(BLOCK_SIZE);

	// Open the disk
	if (open_fs(VDISK_PATH) != 0) {
		return NULL;
	}

	// Superblock is always the first block
	block_offset = 0;

	// Move cursor to top of superblock
	fseek(vdisk, block_offset, SEEK_SET);

	// Read superblock from disk
	fread(sb, BLOCK_SIZE, 1, vdisk);

	// Close the disk
	close_fs();

	return sb;
}

int write_superblock(superblock* sb)
{
	int block_offset;

	// Open the disk
	if (open_fs(VDISK_PATH) != 0) {
		return 1;
	}

	// Superblock is always the first block
	block_offset = 0;

	// Move cursor to top of superblock
	fseek(vdisk, block_offset, SEEK_SET);

	// Write superblock to disk
	fwrite(sb, BLOCK_SIZE, 1, vdisk);

	// Clean up
	free(sb);
	close_fs();

	return 0;
}

int* get_free_blocks()
{
	int block_offset;

	// Set up array for 128 ints (512 bytes);
	int* free_blocks = malloc(MAP_ENTRIES);

	if (free_blocks == NULL) {
		printf("Malloc failed for free block vector!\n");
		return NULL;
	}

	// Open the disk
	if (open_fs(VDISK_PATH) != 0) {
		return NULL;
	}

	// Free block vector is always the second block
	block_offset = BLOCK_SIZE;

	// Move cursor to top of free block vector block
	fseek(vdisk, block_offset, SEEK_SET);

	// Read vector from disk
	fread(free_blocks, sizeof(free_blocks), 1, vdisk);

	// Close the disk
	close_fs();

	return free_blocks;
}

int write_free_blocks(int* free_blocks)
{
	int block_offset;

	// Open disk
	if (open_fs(VDISK_PATH) != 0) {
		return 1;
	}

	// Free block vector is always the second block
	block_offset = BLOCK_SIZE;

	// Move cursor to top of vector block
	fseek(vdisk, block_offset, SEEK_SET);

	// Write vector to disk
	fwrite(free_blocks, sizeof(free_blocks), 1, vdisk);

	// Clean up
	free(free_blocks);
	close_fs();

	return 0;
}

int* get_inode_map()
{
	int block_offset;

	// Set up array for 128 ints (512 bytes)
	int* map = malloc(MAP_ENTRIES);

	if (map == NULL) {
		printf("Malloc failed for get inode map!\n");
		return NULL;
	}

	// Open the disk
	if (open_fs(VDISK_PATH) != 0) {
		return NULL;
	}

	// inode map is always the third block
	block_offset = 2 * BLOCK_SIZE;
	
	// Move cursor to top of map block
	fseek(vdisk, block_offset, SEEK_SET);

	// Read map from disk
	fread(map, sizeof(map), 1, vdisk);

	// Close the disk
	close_fs();

	return map;
}

int write_inode_map(int* map)
{
	int block_offset;

	// Open disk
	if (open_fs(VDISK_PATH) != 0) {
		return 1;
	}

	// inode map is always the third block
	block_offset = 2 * BLOCK_SIZE;

	// Move cursor to top of map block
	fseek(vdisk, block_offset, SEEK_SET);

	// Write map to disk
	fwrite(map, sizeof(map), 1, vdisk);

	// Clean up
	free(map);
	close_fs();

	return 0;
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

	if (ret == NULL) {
		printf("Malloc for getting inode failed!\n");
		return NULL;
	}

	// Grab bytes from disk
	memcpy(ret, &buffer[inode_offset * INODE_SIZE], INODE_SIZE);	

	// Close the disk
	close_fs();

	printf("Returned inode number: %d\n", ret->inode_num);
	
	return ret;
}

int write_inode(inode* node)
{
	int inode_num, block_number, block_offset, inode_offset;

	// Get inode num to write back to disk
	inode_num = node->inode_num;

	// Open the disk
	if (open_fs(VDISK_PATH) != 0) {
		return 1;
	}

	// Get block number to write inode back to
	block_number = 3 + (inode_num / (BLOCK_SIZE / INODE_SIZE));
	block_offset = (block_number * BLOCK_SIZE);

	// Move cursor to top of inode block
	fseek(vdisk, block_offset, SEEK_SET);

	// Calculate inode offset
	inode_offset = inode_num % (BLOCK_SIZE / INODE_SIZE);

	// Move cursor to top of inode
	fseek(vdisk, (inode_offset * INODE_SIZE), SEEK_CUR);

	// Write inode into disk
	fwrite(node, 1, INODE_SIZE, vdisk);

	// Clean up
	free(node);
	close_fs();

	return 0;
}

int init_root()
{
	// Get root inode
	inode *root_inode = get_inode(0);

	// Updat inode for root dir
	root_inode->filesize = 1;
	root_inode->directory_flag = 1;

	// Write root node inode back to disk
	if (write_inode(root_inode) == 1) {
		printf("Problems writing inode to disk!\n");
		return 1;
	}

	// Get inode map to check for availability
	int *map = get_inode_map();

	if (TestBit(map, 0) == 1) {
		// First inode is available
		// Set it to 0 to mark as used
		ClearBit(map, 0);
	} else {
		printf("Root inode is unavailable!\n");
		return 1;
	}

	// Write inode map back to disk
	if (write_inode_map(map) != 0) {
		printf("Problems writing inode map to disk!\n");
		return 1;
	}

	/* START TEST CODE */

	// TESTING FREEBLOCKS
	int *fb = get_free_blocks();

	if (TestBit(fb, 11) == 1) {
		ClearBit(fb, 11);
	} else {
		printf("Bad\n");
		return 1;
	}

	if (write_free_blocks(fb) != 0) {
		printf ("more bad\n");
		return 1;
	}

	// TESTING SUPERBLOCK
	superblock *sb = get_superblock();

	sb->first_free_block++;

	if (write_superblock(sb) != 0) {
		printf("sb bad\n");
		return 1;
	}

	/* END TEST CODE */

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

	if (sb == NULL) {
		printf("Malloc for superblock failed!\n");
		return 1;
	}

	sb->magic = MAGIC_NUM;
	sb->block_count = num_blocks;
	sb->max_files = MAX_FILES;
	sb->first_free_inode = 0;
	sb->first_free_block = 11;
	sb->current_files = 0;

	// Create block vector mapping
	// Helper files found in separate header
	int free_blocks[MAP_ENTRIES]; // 512 bytes, int = 4 bytes, a[i] = 32 bit flags
	int inode_map[MAP_ENTRIES];

	// Fill free blocks
	// 0 = unavailable, 1 = available
	int i;
	for (i = 0; i < NUM_BLOCKS; i++) {
		// Fill inode map with 1s
		if (i < MAX_FILES) {
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
	for (j = 0; j < MAX_FILES; j++) {
		// Create inode block
		inode *node = (inode *)malloc(INODE_SIZE);

		if (node == NULL) {
			printf("Malloc for inode failed!\n");
			return 1;
		}	

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
