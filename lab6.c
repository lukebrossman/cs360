#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <linux/fs.h>
#include <ext2fs/ext2_fs.h>
#include <time.h>

typedef unsigned char  u8;
typedef unsigned short u16;
typedef unsigned int   u32;

// define shorter TYPES, save typing efforts
typedef struct ext2_group_desc  GD;
typedef struct ext2_super_block SUPER;
typedef struct ext2_inode       INODE;
typedef struct ext2_dir_entry_2 DIR;    // need this for new version of e2fs

GD    *gp;
SUPER *sp;
INODE *ip;
DIR   *dp; 

#define BLKSIZE 1024

/******************* in <ext2fs/ext2_fs.h>*******************************
struct ext2_super_block {
  u32  s_inodes_count;       // total number of inodes
  u32  s_blocks_count;       // total number of blocks
  u32  s_r_blocks_count;     
  u32  s_free_blocks_count;  // current number of free blocks
  u32  s_free_inodes_count;  // current number of free inodes 
  u32  s_first_data_block;   // first data block in this group
  u32  s_log_block_size;     // 0 for 1KB block size
  u32  s_log_frag_size;
  u32  s_blocks_per_group;   // 8192 blocks per group 
  u32  s_frags_per_group;
  u32  s_inodes_per_group;    
  u32  s_mtime;
  u32  s_wtime;
  u16  s_mnt_count;          // number of times mounted 
  u16  s_max_mnt_count;      // mount limit
  u16  s_magic;              // 0xEF53
  // A FEW MORE non-essential fields
};
**********************************************************************/

char buf[BLKSIZE], *tokenPath[64] = {"\0"}, *disk, pathtemp[64][64], *path;
int fd, iblock;

int tokenizePath(char input[], int index)
{
    printf("%s \n", input);
    char temp[64];
    if (*input == '\0')
    {
        printf("end tokenize\n");
        tokenPath[index] = 0;
        return index;
    }
    else
    {
        sscanf(input, "%[^/]", pathtemp[index]);
        tokenPath[index] = pathtemp[index];
        input += 1;
        if (*input != '\0')
            input += strlen(pathtemp[index]);
        index += 1;
        printf("%x \n", input);
        printf("%d \n", index);
        tokenizePath(input, index);
    }
}

int tst_bit(char *buf, int bit)
{
  int i, j;
  i = bit / 8;  j = bit % 8;
  if (buf[i] & (1 << j))
     return 1;
  return 0;
}

int get_block(int fd, int blk, char buf[ ])
{
  lseek(fd, (long)blk*BLKSIZE, 0);
  read(fd, buf, BLKSIZE);
}

int bmap()
{
  char buf[BLKSIZE];
  int  bmap, nblocks;
  int  i;

  // read SUPER block
  get_block(fd, 1, buf);
  sp = (SUPER *)buf;

  nblocks = sp->s_blocks_count;

  // read Group Descriptor 0
  get_block(fd, 2, buf);
  gp = (GD *)buf;

  bmap = gp->bg_block_bitmap;
  printf("bmap = %d\n", bmap);

  // read inode_bitmap block
  get_block(fd, bmap, buf);

  for (i=0; i < nblocks; i++){
    (tst_bit(buf, i)) ?	putchar('1') : putchar('0');
    if (i && (i % 8)==0)
       printf(" ");
  }
  printf("\n");
}

int super()
{
  // read SUPER block
  get_block(fd, 1, buf);  
  gp = (GD *)buf;

  printf("bg_block_bitmap = %d\n", gp->bg_block_bitmap);
  printf("bg_inode_bitmap = %d\n", gp->bg_inode_bitmap);

  printf("bg_inode_table = %d\n", gp->bg_inode_table);
  printf("bg_free_blocks_count = %d\n", gp->bg_free_blocks_count);
  printf("bg_free_inodes_count = %d\n", gp->bg_free_inodes_count);

  printf("bg_used_dirs_count = %d\n", gp->bg_used_dirs_count);
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

int main(int argc, char *argv[])
{ 
    int i = 0;
    if (argc > 2)
        disk = argv[1];
        path = argv[2];

      // read SUPER block
    get_block(fd, 1, buf);
    sp = (SUPER *)buf;
    tokenizePath(path, 0);
    
    for(i = 0; tokenPath[i] != 0; i++)
    {
        printf("%s  :", tokenPath[i]);
    }

    fd = open(disk, O_RDONLY);
    if (fd < 0)
    {
        printf("open %s failed\n", disk);
        exit(1);
    }

    //inode();
    //super();
}