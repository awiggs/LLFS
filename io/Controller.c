// File containing basic functions
// for disk manipulation

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "FS.h"
#include "Controller.h"

/****************************************/
// Basic disk functions

void* block_read(int offset)
{
	// Open disk
	if (open_fs(VDISK_PATH) != 0) {
		return NULL;
	}
	
	void* buffer = malloc(sizeof(block));

	// Move cursor to top of block
	fseek(vdisk, offset, SEEK_SET);

	// Read block into buffer
	fread(buffer, BLOCK_SIZE, 1, vdisk);

	// Close disk
	close_fs();

	return buffer;
}

int block_write(void* block, int offset, int block_size)
{
	// Open disk
	if (open_fs(VDISK_PATH) != 0) {
		return 1;
	}

	// Move cursor to top of block
	fseek(vdisk, offset, SEEK_SET);

	// Write block to disk
	fwrite(block, 1, block_size, vdisk);

	// Close disk
	close_fs();

	return 0;
}

/****************************************/
// Read/Write superblock

superblock* get_superblock()
{
	int block_offset;
	superblock* sb;

	// Set up superblock struct
	sb = (superblock *)malloc(sizeof(block));

	if (sb == NULL) {
		printf("Problems with malloc for superblock!\n");
		return NULL;
	}

	// Superblock is always the first block
	block_offset = 0;

	// Read superblock from disk
	sb = (superblock*)block_read(block_offset);

	return sb;
}

int write_superblock(superblock* sb)
{
	int block_offset;

	// Superblock is always the first block
	block_offset = 0;

	// Write superblock to disk
	if (block_write(sb, block_offset, BLOCK_SIZE) != 0) {
		printf("Problem writing superblock!\n");
		return 1;
	}

	// Clean up
	free(sb);

	return 0;
}

/****************************************/
// Read/Write free block vector

int* get_free_blocks()
{
	int block_offset;

	// Set up array for 128 ints (512 bytes);
	int* free_blocks = malloc(MAP_ENTRIES);

	if (free_blocks == NULL) {
		printf("Malloc failed for free block vector!\n");
		return NULL;
	}

	// Free block vector is always the second block
	block_offset = BLOCK_SIZE;

	// Read free block vector from disk
	free_blocks = (int *)block_read(block_offset);

	return free_blocks;
}

int write_free_blocks(int* free_blocks)
{
	int block_offset;

	// Free block vector is always the second block
	block_offset = BLOCK_SIZE;

	// Write free block vector to disk
	if (block_write(free_blocks, block_offset, BLOCK_SIZE) != 0) {
		printf("Problem writing free block vector to disk!\n");
		return 1;
	}

	// Clean up
	free(free_blocks);

	return 0;
}

int set_block_vector(int k)
{
	int *bv;

	// Get block vector
	bv = get_free_blocks();

	// Check bit: 1 = available slot
	if (TestBit(bv, k) == 1) {
		ClearBit(bv, k);
	} else {
		printf("Block vector item is already set!\n");
		return 1;
	}

	// Write block vector back to disk
	if (write_free_blocks(bv) != 0) {
		printf("Problems writing block vector to disk!\n");
		return 1;
	}

	return 0;
}

int clear_block_vector(int k)
{
	int *bv;

	// Get block vector
	bv = get_free_blocks();

	// Check bit: 0 = unavailable slot
	if (TestBit(bv, k) == 0) {
		SetBit(bv, k);
	} else {
		printf("Block vector item is already cleared!\n");
		return 1;
	}

	// Write block vector back to disk
	if (write_free_blocks(bv) != 0) {
		printf("Problems writing block vector to disk!\n");
		return 1;
	}

	return 0;
}

/****************************************/
// Read/Write inode map

int* get_inode_map()
{
	int block_offset;

	// Set up array for 128 ints (512 bytes)
	int* map = malloc(MAP_ENTRIES);

	if (map == NULL) {
		printf("Malloc failed for get inode map!\n");
		return NULL;
	}

	// inode map is always the third block
	block_offset = 2 * BLOCK_SIZE;

	// Read inode map from disk
	map = (int*)block_read(block_offset);

	return map;
}

int write_inode_map(int* map)
{
	int block_offset;

	// inode map is always the third block
	block_offset = 2 * BLOCK_SIZE;

	if (block_write(map, block_offset, sizeof(map)) != 0) {
		printf("Problems writing inode map to disk!\n");
		return 1;
	}

	// Clean up
	free(map);

	return 0;
}

int set_inode_map(int k)
{
	int *map;

	// Get map
	map = get_inode_map();

	// Check bit: 1 = available slot
	if (TestBit(map, k) == 1) {
		ClearBit(map, k);
	} else {
		printf("Inode already set in map!\n");
		return 1;
	}

	// Write map back to disk
	if (write_inode_map(map) != 0) {
		printf("Problems writing inode map to disk!\n");
		return 1;
	}

	return 0;
}

