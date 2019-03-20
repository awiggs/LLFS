// LLFS Assignment, CSC 360
// Andrew Wiggins, V00817291

#include <stdio.h>
#include <stdlib.h>
#include "LLFS.h"

/*****************************************************/

// Functions

int open_llfs(char *llfs_path)
{
	// Open vdisk
	vdisk = fopen(llfs_path, "w+");

	if (vdisk == NULL) {
		fprintf(stderr, "Error opening file system!\n");
		return 1;
	}

	return 0;

}

void close_llfs(void)
{
	// Close vdisk
	int retval = fclose(vdisk);

	if (retval == 0) {
		printf("File system closed successfully...\n");
	} else {
		fprintf(stderr, "File system could not be closed!\n");
	}
}

int init_llfs(char *llfs_path, int num_blocks)
{
	printf("Formatting disk...\n");
	superblock *sb;

	// Open disk
	vdisk = fopen(llfs_path, "w+");

	if (vdisk == NULL) {
		fprintf(stderr, "Error opening initial vdisk!\n");
		return 1;
	}

	// Create Superblock
	sb = (superblock *)malloc(sizeof(superblock));
	sb->magic = MAGIC_NUM;
	sb->block_count = num_blocks;
//	sb->max_files = MAX_FILES;
	sb->current_files = 0;

	// Create block vector mapping
	// Helper files found in separate header
	int free_blocks[128]; // 512 bytes, int = 4 bytes, a[i] = 32 bit flags	

	// Create inodes
		

	// Create empty data blocks

	fwrite(sb, 1, sizeof(superblock), vdisk);


	// Clean up
	free(sb);
	fclose(vdisk);
	printf("Formatted disk. Exiting.\n");
	return 0;
}

/*****************************************************/

int test_llfs()
{
	// Use this for testing the file system
	return 0;
}

/*****************************************************/

int main()
{
	char *LLFS_PATH = VDISK_PATH;

	// Initialize vdisk
	int start = init_llfs(LLFS_PATH, NUM_BLOCKS);

	if (start == 1) {
		fprintf(stderr, "Error starting file system!\n");
	}

	// Open vdisk for use
//	open_llfs(LLFS_PATH);

	// Do stuff
//	test_llfs();

	// Close vdisk
//	close_llfs();

	return 0;
}
