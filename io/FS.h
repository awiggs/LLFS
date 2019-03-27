// FS Assignment, CSC 360
// Andrew Wiggins, V00817291

#include <stdio.h>
#include <stdlib.h>

/*****************************************************/

// FS Globals and Defines

#define NUM_BLOCKS 4096
#define BLOCK_SIZE 512 // bytes
#define INODE_SIZE 32  // bytes
#define DIRENTRY_SIZE 16 // bytes
#define MAGIC_NUM 0x2f8c8075
#define MAX_FILES 112 // based on 7 blocks of inodes
#define MAP_ENTRIES 128
#define VDISK_PATH "../disk/vdisk"

FILE *vdisk; // used for opening vdisk

/*****************************************************/

// FS Structs

typedef struct fs_sb {
	// Magic number
	int magic;

	// Number of blocks on disk
	int block_count;

	// Number of used data blocks
	int used_blocks;

	// Number of inodes
	int inode_count;

	// Number of files stored
	int file_count;

	// Max number of files allowed
	int max_files;

	// Current number of files
	int current_files;

	// Pointer to first free inode
	int first_free_inode;

	// Pointer to first free data block
	int first_free_block;

	// Padding to make superblock 512 bytes
	char padding[BLOCK_SIZE - (sizeof(int) * 9)];
} superblock;

typedef struct inode {
	// inode num
	int inode_num;

	// Directory flag, 0 -> file, 1 -> dir
	short directory_flag;

	// Size of inode file
	int filesize;

	// Pointers to data blocks
	short block_pointers[10];
	short indirect_pointer;

} inode;

typedef struct fs_db {
	// Pointers to next blocks
	int next_free;
	int prev_free;

	// Size of block left
	char data[BLOCK_SIZE - (sizeof(int) * 2)];
	
} block;

typedef struct direntry {
	// Dir name
	char name[12];

	// inode num
	int inode_num;
} direntry;

/*********************************************/
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
	return ( (A[k/32] & (1 << (k%32) )) != 0 );
}

