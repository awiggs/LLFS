// File containing basic functions
// for disk manipulation

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "FS.h"

/****************************************/
// Basic disk functions

void* block_read(int offset)
{
	void* buffer = malloc(sizeof(block));

	// Open disk
	if (open_fs(VDISK_PATH) != 0) {
		return NULL;
	}

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

	// Read superblock from disk
	if (block_write(sb, block_offset, BLOCK_SIZE) != 0) {
		printf("Problem writing superblock!\n");
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
	}

	// Clean up
	free(node);

	return 0;
}

/****************************************/
// Directory functions

//int create_dir(int inode_num)
//{
//	int block_number;
//
//	// Allocate new block
////	block new_block = (block*)malloc(BLOCK_SIZE);
//
//	// Get superblock
//	superblock* sb = (superblock *)malloc(BLOCK_SIZE);
//	sb = get_superblock();
//
//	// Fill with 16 byte dir entries
//	int i;
//	for (i = 0; i < (BLOCK_SIZE / DIRENTRY_SIZE); i++) {
//		// Create entries
//		direntry e = (direntry*)malloc(DIRENTRY_SIZE);
//		e->inode_num = -1;
//		e->name = "EMPTY";
//
//		// Write to next free block space on disk
//		
//
//	}
//
//	// Write to next free spot on disk
//
//	// Update new block number
//
//	// Return block number
//	return block_number;
//}

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