int clear_inode_map(int k)
{
	int *map;

	// Get map
	map = get_inode_map();

	// Check bit: 0 = unavailable slot
	if (TestBit(map, k) == 0) {
		SetBit(map, k);
	} else {
		printf("Inode map bit already cleared!\n");
		return 1;
	}

	// Write map back to disk
	if (write_inode_map(map) != 0) {
		printf("Problems writing inode map to disk!\n");
		return 1;
	}

	return 0;
}

/****************************************/
// Read/Write inodes

inode* get_inode(int inode_num)
{
	inode* ret;
	int block_number, block_offset, inode_offset;
	char* buffer;

	// Ensure requested inode number is valid
	if (inode_num >= MAX_FILES) {
		printf("Requested invalid inode -- returning NULL\n");
		return NULL;
	}

	// Get block number that contains desired inode
	block_number = 3 + (inode_num / (BLOCK_SIZE / INODE_SIZE));
	block_offset = (block_number * BLOCK_SIZE); // + ((block_number - 1) * BLOCK_SIZE);

	// Create buffer for block to be read into
	buffer = malloc(BLOCK_SIZE);

	// Get block containing inode
	buffer = (char*)block_read(block_offset);

	// Calculate inode offset
	inode_offset = inode_num % (BLOCK_SIZE / INODE_SIZE);

	// Create a return inode
	ret = (inode *)malloc(INODE_SIZE);

	if (ret == NULL) {
		printf("Malloc for getting inode failed!\n");
		return NULL;
	}

	// Grab inode bytes from block
	memcpy(ret, &buffer[inode_offset * INODE_SIZE], INODE_SIZE);	
	
	return ret;
}

int write_inode(inode* node)
{
	int inode_num, block_number, block_offset, inode_offset;

	// Get inode num to write back to disk
	inode_num = node->inode_num;

	// Get block number to write inode back to
	block_number = 3 + (inode_num / (BLOCK_SIZE / INODE_SIZE));
	block_offset = (block_number * BLOCK_SIZE);

	// Calculate inode offset
	inode_offset = inode_num % (BLOCK_SIZE / INODE_SIZE);

	if (block_write(node, (block_offset + (inode_offset * INODE_SIZE)), INODE_SIZE) != 0) {
		printf("Shit went bad\n");
		return 1;
	}

	// Clean up
	free(node);

	return 0;
}

int path_to_inode(char* path)
{
	const char d[2] = "/";
	int token_counter, j, k, entries, found_inode, found_next;
	int i = 0;
	char *token;
	char *copy;
	direntry *entry;
	block *parent_block;
	inode *parent_inode;

	if (!path) {
		return -1;
	}
	if (strlen(path) == 0 || strcmp(path, "/") == 0) {
		return 0;
	}

	// TODO: other directory paths
	
	// Get number of path tokens
	copy = strdup(path);
	token = strtok(copy, d);
	token_counter = 0;

	while (token != NULL) {
		token_counter++;
		token = strtok(NULL, d);
	}

	// Store tokens in an array
	char *tokens[token_counter];
	char *copy2 = strdup(path);
	tokens[i] = strtok(copy2, d);
	while (tokens[i] != NULL) {
		tokens[++i] = strtok(NULL, d);
	}

	// Find inode num at end of path
	for (j = 0; j < token_counter; j++) {
		// Get new token
		token = tokens[j];

		// Root?
		if (j == 0) {
			parent_inode = get_inode(0);
		} else {
			parent_inode = get_inode(found_inode);
		}

		// Get parent block
		parent_block = (block *)block_read((parent_inode->block_pointers[0] - 1) * BLOCK_SIZE);

		if (parent_block == NULL) {
			// Problems
			printf("Error reading parent block!\n");
			free(parent_inode);
			return -1;
		}

		// Get number of direntries in parent b lock
		entries = parent_inode->filesize / DIRENTRY_SIZE;

		// Search for token name in each direntry
		entry = (direntry *)malloc(sizeof(direntry));

		for (k = 0; k < entries; k++) {
			// Extract direntry
			memcpy(entry, &parent_block->data[sizeof(direntry) * k], sizeof(direntry));

			if (strcmp(entry->name, token) == 0) {
				found_inode = entry->inode_num;
				found_next = 1;
				break;
			}
		}

		// Make sure we didn't not find a directory
		if (found_next == 0 && j != (token_counter - 1)) {
			printf("Directory \"%s\" doesn't exist!\n", token);
			return -1;
		}

		// Clean up
		found_next = 0;
		free(entry);
		free(parent_block);
		free(parent_inode);
	}

	return found_inode;
}

/****************************************/
// Bit manipulation for maps

void SetBit(int A[], int k)
{
	A[k/32] |= 1 << (k%32);
}

void ClearBit(int A[], int k)
{
	A[k/32] &= ~(1 << (k%32));
}

int TestBit(int A[], int k)
{
	return ( (A[k/32] & (1 << (k%32) )) != 0);
}
