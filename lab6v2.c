
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/stat.h>
#include <ext2fs/ext2_fs.h>
#include <time.h>

typedef unsigned int   u32;

//Define shorter TYPES, save typing efforts
typedef struct ext2_group_desc  GD;
typedef struct ext2_super_block SUPER;
typedef struct ext2_inode       INODE;
typedef struct ext2_dir_entry_2 DIR;//Need this for new version of e2fs

GD    *gp;
SUPER *sp;
INODE *ip;
DIR   *dp;

#define BLKSIZE 1024
#define BLOCK_OFFSET(block) (1024 + (block - 1) * 1024)

char buf[BLKSIZE], *device;
int fd, iblock;
int imap, bmap;
int ninodes, nblocks, nfreeInodes, nfreeBlocks;

char  *tokenPath[64] = {"\0"}, *disk, pathtemp[64][64], *path;

// my tokenize method because strtok is the devil's work
int tokenizePath(char input[], int index)
{
    printf("%s \n", input);
    char temp[64];
    if (*input == '\0')
    {
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
        tokenizePath(input, index);
    }
}

int tokenize(char input[])
{
	tokenizePath(input, 0);
}
//Step 0
void get_block(int fd, int blk, char buf[BLKSIZE])
{
	lseek(fd, (long)(blk*BLKSIZE), 0);
	read(fd, buf, BLKSIZE);
}

static void get_inode(int fd, int ino, INODE *inode)
{
	lseek(fd, BLOCK_OFFSET(iblock) + (ino - 1) * sizeof(INODE), SEEK_SET);
	read(fd, inode, sizeof(INODE));
}


//prelab
super()
{
  // Read SUPER block
  get_block(fd, 1, buf);
  sp = (SUPER *)buf;

  // Check for EXT2 magic number:
  // Lets us know if it is an EXT2FS
  printf("s_magic = \t\t\t\t%x\n", sp->s_magic);
  if (sp->s_magic != 0xEF53){
    printf("NOT an EXT2 FS\n");
    exit(1);
  }
}

int search(INODE *ip, char *name)
{
    char dbuf[1024], dirname[256];
    DIR *dp; 
    char *cp;
    get_block(fd, ip->i_block[0], dbuf);
    dp = (DIR*)dbuf;
    cp = dbuf;
    while (cp < (dbuf + BLKSIZE))
    {
		strncpy(dirname, dp->name, dp->name_len);
		dirname[dp->name_len] = 0;
        printf("%s  ", dirname);
		if (!strcmp(name, dirname))
		{
			printf("Found\n");
			return dp->inode;
		}
        cp += dp->rec_len;       // advance cp by rec_len in BYTEs
        dp = (DIR*)cp;
    }
	printf("Path not found \n");
	return 0;
}

