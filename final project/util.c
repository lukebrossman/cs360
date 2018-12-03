#include "functions.h"
#include "type.h"
#include "util.h"

// read the block of data from the file device (fd) into the buffer (buf).
void get_block(int fd,int block, char *buf)
{
	//set the pointer to the block number to read
	lseek(fd,(long)(BLOCK_SIZE*block),0);
	//read from the block pointed to by fd, blocksize number of bits into buff
	read(fd,buf,BLOCK_SIZE); 
	//buf is an uotput parameter here
}

// put the block of data from the given buffer into the given file descriptor
void put_block(int fd, int block, char *buf)
{
	//set the pointer to the block number to write
	lseek(fd, (long)(BLOCK_SIZE*block),0);
	//write the buf contents to the block pointed to by fd
	write(fd, buf, BLOCK_SIZE);
}

// tokenize a path into its sub-components and return the number of pieces to the path
int tokenize_path(char *path)
{
	char *temp;
	int i = 0;
	
	temp = strtok(path, "/");
	while(temp != NULL)
	{
		//store the token in our global path array
		name[i] = temp;
		temp = strtok(NULL, "/");
		i++;
	}
	//return the number of dir names/levels that are contained in the given path
	return i;
}

// return the directory of the path
char *dirname(char *pathname)
{
	//how many characters is the pathname? -1 because 0 based
	int i = strlen(pathname) - 1;
	char *dirname;
	
	while(pathname[i] != '/')
	{
		i--;
	} //i is now the charater of the first '/' from the end of the pathname
		
	if (!i)//there was no '/' in the given pathname, so dirname is the current directory
	{
		dirname = (char *)malloc(2*sizeof(char));
		strcpy(dirname, "/"); //dirname is now the current directory
	}
	else
	{
		dirname = (char *)malloc((i+1)*sizeof(char));
		strncpy(dirname, pathname, i); //set dirname to the first i characters of pathname
		dirname[i] = 0; //set the last char of dirname to null incase it wasn't already
	}
	return dirname; //dirname is now everything up to the basname in the path
}

// return the basename of the path, i.e the last element of the pathname
char *basename(char *pathname)
{
	int i = strlen(pathname) - 1; // -1 because 0 based
	char *basename; //will store the basename
	while(pathname[i] != '/')
		i--;
		
	basename = (char *)malloc((strlen(pathname)-i)*sizeof(char));
	strcpy(basename,&pathname[i+1]);//copy everything from that last '/' to the end of pathname
	return basename;
}

// get the ino number and device that corresponds to the given pathname, device here is an output parameter
unsigned long getino(int *device, char *pathname){
	int i, pathlevels;
	unsigned long inumber;
	char path[256];
	MINODE *mip;

	if(pathname[0] == '/') //path is absolute
	{
		*device = root->dev; //set our device output to the root device
		inumber = root->ino; //set inumber to the index number from the root
	}
	else //local path means we need to access info from current working directory
	{
		*device = running->cwd->dev; //set device to the device of the running process
		inumber = running->cwd->ino;//set inumber to the indexnumber of the cwd of running process
	}

	// need to copy over the pathname because we want to preserve the original pathname for future use
	strcpy(path, pathname);
	//break our pathname up into each level so we have number of leavels to check
	pathlevels = tokenize_path(path);

	//we need to iterate potentially through every level, unless we reach the end (pathname)
	for(i = 0; i < pathlevels ; i++)
	{
		mip = iget(*device, inumber); // get the minode for the current inumber
		inumber = search(mip, name[i]); // search for the next inumber based on the results

		// if inumber is 0 then we reached the last inode of the block before the end of the pathname
		if(inumber == 0)
		{
			printf("The given path does not exist\n",name[i]);
			iput(mip); // cleanup
			return 0; // we failed to find the given pathname, DOES NOT MEAN PATHNAME IS ROOT! ROOT IS INO 2 (. and .. are 0 and 1)
		}

		// FLAG
		if((mip->INODE.i_mode & 0040000) != 0040000)//from pg 254 in the book
		{
			printf("The path part '%s' is not a directory!\n",name[i]);
			iput(mip); // cleanup
			return 0; // return that we failed :(
		}
		iput(mip); // cleanup
	}
	return inumber; // return the inode number referenced by the path
}

//search for the file called filename in the given inode
unsigned long search(MINODE * mip, char *filename)
{
	int i;
	char buf[BLOCK_SIZE], namebuf[256], *cp;

	//each inode has 12 direct blocks iterate over them
	for(i = 0; i <= 11 ; i ++)
	{
		if(mip->INODE.i_block[i])//null pointer means no more blocks
		{
			get_block(mip->dev, mip->INODE.i_block[i], buf);//read that block into buf
			dp = (DIR *)buf;
			cp = buf;

			while(cp < &buf[BLOCK_SIZE])
			{
				strncpy(namebuf,dp->name,dp->name_len); // get the record name at dp
				namebuf[dp->name_len] = 0; //dp->name does not end in a null character, we need to add

				if(!strcmp(namebuf, filename))//compare to our filname parameter to see if this is the record we want
				{
					return  dp->inode;//return the inode number of the file
				}
				cp +=dp->rec_len;//move the pointer ahead by the length of the record
				dp = (DIR *)cp;//cast the pointer as a DIR so we can access name and name length of record
			}
		}    
	}
	return 0;//0 return means we didn't find the file we wer looking for
}

