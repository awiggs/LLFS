// Header containing OS-level function definitions
#ifndef FILE_HEADER
#define FILE_HEADER
//#include "FS.h"
//#include "Controller.h"

/****************************************/
int open_fs(char*);
void close_fs(void);
int read_file(char*);
int write_file(char*, char*);
int mkdir(char*);
int create_file(char*);
int init_fs(char*, int);
int init_root();

#endif /* FILE_HEADER */