void printStuff(int ino, int num)
{
	int i, j, cycle_blocks, num_blocks, indirect[256], double_indirect[256];
	INODE file;
	SUPER super;
	int blk_size = 1024;

	lseek(fd, iblock, SEEK_SET);
	read(fd, &super, sizeof(super));

	get_inode(fd, ino, &file);
	num_blocks = file.i_size / BLKSIZE;
	cycle_blocks = num_blocks;

	printf("PRINT---------------\n");
	printf("--------%s---------\n", tokenPath[num-1]);
	printf("size: %u\n", file.i_size);
	printf("blocks: %u\n", num_blocks);
	printf("access time: %s", ctime(&(file.i_atime)));
	printf("creation time: %s", ctime(&(file.i_ctime)));
	printf("modification time: %s", ctime(&(file.i_mtime)));
	printf("gid: %d\n", file.i_gid);
	printf("links count: %d\n", file.i_links_count);
	printf("flags: %d\n", file.i_flags);
	printf("blk = %d\n", blk_size);

	printf("\n-------- DISK Blocks\n");
	for (i = 0; i < 14; i++)
	{
		printf("block[%d]: %d\n", i, file.i_block[i]);
	}

	printf("\nDIRECT Blocks --------\n");
	if (cycle_blocks > 12)
	{
		cycle_blocks = 12;
	}

	printBlocks(cycle_blocks, file.i_block);
	num_blocks -= cycle_blocks;
	printf("\nBlocks Remaining: %u\n", num_blocks);

	if (num_blocks > 0)
	{
		printf("IND Blocks----\n");
		cycle_blocks = num_blocks;
		if (cycle_blocks > 256)
		{
			cycle_blocks = 256;
		}
		get_block(fd, file.i_block[12], indirect);
		printBlocks(cycle_blocks, indirect);
		num_blocks -= cycle_blocks;
		printf("\nBlocks Remaining: %u\n", num_blocks);

		if (num_blocks > 0)
		{
			printf("Double IND Blocks-------\n");
			get_block(fd, file.i_block[13], double_indirect);
			for (j = 0; j < 256; j++)
			{
				if (double_indirect[j] == 0)
				{
					break;
				}

				printf("-------- %d --------\n", double_indirect[j]);
				cycle_blocks = num_blocks;

				if (cycle_blocks > 256)
				{
					cycle_blocks = 256;
				}

				get_block(fd, double_indirect[j], indirect);
				printBlocks(cycle_blocks, indirect);
				num_blocks -= cycle_blocks;
				printf("\nBlocks Remaining: %u\n", num_blocks);
			}
		}
	}

}


void printBlocks(int cycle_blocks, int indirect[256])
{
	int i;
	for (i = 0; i < cycle_blocks; i++)
	{
		printf("%d ", indirect[i]);
		if ((i + 1) % 16 == 0)
		{
			printf("\n");
		}
	}
}

int main(int argc, char *argv[])
{
	int i, j, ino, block, offset;
	char buf[BLKSIZE];
	char path[2048];
	char *temp, *cp;

	//Check if enough arguments
	if(argc > 2)
	{
		device = argv[1];
		strcpy(path, argv[2]);
	}
	else
	{
		printf("not enough arguments\n");
		exit(0);
	}

	//tokenize path
	j = tokenize(path);

	for(i = 0; i < j; i++)
		printf("  %s  ", tokenPath[i]);

  	fd = open(device, O_RDONLY);
	if(fd < 0)
	{
		printf("open %s failed\n", device);
		exit(1);
	}

	printf("\n");

	//task 1
	super();

	//task 2
	//This puts the group descriptor block in the buff
	//So we can access the inodes
	get_block(fd, 2, buf);
	gp = (GD *)buf; //Points to the struct in our buf
	iblock = gp->bg_inode_table;   // Get inode start block#
	printf("inode_block=\t\t\t\t%d\n", iblock);

	// get inode start block
	// iblock is the starting point of the Inodes
	get_block(fd, iblock, buf); //This puts the inode table into the buf

	//Task 3
	ip = (INODE *)buf + 1;      // This makes ip point at the 2nd INODE WHICH IS ROOT

	printf ("-------- Root Node Info --------\n");
	printf("mode=%4x ", ip->i_mode);
	printf("uid=%d  gid=%d\n", ip->i_uid, ip->i_gid);
	printf("size=%d\n", ip->i_size);
	printf("time=%s", ctime(&ip->i_ctime));
	printf("link=%d\n", ip->i_links_count);
	printf("i_block[0]=%d\n", ip->i_block[0]);

	for (i=0; i < j; i++)
	{
		ino = search(ip, tokenPath[i]);
		if (ino==0)
		{
			printf("can't find %s\n", tokenPath[i]); exit(1);
		}
		// Mailman's algorithm: Convert (dev, ino) to inode pointer
		block  = (ino - 1) / 8 + iblock;  // disk block contain this INODE 
		offset = (ino - 1) % 8;         // offset of INODE in this block
		get_block(fd, block, buf);
		ip = (INODE *)buf + offset;    // ip -> new INODE
	}
	//ino = fullSearch(j);

	//Task 9
	//Print some stuff
	printStuff(ino, j);


	return 0;
}
