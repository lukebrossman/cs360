#include <unistd.h>
#include "type.h"
#define BLKSIZE 1024

void get_block(int fd, int blk, char buf[BLKSIZE])
{
    lseek(fd, (long)(blk*BLKSIZE), 0);
    read(fd, buf, BLKSIZE);
}

void put_block(int fd, int blk, char buf[BLOCK_SIZE])
{
    lseek(fd, (long)(blk * BLOCK_SIZE), 0);
    write(fd, buf, BLOCK_SIZE);
}


int ialloc(int dev)
{
    int  i;
    char buf[1024];

    // read inode_bitmap block
    get_block(dev, imap, buf);

    for (i=0; i < numinodes; i++){
        if (tst_bit(buf, i)==0){
            set_bit(buf,i);
            decFreeInodes(dev);

            put_block(dev, imap, buf);

            return i+1;
        }
    }
    printf("ialloc(): no more free inodes\n");
    return 0;
}

int tst_bit(char *buf, int bit)
{
    int i, j;
    i = bit/8; j=bit%8;
    if (buf[i] & (1 << j))
        return 1;
    return 0;
}

int set_bit(char *buf, int bit)
{
    int i, j;
    i = bit/8; j=bit%8;
    buf[i] |= (1 << j);
}

int decFreeInodes(int dev)
{
    char buf[BLKSIZE];

    // dec free inodes count in SUPER and GD
    get_block(dev, 1, buf);
    sp = (SUPER *)buf;
    sp->s_free_inodes_count--;
    put_block(dev, 1, buf);

    get_block(dev, 2, buf);
    gp = (GD *)buf;
    gp->bg_free_inodes_count--;
    put_block(dev, 2, buf);
}

int getino(MINODE *mp, char *name)
{
    int i = 0, j = 0, return_val = -1;
    char buf[1024], temp[1024];
    char *cp;

    //Break up the path
    //j = parse(name, pathparts);
    //put the iblock in the buf
    get_block(dev, mp->INODE.i_block[0], buf);
    dp = (DIR *)buf; //set the dir
    cp = buf; //set the cp

    while (cp < buf + BLKSIZE)
    {
        strncpy(temp, dp->name, dp->name_len);
        temp[dp->name_len] = 0;
        if (!strcmp(temp, name))
        {
            return_val = dp->inode;
            return return_val;
        }
        //clear temp
        memset(temp, 0, 1024);
        //increment
        cp += dp->rec_len;
        dp = (DIR *)cp;
        i++;
    }

    return return_val;
}

MINODE *iget(int dev, int ino)
{
    char buf[1024];
    int ipos = 0, off = 0;
    MINODE *mnode = malloc(sizeof(MINODE));
    INODE *inode = malloc(sizeof(INODE));

    ipos = (ino - 1)/8 + INODE_START_POS;
    off = (ino - 1) % 8;

    get_block(dev, ipos, buf);
    inode = (INODE *)buf + off;

    if(S_ISDIR(inode->i_mode))
    {
        mnode->INODE = *inode;
        return mnode;
    }
    else
    {
        printf("A file is not a directory!\n");
        return NULL;
    }
}

void iput(MINODE *mip)
{
    int ino = -1;
    char buf[1024];
    int ipos = 0, off = 0;
    INODE *inode = malloc(sizeof(INODE));

    ino = getino(mip, ".");

    mip->refCount--;
    if (mip->refCount)
        return;
    if (!mip->dirty)
        return;

    ipos = (ino - 1)/8 + INODE_START_POS;
    off = (ino -1) % 8;
    get_block(dev, ipos, buf);
    inode = (INODE*)buf + off;
    *inode = mip->INODE;

    put_block(dev, ipos, buf);
}

void mount_root()
{
    char buf[1024];

    dev = open("mydisk", O_RDWR);
    if (dev < 0)
    {
        printf ("Cannot open!\n");
        exit(0);
    }
    root = iget(dev, 2);
    P0->cwd = root;
    P1->cwd = root;

    get_block(dev, 1, buf);
    sp = (SUPER *)buf;

    numinodes = sp->s_inodes_count;
    numblocks = sp->s_blocks_count;
    if(sp->s_magic !=  0xEF53 && sp->s_magic != 0xEF51)
    {
        printf("Device is invalid!\n");
        exit(0);
    }

    // read Group Descriptor 0
    get_block(dev, 2, buf);
    gp = (GD *)buf;
    imap = gp->bg_inode_bitmap;
}