// get the minode corresponding to the given device, and inode number of that device
MINODE *iget(int dev, unsigned long ino){
	int i, nodeIndex, blockIndex;
	INODE *cp;
	char buf[BLOCK_SIZE];
	
	// iterate through the minodes in memory to see if minode already in memory
	for(i = 0; i < NMINODES; i++)
	{
		if(minode[i].refCount) //just to make sure that there a references to the minode at index i
		{
			//check the minode at index i to see if the device number matches our device
			//and that the inode number matches the inode we are looking for
			if(minode[i].dev == dev && minode[i].ino == ino)
			{
				minode[i].refCount++;//we have a new reference to the minode
				return &minode[i];//return a pointer to the minode
			}
		}
	}

	//minode isn't already in memory so find a space in the minode table to add it
	for(i = 0; i < NMINODES; i++)
	{
		if(minode[i].refCount == 0)//if no references to this minode it is safe to overwrite
		{
			nodeIndex = (ino - 1) % INODES_PER_BLOCK;//mailmans algorithm to get the inode index we want
			blockIndex = (ino - 1) / INODES_PER_BLOCK + INODEBLOCK; //mailmans algorithm to get the blockindex we want from device
			get_block(dev,blockIndex,buf);//get the block we want so we can write it into the minode table
			cp = (INODE *)buf;//we need to be able to reference the buffer as a block
			cp += nodeIndex;//move the pointer to BLOCK_SIZE number of bytes up to the inode we want
			minode[i].INODE = *cp;//set the minode.inode at i to point at our inode
			minode[i].dev = dev;//device is the given device so we can find later
			minode[i].ino = ino;//which index is this inode
			minode[i].refCount = 1;//so far only one reference to this inode
			minode[i].dirty = 0;//clean since we just added it
			return &minode[i];//return pointer to the minode we just updated
		}
	}
	
	printf("ERROR: MINODE TABLE HAS NO MORE SPACE(\n");
	exit(0);
}

//save minode given back to the disk
void iput(MINODE *mip)
{
	int nodeIndex,blockIndex;
	char buf[BLOCK_SIZE];

	mip->refCount--;//saving means we are done with this minode

	//we need to check if someone else is usng this minode, or maybe it doesn't need to be saved
	//because nothing has been updated (clean)
	if((mip->refCount) || (mip->dirty == 0))
	{
		return;
	}

	//if we are here we are the only one using minode, or it needs to be saved because it is dirty
	nodeIndex = (mip->ino -1 ) % INODES_PER_BLOCK; //mailmans
	blockIndex = (mip->ino -1) / INODES_PER_BLOCK + INODEBLOCK; //mailmans

	get_block(mip->dev,blockIndex, buf);//get the block that we care about
	ip = (INODE *)buf;
	ip += nodeIndex;
	*ip = mip->INODE;//get the inode that we care about from the block
	put_block(mip->dev,blockIndex,buf); //save that block so that it is up to date
}

//search for a file with the given inode index and return its name as output parameter
int searchname(MINODE *parent, unsigned long ino, char *name)
{
	int i;
	char buf[BLOCK_SIZE], namebuf[256], *cp;
	//iterate over direct blocks
	for(i = 0; i <= 11 ; i ++)
	{
		if(parent->INODE.i_block[i] != 0)
		{
			get_block(parent->dev, parent->INODE.i_block[i], buf);//get block into buf
			dp = (DIR *)buf;
			cp = buf;

			while(cp < &buf[BLOCK_SIZE])//iterate over all the records in the block
			{
				strncpy(namebuf,dp->name,dp->name_len);
				namebuf[dp->name_len] = 0;

				if(dp->inode == ino)//this is the file we are looking for
				{
					strcpy(name, namebuf);//copy file name to output parameter
					return 1;
				}
				cp +=dp->rec_len;
				dp = (DIR *)cp;
			}
		}
	}
	return -1; //we didn't find the inode we were looking for
}

//FLAG 
int findino(MINODE *mip, unsigned long *myino, unsigned long *parentino)
{
	int i;
	char buf[BLOCK_SIZE], namebuf[256], *cp;

	get_block(mip->dev, mip->INODE.i_block[0], buf);
	dp = (DIR *)buf;
	cp = buf;
	 
	*myino = dp->inode;
	cp +=dp->rec_len;
	dp = (DIR *)cp;
	*parentino = dp->inode;	    	
	return 0;
}

