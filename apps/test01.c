#include <stdio.h>
#include <stdlib.h>
#include "../io/File.h"
#include "../io/Config.h"

int main()
{
	// File for testing creating the File System,
	// creating the Root Dir, and adding ONE dir to
	// the root

	int init, root, m;

	init = init_fs(VDISK_PATH, NUM_BLOCKS);
	if (init != 0) return 1;

	root = init_root();
	if (root != 0) return 1;

	m = mkdir("/usr/");
	if (m != 0) return 1;

	return 0;
}
