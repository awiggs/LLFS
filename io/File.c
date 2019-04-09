// FS Assignment, CSC 360
// Andrew Wiggins, V00817291

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "Config.h"
#include "File.h"
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
int read_file(char *path)
{
	// Path = path to read file from on FS
	char *contents;
	int filesize;
	int inode_num, block_offset;
	inode *path_inode;
	block *path_block;

	// Get inode of specified file
	inode_num = path_to_inode(path);

	if (inode_num < 0) {
		// Problems
		return 1;
	}
	
	path_inode = get_inode(inode_num);

	// Get filesize
	filesize = path_inode->filesize;
	contents = malloc(filesize + 1);
	
	// Get block
	block_offset = (path_inode->block_pointers[0] - 1) * BLOCK_SIZE;
	path_block = (block *)block_read(block_offset);

	// Get contents of block
	memcpy(contents, &path_block->data, filesize);

	// Print contents of the file to command line
	printf("Contents:\n%s\n", contents);

	// Clean up
	free(path_block);
	free(contents);
	free(path_inode);

	return 0;
}

int write_file(char *file, char *path)
{
	// File = file to be read from host machine
	// Path = path to write the file to
	FILE *host;
	char *contents;
	long host_size;
	int inode_num, block_offset;
	inode *path_inode;
	block *path_block;

	// Open file from (real) disk
	host = fopen(file, "rb");
	if (!host) {
		printf("Could not open file!\n");
		return 1;
	}
	fseek(host, 0, SEEK_END);

	// Ensure proper sizing before reading
	host_size = ftell(host);
	rewind(host);

	// Read file into memory
	contents = malloc(host_size + 1);
	fread(contents, 1, host_size, host);
	fclose(host);

	// Null termination
	contents[host_size] = 0;

	// Get inode we need to write to
	inode_num = path_to_inode(path);

	if (inode_num < 0) {
		// Problems
		free(contents);
		return 1;
	}
	
	path_inode = get_inode(inode_num);

	// Get block to write to
	block_offset = (path_inode->block_pointers[0] - 1) * BLOCK_SIZE;
	path_block = (block *)block_read(block_offset);

	if (path_block == NULL) {
		// Problems
		free(contents);
		free(path_inode);
		return 1;
	}

	// Write contents to block
	memcpy(&path_block->data, contents, host_size);
	free(contents);

	// Write block back to disk
	if (block_write(path_block, block_offset, BLOCK_SIZE) != 0) {
		// Problems
		free(path_block);
		free(path_inode);
		return 1;
	}

	free(path_block);

	// Update + write inode
	path_inode->filesize += host_size;
	if (write_inode(path_inode) != 0) {
		// Problems
		free(path_inode);
		return 1;
	}

	return 0;
}

int mkdir(char *path)
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
	//sb->first_free_inode++;
	//sb->first_free_block++;
	int test = update_first_free(sb);
	if (test != 0) return 1;
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

	// Get next available inode for the file
	sb = get_superblock();
	new_inode = get_inode(sb->first_free_inode);
	new_inode->directory_flag = 0;

	// Get block to use for the file
	new_block = (block *)malloc(sizeof(block));
	memset(new_block->data, 0, sizeof(new_block->data));
	block_number = sb->first_free_block;	

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
	path_node->filesize += 16;
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
	int test = update_first_free(sb);
	if (test != 0) return 1;
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

	printf("File \"%s\" created!\n", filename);

	return 0;
}

