// File for basic disk controls

/****************************************/
void* block_read(int offset);
int block_write(void* block, int offset, int block_size);

/****************************************/
superblock* get_superblock();
int write_superblock(superblock* sb);

/****************************************/
int* get_free_blocks();
int write_free_blocks(int* free_blocks);
int set_block_vector(int k);
int clear_block_vector(int k);

/****************************************/
int* get_inode_map();
int write_inode_map(int* map);
int set_inode_map(int k);
int clear_inode_map(int k);

/****************************************/
inode* get_inode(int inode_num);
int write_inode(inode* node);
int path_to_inode(char* path);

/****************************************/
void SetBit(int A[], int k);
void ClearBit(int A[], int k);
int TestBit(int A[], int k);
