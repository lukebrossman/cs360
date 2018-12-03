/* Header file for the utility methods used in accessing the data blocks,
 in-memory nodes, directory names, base names etc of the ext2fs for use in the 
 command methods */
#ifndef UTIL_H
#define UTIL_H

#include "type.h" //include the main file for structs, macros, and c libraries


void get_block(int fd,int block, char *buf);
void put_block(int fd, int block, char *buf);
int tokenize_path(char *pathname);//break up a pathname into its constituent driectories and files
char *dirname(char *pathname);
char *basename(char *pathname);
unsigned long  getino(int *dev, char *pathname);
unsigned long  search(MINODE *mip, char *filename);
MINODE *iget(int dev, unsigned long ino);
void iput(MINODE *mip);
int searchname(MINODE *parent, unsigned long myino, char *name);
int findmyname(MINODE *parent, unsigned long myino, char *myname);
int findino(MINODE *mip, unsigned long *myino, unsigned long *parentino);
unsigned long ialloc(int dev);
unsigned long idealloc(int dev, unsigned long ino);
unsigned long balloc(int dev);
unsigned long bdealloc(int dev, unsigned long iblock);

void incFreeInodes(int dev);
void decFreeInodes(int dev);
void incFreeBlocks(int dev);
void decFreeBlocks(int dev);

// buf is a pointer to the block, char* because a block is BLKSIZE number of bytes, and char is 1 byte.
//given a block pointed to by buf, perform operation on bit given by BIT
// uses the mailman's algorithm to first find the appropriate byte, then the appropriate bit in that byte
int tst_bit(char *buf, int BIT); //test the value of given bit
int set_bit(char *buf, int BIT); //set given bit to 1
int clear_bit(char *buf, int BIT); //set given bit to 0
void patherror(char *cmdtemp);

#endif
