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
int mkdir(char* path)
{
	int i, token_counter, entry_offset, block_number, block_offset, root_offset;
	const char d[2] = "/";
	char *copy;
	char *token;
	inode *root_inode, *new_inode;
	superblock *sb;
	block *root_block, *new_block;
	direntry *new_entry;

	// Parse path to get tokens
	copy = strdup(path);
	token = strtok(copy, d);
	token_counter = 0;

	// Get number of tokens
	while (token != NULL) {
		token_counter++;
		token = strtok(NULL, d);
	}

	// Store tokens into an array
	char *tokens[token_counter];
	char *copy2 = strdup(path);
	i = 0;
	tokens[i] = strtok(copy2, d);
	while (tokens[i] != NULL) {
		tokens[++i] = strtok(NULL, d);
	}

	// Get inode of parent dir
	int parent = path_to_inode(path);

	// Make sure we got the right inode
	if (parent < 0) {
		return 1;
	}

	// Get root inode
	root_inode = get_inode(parent);		

	// Get root dir block
	root_block = (block *)block_read((root_inode->block_pointers[0] - 1) * BLOCK_SIZE);

	if (root_block == NULL) {
		printf("Problems reading root dir block!\n");
		free(root_inode);
		return 1;
	}

	// Get Superblock
	sb = get_superblock();

	// Create new dir block
	new_block = (block *)malloc(sizeof(block));

	if (new_block == NULL) {
		printf("Problems with new block malloc!\n");
		free(root_inode);
		free(root_block);
		free(sb);
		return 1;
	}

	memset(new_block->data, 0, sizeof(new_block->data));
	block_number = sb->first_free_block;

	// Write new block to disk
	block_offset = (block_number - 1) * BLOCK_SIZE;
	if (block_write(new_block, block_offset, BLOCK_SIZE) != 0) {
		printf("Problems writing new block to disk!\n");
		free(new_block);
		free(root_block);
		free(root_inode);
		free(sb);
		return 1;
	}
		
	free(new_block);

	// Create new inode + init
	new_inode = get_inode(sb->first_free_inode);
	new_inode->directory_flag = 1;
	new_inode->filesize = 0;
	memset(new_inode->block_pointers, 0, sizeof(new_inode->block_pointers));
	new_inode->block_pointers[0] = block_number;

	// Write new inode back to disk
	if (write_inode(new_inode) != 0) {
		printf("Problems writing new inode to disk!\n");
		free(new_inode);
		free(root_block);
		free(root_inode);
		free(sb);
		return 1;
	}

	// Add entry to root dir block
	new_entry = (direntry *)malloc(sizeof(direntry));

	if (new_entry == NULL) {
		printf("Problems with new entry malloc!\n");
		free(new_inode);
		free(root_block);
		free(root_inode);
		free(sb);
		return 1;
	}

	new_entry->inode_num = sb->first_free_inode;
	memset(new_entry->name, 0, sizeof(new_entry->name));
	strcpy(new_entry->name, tokens[token_counter - 1]);

	entry_offset = root_inode->filesize / 16;
	memcpy(&root_block->data[entry_offset * 16], new_entry, sizeof(direntry));

	root_offset = (root_inode->block_pointers[0] - 1) * BLOCK_SIZE;

	// Write root block back to disk
	if (block_write(root_block, root_offset, BLOCK_SIZE) != 0) {
		printf("Problem writing root block back to disk!\n");
		free(root_block);
		free(root_inode);
		free(sb);
		return 1;
	}

	free(root_block);

	// Update + write root inode
	root_inode->filesize += 16;
	if (write_inode(root_inode) != 0) {
		// Problems
		free(sb);
		free(root_inode);
		return 1;
	}
	
	// Update inode map
	if (set_inode_map(sb->first_free_inode) != 0) {
		// Problems
		free(sb);
		return 1;
	}

	// Update block vector
	if (set_block_vector(sb->first_free_block) != 0) {
		// Problems
		free(sb);
		return 1;
	}
		
	// Update Superblock
	sb->first_free_inode++;
	sb->first_free_block++;
	sb->used_blocks++;

	// Write Superblock
	if (write_superblock(sb) != 0) {
		// Problems
		free(sb);
		return 1;
	}

	printf("Directory \"%s\" added!\n", tokens[token_counter - 1]);

	return 0;
}