// allocate an inode
unsigned long ialloc(int dev)
{
	int i, ninodes;
	char buf[BLOCK_SIZE]; // BLOCK_SIZE = block size in bytes
	SUPER *temp;

	// get total number of inodes
	get_block(dev,SUPERBLOCK,buf);
	temp = (SUPER *)buf;
	ninodes = temp->s_inodes_count;
	put_block(dev,SUPERBLOCK,buf);

	//load our bitmap to look for free space
	get_block(dev, IBITMAP,buf);

	for(i = 0; i < ninodes ; i++) 
	{
		if(TST_bit(buf,i) == 0)//free space found
		{
			SET_bit(buf,i);
			put_block(dev,IBITMAP,buf); //save our changes

			decFreeInodes(dev);//inode allocated so decrement free inodes
			return i+1;
		}
	}

	return 0; //no space
}

// deallocate an inode
unsigned long idealloc(int dev, unsigned long ino)
{
	int i ;
	char buf[BLOCK_SIZE];

	// get inode bitmap block
	get_block(dev, IBITMAP, buf);
	CLR_bit(buf,ino-1);

	// write buf back
	put_block(dev, IBITMAP,buf);

	// update free inode count in SUPER and GD
	incFreeInodes(dev);
}

// allocate disk block
unsigned long balloc(int dev)
{
	int i;
	char buf[BLOCK_SIZE];
	int nblocks;
	SUPER *sblock;

	//load our super block
	get_block(dev,SUPERBLOCK,buf);
	sblock = (SUPER *)buf;
	//get the number of blocks from super
	nblocks = sblock->s_blocks_count;
	put_block(dev,SUPERBLOCK,buf);//just for safety

	//load our bitmap so we can check for free space
	get_block(dev, BBITMAP,buf);

	for(i = 0; i < nblocks ; i++)
	{
		if(TST_bit(buf,i) == 0)//hooray there is room
		{
			SET_bit(buf,i);
			put_block(dev,BBITMAP,buf);//save our changes

			decFreeBlocks(dev);//block was allocated so decrement free blocks
			return i+1;
		}
	}
	return 0;//no room
}

// deallocate a disk block
unsigned long bdealloc(int dev, unsigned long iblock)
{
	int i ;
	char buf[BLOCK_SIZE];

	//get the mitmap block
	get_block(dev, BBITMAP, buf);
	CLR_bit(buf,iblock-1);

	put_block(dev, BBITMAP,buf);//save our changes

	incFreeBlocks(dev);//we deallocated a block so there is one more free block
}

// decrement free blocks on device
void decFreeBlocks(int dev)
{
	char buf[BLOCK_SIZE];
	get_block(dev, SUPERBLOCK, buf);//load the superblock (..)
	sp = (SUPER *)buf;
	sp->s_free_blocks_count--;//one less free block in the super block
	put_block(dev, SUPERBLOCK,buf);//save the changes to free block count

	get_block(dev, GDBLOCK,buf);//load the group descriptor block
	gp = (GD *)buf;
	gp->bg_free_blocks_count--;//one less free block in the group descriptor
	put_block(dev, GDBLOCK,buf);//save our changes
}

// increment free blocks on device
void incFreeBlocks(int dev)
{
	char buf[BLOCK_SIZE];
	get_block(dev, SUPERBLOCK, buf);
	sp = (SUPER *)buf;
	sp->s_free_blocks_count++;
	put_block(dev, SUPERBLOCK,buf);

	get_block(dev, GDBLOCK,buf);
	gp = (GD *)buf;
	gp->bg_free_blocks_count++;
	put_block(dev, GDBLOCK,buf);
}

// increment free inodes on device
void incFreeInodes(int dev)
{
	char buf[BLOCK_SIZE];
	get_block(dev, SUPERBLOCK, buf);
	sp = (SUPER *)buf;
	sp->s_free_inodes_count++;
	put_block(dev, SUPERBLOCK,buf);

	get_block(dev, GDBLOCK,buf);
	gp = (GD *)buf;
	gp->bg_free_inodes_count++;
	put_block(dev, GDBLOCK,buf);
}

// decrement free inodes on device
void decFreeInodes(int dev)
{
	char buf[BLOCK_SIZE];
	get_block(dev, SUPERBLOCK, buf);
	sp = (SUPER *)buf;
	sp->s_free_inodes_count--;
	put_block(dev, SUPERBLOCK,buf);

	get_block(dev, GDBLOCK,buf);
	gp = (GD *)buf;
	gp->bg_free_inodes_count--;
	put_block(dev, GDBLOCK,buf);
}

// test a bits value, 0 or 1
int TST_bit(char *buf, int BIT)
{	
	return buf[BIT / 8] & (1 << (BIT % 8)); 
}

// set a bit to 1
int SET_bit(char *buf, int BIT)
{
	return buf[BIT / 8] |= (1 << (BIT % 8));
}

// set a bit to 0
int CLR_bit(char *buf, int BIT)
{
	return buf[BIT / 8] &= ~(1 << (BIT % 8));
}

// print a bad path error
void patherror(char *cmdtemp)
{
	printf("%s: bad path!\n", cmdtemp);
}