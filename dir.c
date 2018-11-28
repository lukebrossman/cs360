/********* inode.c: print information in / INODE (INODE #2) *********/

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <ext2fs/ext2_fs.h>

#define BLKSIZE 1024

// define shorter TYPES, save typing efforts
typedef struct ext2_group_desc  GD;
typedef struct ext2_super_block SUPER;
typedef struct ext2_inode       INODE;
typedef struct ext2_dir_entry_2 DIR;    // need this for new version of e2fs

GD    *gp;
SUPER *sp;
INODE *ip;
DIR   *dp; 

int fd;
int iblock;

int get_block(int fd, int blk, char buf[ ])
{
  lseek(fd,(long)blk*BLKSIZE, 0);
   read(fd, buf, BLKSIZE);
}

int search(INODE *ip, char *name)
{
    char dbuf[1024];
    DIR *dp; 
    char *cp;
    get_block(fd, ip->i_block[0], dbuf);
    dp = (DIR*)dbuf;
    cp = dbuf;
    while (cp < (dbuf + 1024))
    {
        printf("%.*s  ", dp->name_len, dp->name);
        cp += dp->rec_len;       // advance cp by rec_len in BYTEs
        dp = (DIR*)cp;
    }
}

int inode()
{
  char buf[BLKSIZE];

  // read GD
  get_block(fd, 2, buf);
  gp = (GD *)buf;
  /****************
  printf("%8d %8d %8d %8d %8d %8d\n",
	 gp->bg_block_bitmap,
	 gp->bg_inode_bitmap,
	 gp->bg_inode_table,
	 gp->bg_free_blocks_count,
	 gp->bg_free_inodes_count,
	 gp->bg_used_dirs_count);
  ****************/ 
  iblock = gp->bg_inode_table;   // get inode start block#
  printf("inode_block=%d\n", iblock);

  // get inode start block     
  get_block(fd, iblock, buf);

  ip = (INODE *)buf + 1;         // ip points at 2nd INODE
  search(ip, buf);
  printf("mode=0x%4x\n", ip->i_mode);
  printf("uid=%d  gid=%d\n", ip->i_uid, ip->i_gid);
  printf("size=%d\n", ip->i_size);
  printf("time=%s", ctime(&ip->i_ctime));
  printf("link=%d\n", ip->i_links_count);
  printf("i_block[0]=%d\n", ip->i_block[0]);

 /*****************************
  u16  i_mode;        // same as st_imode in stat() syscall
  u16  i_uid;                       // ownerID
  u32  i_size;                      // file size in bytes
  u32  i_atime;                     // time fields  
  u32  i_ctime;
  u32  i_mtime;
  u32  i_dtime;
  u16  i_gid;                       // groupID
  u16  i_links_count;               // link count
  u32  i_blocks;                    // IGNORE
  u32  i_flags;                     // IGNORE
  u32  i_reserved1;                 // IGNORE
  u32  i_block[15];                 // IMPORTANT, but later
 ***************************/
}

char *disk = "mydisk";
int main(int argc, char *argv[])
{ 
  if (argc > 1)
    disk = argv[1];

  fd = open(disk, O_RDONLY);
  if (fd < 0){
    printf("open %s failed\n", disk);
    exit(1);
  }

  inode();
}

/*   Modify YOUR dir.c to write a function
               int search(INODE *ip, char *name)
   which searches the directory INODE pointed by ip for a name string,
   return name's inode number if found; return 0 if NOT.

----------------------------------------------------------------------------
              HOW TO STEP THROUGH dir_entries:

   struct ext2_dir_entry_2 {
	u32  inode;        // Inode number; count from 1, NOT from 0
	u16  rec_len;      // This entry length in bytes
	u8   name_len;     // Name length in bytes
	u8   file_type;    // for future use
	char name[EXT2_NAME_LEN];  // File name: 1-255 chars, no NULL byte
   }

Assume the root directory / contains entries 
           this  is  aVeryLongName short
Then the 0th data block (i_block[0] in its inode) of this DIR contains:
 
|2 12 1.|2 12 2..|11 12 4this|12 12 2is|13 24 13aVeryLongName|14 952 5short   |

Each record has a rec_len and a name_len field.

First, read the block into a char dbuf[1024].
Let DIR *dp and char *cp BOTH point at dbuf;
Use dp-> to access the fields of the record, e.g. print its name

                    TO MOVE TO THE NEXT entry:
        cp += dp->rec_len;       // advance cp by rec_len in BYTEs
        dp = (shut-up)cp;        // pull dp along to the next record

This way, you can step through ALL the record entries of a DIR file.*/
