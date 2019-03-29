// FS Assignment, CSC 360
// Andrew Wiggins, V00817291

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "FS.h"
#include "Controller.h"

/****************************************/
// Open/Close disk

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
		// Successful close
	} else {
		fprintf(stderr, "File system could not be closed!\n");
	}
}

/****************************************/
// TODO: File System functions (file create, etc.)

/****************************************/
// Init functions

int init_root()
{
	int block_offset, dir_offset;

	// Get superblock
	superblock *sb = get_superblock();

	// Get inode map to check for availability
	int *map = get_inode_map();

	if (TestBit(map, sb->first_free_inode) == 1) {
		// First inode is available
		// Set it to 0 to mark as used
		ClearBit(map, sb->first_free_inode);
	} else {
		printf("Root inode is unavailable!\n");
		free(sb);
		free(map);
		return 1;
	}

	// Write inode map back to disk
	if (write_inode_map(map) != 0) {
		printf("Problems writing inode map to disk!\n");
		return 1;
	}

	// Make root inode
	inode *root_inode = get_inode(sb->first_free_inode);

	// Updat inode for root dir
	root_inode->inode_num = sb->first_free_inode;
	root_inode->directory_flag = 1;
	root_inode->filesize = 1000;
	root_inode->block_pointers[0] = 123;

	// Write root node inode back to disk
	if (write_inode(root_inode) != 0) {
		printf("Problems writing inode to disk!\n");
		return 1;
	}

	// Check for first available block
	int *fb = get_free_blocks();

	if (TestBit(fb, sb->first_free_block) == 1) {
		ClearBit(fb, sb->first_free_block);
	} else {
		printf("First free block not available, check superblock!\n");
		free(fb);
		return 1;
	}

	// Write free blocks back to disk
	if (write_free_blocks(fb) != 0) {
		printf ("more bad\n");
		return 1;
	}

	// Create block for root directory
	dir_block *root_block = (dir_block *)malloc(BLOCK_SIZE);
	root_block->next_slot = 123;

	// Store first dir entry as itself
	direntry *root_entry = (direntry *)malloc(sizeof(direntry));
	root_entry->inode_num = sb->first_free_inode;
	strcpy(root_entry->name, (char*)"Hello");

	// Write root block (512B) to disk (empty)
	block_offset = (sb->first_free_block - 1) * BLOCK_SIZE;

	if (block_write(root_block, block_offset, BLOCK_SIZE) != 0) {
		printf("Problems writing root block to disk!\n");
		return 1;
	}

	// Write root direntry at start of root block
	dir_offset = ((root_block->next_slot) * DIRENTRY_SIZE);

	printf("TEST ONLY: %d\n", dir_offset);

	// Update superblock
	sb->first_free_inode++;
	sb->first_free_block++;
	sb->used_blocks++;
	

	if (write_superblock(sb) != 0) {
		printf("Problems writing superblock to disk!\n");
		free(sb);
		return 1;
	}

	// Clean up
	free(root_entry);
	free(root_block);

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
