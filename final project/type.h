/*	type header file.
  -defines KC's short types to save on precious typing efforts.
  -defines some enum types for my own benifit, readability so that I'm
    not super confused about what is going on.
  -some custom structs for handling the commands in a function table
  -some structs for handling memory nodes and open files, etc... 
  -includes all the c libraries needed*/
#ifndef TYPE_H
#define TYPE_H

#include <stdio.h>
#include <fcntl.h>
#include <linux/fs.h>
#include <ext2fs/ext2_fs.h>
#include <time.h>
#include <libgen.h>
#include <string.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <sys/ioctl.h>

// define shorter TYPES, save typing efforts
typedef struct ext2_group_desc  GD;
typedef struct ext2_super_block SUPER;
typedef struct ext2_inode       INODE;
typedef struct ext2_dir_entry_2 DIR;    // need this for new version of e2fs

GD    *gp;
SUPER *sp;
INODE *ip;
DIR   *dp; 

#ifndef BLOCK_SIZE
#define BLOCK_SIZE        1024
#endif

#define BITS_PER_BLOCK    (8*BLOCK_SIZE)
#define INODES_PER_BLOCK  (BLOCK_SIZE/sizeof(INODE))

// Block numbers for EXT2 FS on FD
#define SUPERBLOCK        1
#define GDBLOCK           2
#define BBITMAP           3
#define IBITMAP           4
#define INODEBLOCK        5
#define ROOT_INODE        2
#define INODE_START_POS   5

// standard dr and file modes
#define DIR_MODE          0040777 
#define FILE_MODE         0100644
#define SUPER_MAGIC       0xEF53
#define SUPER_USER        0

// defining proccess status for code readability
#define FREE              0
#define BUSY              1
#define KILLED            2

// Table sizes
#define NMINODES          100
#define NMOUNT            10
#define NPROC             10
#define NFD               10
#define NOFT              100

// File types for code readability
#define LINK 			0
#define FILE 			1
#define DIRECTORY		2

// Globals
PROC P[2]; //process array
MINODE minode[NMINODES]; //memory inode array of size 100
int dev, sleepmode;
MINODE *root; //memeroy inode root of the files system
PROC *running, *readQueue;
char pathname[256], parameter[256], *name[256];


// Table for open files
typedef struct Oft{
  int   mode, refCount;
  struct Minode *inodeptr;
  long  offset;
} OFT;

// PROC structure
typedef struct Proc{
  int   uid, pid, gid, ppid, status;
  struct Proc *parent; // parent process
  struct Minode *cwd; //working directory for the process
  OFT   *fd[NFD]; //table of file descriptors open
} PROC;
      
// In-memory inodes structure
typedef struct Minode{		
  INODE    INODE;               // disk inode
  ushort   dev, refCount, dirty, mounted;
  unsigned long ino;
  struct Mount *mountptr;
} MINODE;

// Mount Table structure
typedef struct Mount{
  int    ninodes, nblocks, dev, busy;
  struct Minode *mounted_inode;
  char   name[256], mount_name[64]; 
} MOUNT;

// Function for the function table so that I can store all functions in the same
// table regardless of # of parameters
typedef struct Command{
  char *functionName; //function name as string input from the user, used for lookup of the function pointer.
  int (*f)();
  int paramOptional; // 0 = no, 1 = yes, 2 = no optional parameters
} command;

#endif 