int create_file(char* path)
{
	/*
	* Essentially a 'touch' function currently
	* Creates a new empty file (in the ROOT dir only currently)
	*/

	int i, j, path_length, slash_index, inode_num, block_number, map_update;
	int parent_dir_block, block_offset, parent_offset, dirents;
	char *new_path;
	char filename[12];
	inode *path_node;
	inode *new_inode;
	superblock *sb;
	block *new_block;
	block *parent_block;
	direntry *parent_entry;
	
	// Check for no parameter passed
	if (!path) {
		printf("No path passed to create file!\n");
		return 1;
	}

	// Get file name from end of path
	path_length = strlen(path);
	for (i = 0; i < path_length; i++) {
		if (path[i] == '/') {
			slash_index = i;
		}
	}
	new_path = strdup(path);
	new_path[slash_index] = '\0';
	memcpy(filename, &new_path[slash_index + 1], 12);

	// Get inode for path
	inode_num = path_to_inode(new_path);
	path_node = get_inode(inode_num);

	// Update path node
	path_node->filesize += 16;

	// Get next available inode for the file
	sb = get_superblock();
	new_inode = get_inode(sb->first_free_inode);
	new_inode->directory_flag = 0;

	// Get block to use for the file
	new_block = (block *)malloc(sizeof(block));
	memset(new_block->data, 0, sizeof(new_block->data));
	block_number = sb->first_free_block;	

	// TODO: write contents to the new block

	// Write new block back to disk
	block_offset = (block_number - 1) * BLOCK_SIZE;
	if (block_write(new_block, block_offset, BLOCK_SIZE) != 0) {
		printf("Problems writing new block back to disk!\n");
		free(new_block);
		free(sb);
		return 1;
	}

	// Clean up
	free(new_block);
	
	// Add block number to inode's points
	memset(new_inode->block_pointers, 0, sizeof(new_inode->block_pointers));
	for (j = 0; j < 10; j++) {
		if (new_inode->block_pointers[j] == 0) {
			new_inode->block_pointers[j] = block_number;
			break;
		}
	}

	// Write new inode to disk
	if (write_inode(new_inode) != 0) {
		printf("Problems writing new inode to disk!\n");
		free(new_inode);
		free(sb);
		return 1;
	}

	// Update block vector
	if (set_block_vector(sb->first_free_block) != 0) {
		// Problems
		free(sb);
		return 1;
	}
	
	// Update inode map vector
	map_update = set_inode_map(sb->first_free_inode);	
	if (map_update != 0) {
		// Problems
		return 1;
	}

	// Add directory entry to parent block
	// TODO: make this more robust
	parent_dir_block = path_node->block_pointers[0];
	parent_offset = (parent_dir_block - 1) * BLOCK_SIZE;
	parent_block = block_read(parent_offset);
	
	// SIMPLE CHECK FOR NOW
	parent_entry = (direntry *)malloc(sizeof(direntry));
	memset(parent_entry->name, 0, sizeof(parent_entry->name));
	parent_entry->inode_num = sb->first_free_inode;
	strcpy(parent_entry->name, filename);

	// Get number of directory entries in the parent block
	dirents = path_node->filesize / DIRENTRY_SIZE;

	// Add direntry to the dir block
	memcpy(&parent_block->data[dirents * DIRENTRY_SIZE], parent_entry, sizeof(direntry));

	// Write path inode back to disk
	if (write_inode(path_node) != 0) {
		printf("Problems writing path inode back to disk!\n");
		free(path_node);
		free(parent_block);
		return 1;
	}

	// Write parent block back to disk
	if (block_write(parent_block, parent_offset, BLOCK_SIZE) != 0) {
		printf("Problem writing parent block back to disk!\n");
		return 1;
	}

	// Update Superblock
	sb->first_free_block++;
	sb->first_free_inode++;
	sb->used_blocks++;

	// Clean up
	free(parent_block);
	free(parent_entry);

	// Write Superblock back to disk
	if(write_superblock(sb) != 0) {
		printf("Problems writing superblock to disk!\n");
		free(sb);
		return 1;
	}

	printf("File %s created!\n", filename);

	return 0;
}

/****************************************/
// Init functions

int init_root()
{
	int block_offset;

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
	root_inode->filesize = 16; // Has its own entry as default for root
	root_inode->block_pointers[0] = sb->first_free_block;

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
		printf ("Problem writing free block vector to disk!\n");
		return 1;
	}

	// Create block for root directory
	block *root_block = (block *)malloc(sizeof(block));

	// Create root direntry
	direntry *root_entry = (direntry *)malloc(sizeof(direntry));

	// Initialize details of root direntry
	memset(root_block, 0, sizeof(block));
	memset(root_entry->name, 0, sizeof(root_entry->name));
	root_entry->inode_num = 0;
	strcpy(root_entry->name, (char*)"/");

	// Write direntry into block
	memcpy(root_block->data, root_entry, sizeof(direntry));

	// Write root block (512B) to disk (empty)
	block_offset = (sb->first_free_block - 1) * BLOCK_SIZE;

	if (block_write(root_block, block_offset, BLOCK_SIZE) != 0) {
		printf("Problems writing root block to disk!\n");
		return 1;
	}

	// Clean up root
	free(root_entry);
	free(root_block);

	// Update superblock
	sb->first_free_inode++;
	sb->first_free_block++;
	sb->used_blocks++;

	if (write_superblock(sb) != 0) {
		printf("Problems writing superblock to disk!\n");
		free(sb);
		return 1;
	}

	return 0;
}

int init_fs(char *fs_path, int num_blocks)
{
	// Note: init_fs does NOT use generic functions as defined
	// in Controller.h, it initializes the disk on its own

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

	create_file("/file.txt");
//	create_file("/test.c");
//	create_file("/boop.txt");
//	create_file("/number1.txt");
//	create_file("/number2.txt");

	
	mkdir("/usr");
	mkdir("/usr/Andrew/");
	mkdir("/usr/Andrew/Documents/");
	mkdir("/usr/test/temp");

	create_file("/usr/Andrew/Documents/foo.txt");

	return 0;
}
