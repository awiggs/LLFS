#include <stdio.h>
#include <stdlib.h>
#include "../io/File.h"
#include "../io/Config.h"

int main()
{
	// File assumes that the File System has be initialized
	// and that it has and Root Dir + /usr directory
	// RUN TEST01.C FIRST BEFORE RUNNING THIS OR IT'LL FAIL

	// This file creates a few more directories and then touches a file
	// into "/usr/Andrew/Documents"/, followed by reading Test.txt into
	// the empty touched file

	int m, c;

	m = mkdir("/var");
	if (m != 0) return 1;

	m = mkdir("/usr/Andrew/");
	if (m != 0) return 1;

	m = mkdir("/usr/Andrew/Documents");
	if (m != 0) return 1;

	c = create_file("/usr/Andrew/Documents/Test.txt");
	if (c != 0) return 1;

	return 0;
}