int delete(char* path)
{
	int i, path_inode, block_num, num_blocks, updated, data_length;
	int path_length, slash_index, pinode, k, entries, found_entry, pblocks;
	char *ppath;
	char entry_name[12];
	inode *node, *pnode;
	block *dblock;
	superblock *sb;
	direntry *entry;

	// Get inode num for end of path item
	path_inode = path_to_inode(path);

	if (path_inode < 0) {
		return 1;
	}

	// Get inode
	node = get_inode(path_inode);

	// Check if empty
	if (node->filesize > 0 && node->directory_flag == 1) { // Non-empty
		printf("Cannot delete item that isn't empty.\n");
		free(node);
		return 1;
	} else { // Empty
		// Get block(s)
		num_blocks = node->filesize / BLOCK_SIZE;
		for (i = 0; i < num_blocks + 1; i++) {
			// Get block num
			block_num = node->block_pointers[i];

			// Get block
			dblock = (block *)block_read((block_num - 1) * BLOCK_SIZE);

			if (dblock == NULL) {
				printf("Problems reading block!\n");
				free(dblock);
				free(node);
				return 1;
			}

			// Set the block's mem to 0s
			data_length = strlen(dblock->data);
			memset(dblock->data, 0, BLOCK_SIZE);
			data_length = strlen(dblock->data);

			// Write block back to disk
			if (block_write(dblock, (block_num - 1) * BLOCK_SIZE, BLOCK_SIZE) != 0) {
				printf("Problems writing block back to disk!\n");
				free(dblock);
				free(node);
				return 1;
			}

			free(dblock);

			// Remove pointer from node
			node->block_pointers[i] = 0;

			// Update node filesize
			node->filesize -= data_length;

			// Update Free Block Vector
			if (clear_block_vector(block_num) != 0) {
				// Problems
				free(node);
				return 1;
			}

			// Get + update + write Superblock
			sb = get_superblock();
			updated = update_first_free(sb);
			if (updated != 0) {
				// Problems
				free(node);
				free(sb);
				return 1;
			}
			sb->used_blocks--;
			if (write_superblock(sb) != 0) {
				printf("Problems writing Superblock!\n");
				free(node);
				free(sb);
				return 1;
			}

			// Update parent
			path_length = strlen(path);
			for (i = 0; i < path_length; i++) {
				if (path[i] == '/' && (i != (path_length - 1))) {
					slash_index = i;
				}
			}
			ppath = strdup(path);
			ppath[slash_index] = '\0';
			memcpy(entry_name, &ppath[slash_index + 1], 12);

			// Check for slash at end of dir name
			if (entry_name[strlen(entry_name) - 1] == '/') {
				entry_name[strlen(entry_name) - 1] = '\0';
			}

			// Get parent inode
			pinode = path_to_inode(ppath);
			pnode = get_inode(pinode);

			// Search through dir blocks to find direntry to delete
			pblocks = pnode->filesize / BLOCK_SIZE;
			entries = pnode->filesize / DIRENTRY_SIZE; 
			for (i = 0; i <= pblocks; i++) {
				// Get block
				block_num = pnode->block_pointers[i];
				dblock = (block *)block_read((block_num - 1) * BLOCK_SIZE);

				// Search through entries
				for (k = 0; k < entries; k++) {
					// Extract direntry
					entry = (direntry *)malloc(sizeof(direntry));
					memcpy(entry, &dblock->data[sizeof(direntry) * k], sizeof(direntry));

					if (strcmp(entry->name, entry_name) == 0) {
						// Found it, so remove it from mem
						memset(&dblock->data[sizeof(direntry) * k], 0, sizeof(direntry));
						found_entry = 1;
						free(entry);
						break;
					}
					free(entry);
				}


				if (found_entry == 1) {
					// Write block back to disk
					if (block_write(dblock, (block_num - 1) * BLOCK_SIZE, BLOCK_SIZE) != 0) {
						// Problems
						printf("Problems writing dblock back to disk!\n");
						free(entry);
						free(dblock);
						free(pnode);
						return 1;
					}
					free(dblock);
					break;
				}
			}

			free(pnode);

			printf("\"%s\" deleted!\n", entry_name);
		}

		// Update inode map
		if (clear_inode_map(path_inode) != 0) {
			// Problems
			free(node);
			return 1;
		}

		// Clean up
		free(node);


		return 0;
	}

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
	//sb->first_free_inode++;
	//sb->first_free_block++;
	int test = update_first_free(sb);
	if (test != 0) return 1;
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
	sb->first_free_block = 10;
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

