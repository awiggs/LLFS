#include <stdio.h>
#include <stdlib.h>
#include "../io/File.h"
#include "../io/Config.h"

int main()
{
	// Section for testing creating the File System,
	// creating the Root Dir, and adding ONE dir to
	// the root

	int init, root, m;

	init = init_fs(VDISK_PATH, NUM_BLOCKS);
	if (init != 0) return 1;

	root = init_root();
	if (root != 0) return 1;

	m = mkdir("/usr/");
	if (m != 0) return 1;

	// Section for add more subdirectories, touching a new file,
	// writing an existing file's contents into that new file, and then
	// reading it back to the console
	// UNCOMMENT TO USE
//	int t, w, r;
//	m = mkdir("/usr/Andrew");
//	if (m != 0) return 1;
//
//	m = mkdir("/usr/Andrew/Documents/");
//	if (m != 0) return 1;
//
//	t = create_file("/usr/Andrew/Documents/Test.txt");
//	if (t != 0) return 1;
//
//	w = write_file("apps/Test.txt", "/usr/Andrew/Documents/Test.txt");
//	if (w != 0) return 1;
//
//	r = read_file("/usr/Andrew/Documents/Test.txt");
//	if (r != 0) return 1;

	// Section for deleting files and directories
	// UNCOMMENT TO USE
//	int d;
//	d = delete("/usr/Andrew/Documents/Test.txt");
//	if (d != 0) return 1;
//
//	// This case SHOULD FAIL
//	d = delete("/usr/Andrew");
//
//	d = delete("/usr/Andrew/Documents/");
//	if (d != 0) return 1;
//
//	d = delete("/usr/Andrew");
//	if (d != 0) return 1;

	return 0;
}
