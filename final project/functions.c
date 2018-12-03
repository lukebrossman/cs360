#include "functions.h"
#include "type.h"
#include "util.h"

// initialize
void init(char *devcmd)
{
	int i;
	printf("Initializing...");
	sleepmode = 0;
	// P[0] has uid= 0, gid = 0, cwd = 0
	P[0].uid = 0;
	P[0].gid = 0;
	P[0].cwd = 0;
	// P[1] has uid=1, gid = 0, cwd = 0
	P[1].uid = 1;
	P[1].gid = 0;
	P[1].cwd = 0;
	// initialize the minode table with 0 references for each minode
	for(i = 0; i < NMINODES; i++)
	{
		minode[i].refCount = 0;
	}
	//set root device = 0
	root = 0;
	// mount root device
	mount_root(devcmd);
	//we initialize with p[0] as running process and p[1] as ready
	running = &P[0];
	readQueue = &P[1];
}

// mount (and verify) the root device
void mount_root(char *devcmd)
{
	char buf[BLOCK_SIZE];
	int dev;
	char devicename[128];
	int magic, nblocks, bfree, ninodes, ifree;

	printf("Mounting root device...");

	// was a device passed as an argument or do we need one?
	if (!strcmp(devcmd, ""))
	{
		printf("This program needs a device to mount: ");
		fgets(devicename, 128, stdin);
		devicename[strlen(devicename) - 1] = 0;
	}
	else
	{
		strcpy(devicename, devcmd);
	}
	dev = open(devicename, O_RDWR); // open device for read/write
	if(dev < 0) //dev was unable to open for some reason
	{
		printf("Cannot open %s\n", devicename);
		exit(1);
	}
	// read device and check some info
	get_block(dev, SUPERBLOCK, buf);
	sp = (SUPER *)buf;
	magic = sp->s_magic;
	nblocks = sp->s_blocks_count;
	bfree = sp->s_free_blocks_count;
	ninodes = sp->s_inodes_count;
	ifree = sp->s_free_inodes_count;

	if (magic != 0xEF53 && magic != 0xEF51)
	{
		printf("ERROR: Non ext2 file system.\n", dev);
		exit(1);
	}
	else
	{
		printf("Getting device info...");
		get_block(dev, GDBLOCK, buf);
		gp = (GD *)buf;
		root = iget(dev, ROOT_INODE);
		root->mountptr = (MOUNT *)malloc(sizeof(MOUNT));
		root->mountptr->ninodes = ninodes;
		root->mountptr->nblocks = nblocks;
		root->mountptr->dev = dev;
		root->mountptr->busy = 1;
		root->mountptr->mounted_inode = root;
		strcpy(root->mountptr->mount_name, devicename);
		strcpy(root->mountptr->name, "/");
		root->mounted = 1;
		printf("Mounted root...");
		P[0].cwd = iget(dev, ROOT_INODE);
		P[1].cwd = iget(dev, ROOT_INODE);
		root->refCount = 3;
	}
	printf("Program loaded\n");
}

// find and execute the inputted command
void find_and_execute_command(char *tempcmd)
{
	int i, r;
	for (i = 0; cmdtable[i].functionName; i++)
	{
		if (!strcmp(tempcmd, cmdtable[i].functionName))
		{
			printf("==========");
			printf("%s", cmdtable[i].functionName);
			printf("==========\n");
			printf("Command Arguments: %s %s", pathname, parameter);
			r = cmdtable[i].f();
			if (r != 0)
			{
				printf("Error executing command: ");
			}
			else
			{
				printf("Successfully executed command: ");
			}
			printf("%s\n", cmdtable[i].functionName);
			printf("=============================================\n");
			return;
		}
	}
	printf("Invalid command\n");
}


//display the suppoted commands
int CMD_MENU()
{
	int i;
	for (i = 0; cmdtable[i].functionName; i++)
	{
		printf(" %s | ", cmdtable[i].functionName);
	}
	printf("\n");
	return 0;
}

// cleanup minodes and exit program
void CMD_QUIT()
{
	int i;
	printf("Shutting down program...Cleaning up minodes");
	// cycle through all minodes and if they are loaded still, save it
	for(i = 0; i < NMINODES; i++)
	{
		printf(".");
		while(minode[i].refCount)
			iput(&minode[i]);
	}
	printf("Cleanup complete...Shutting down\n");
	exit(1);
}

// change the current working directory (cwd)
int CMD_CD()
{
	unsigned long ino;
	int device;
	MINODE *mp;

	// did the user not input any arguments?
	if(pathname[0] == 0)
	{
		// if not, change directory to the root directory
		iput(running->cwd);
		running->cwd = root;
		root->refCount++;
		return 0;
	}

	// get inode of pathname into a minode
	device = running->cwd->dev;
	ino = getino(&device, pathname);
	// directory dne
	if(!ino)
		return 1;
	// otherwise...
	mp = iget(device, ino);

	// Check DIR type
	if(((mp->INODE.i_mode) & 0040000) != 0040000)
	{
		// if not dir...
		printf("%s is not a directory\n", pathname);
		iput(mp);
		return 1;
	}

	// dispose of original CWD
	iput(running->cwd);

	running->cwd = mp; // change running->cwd to point at this minode in memory.
	return 0; // cd was good
}

// list files (either single or in a directory)
int CMD_LS()
{
	return do_ls(pathname);
}

// make a new directory
int CMD_MKDIR()
{
	int ino, r, dev;
	MINODE *pip;
	char *parent, *child;
	
	if (pathname[0] == 0)
	{
		printf("No pathname for a new directory given\n");
		return 1;
	}

	// absolute vs. relative path checking
	if(pathname[0] == '/')
	{
		dev = root->dev;
	}
	else
	{
		dev = running->cwd->dev;
	}
	// find the in-memory minode of the parent
	if(findparent(pathname)) // root inode
	{  
		parent = dirname(pathname);
		child = basename(pathname);
		ino = getino(&dev, parent);

		if(!ino)
		{
			return 1;
		}
		pip = iget(dev, ino);
	}
	else
	{
		pip = iget(running->cwd->dev,running->cwd->ino);
		printf("%d : dev, %d: ino \n");
		child = (char *)malloc((strlen(pathname) + 1) * sizeof(char));
		strcpy(child, pathname);
	}

	// verify INODE is a DIR
	if(!(is_dir(pip))) 
	{
		printf("%s is not a directory.\n", parent);
		iput(pip);
		return 1;
	}
	// verify child does not exist in the parent directory
	if(search(pip,child))
	{
		printf("%s already exists.\n", child);
		iput(pip);
		return 1;
	}

	// create the child
	r = my_mkdir(pip, child);

	return r;
}

// create a new file
int CMD_CREAT()
{
	int ino,r,dev;
	MINODE *pip;
	char *parent, *child;
	
	if (pathname[0] == 0)
	{
		printf("Invalid Pathname\n");
		return 1;
	}

	//check if it is absolute path to determine where the inode comes from
	if(pathname[0] == '/')
	{
		dev = root->dev;
	}
	else
	{
		dev = running->cwd->dev;
	}
	// find the in-memory minode of the parent
	if(findparent(pathname)) // root inode
	{  
		parent = dirname(pathname);
		child = basename(pathname);
		ino = getino(&dev, parent);
		if(!ino)
			return 1;
		pip = iget(dev,ino);
	}
	else // other inode
	{
		pip = iget(running->cwd->dev, running->cwd->ino);
		child = (char *)malloc((strlen(pathname) + 1) * sizeof(char));
		strcpy(child, pathname);
	}

	// verify INODE is a DIR
	if((pip->INODE.i_mode & 0040000) != 0040000) 
	{
		printf("%s is not a directory.\n",parent);
		iput(pip);
		return 1;
	}
	// verify child does not exist in the parent directory
	if(search(pip,child))
	{
		printf("%s already exists.\n",child);
		iput(pip);
		return 1;
	}

	// create the file
	r = my_creat(pip, child);

	return r;
}

// remove a directory
int CMD_RMDIR()
{
	int dev, i;
	unsigned long parent, ino;
	char *my_name;
	MINODE *mip, *pip;
	
	if (pathname[0] == 0)
	{
		return 1;
	}

	// absolute vs. relative path stuffs
	if(pathname[0] == '/')
	{
		dev = root->dev;
	}
	else
	{
		dev = running->cwd->dev;
	}
	// get the inumber of the path
	ino = getino(&dev, pathname);

	// path doesn't exist
	if(!ino)
	{
		return 1;
	}
	// get a pointer to its minode[]
	mip = iget(dev,ino);
	
	// check if not DIR
	if(((mip->INODE.i_mode)&0040000) != 0040000)
	{
		printf("invalid pathname.\n");
		iput(mip);
		return 1;
	}

	// check if its not busy
	if(mip->refCount>1)
	{
		printf("Directory is in use.\n");
		iput(mip);
		return 1;
	}

	// check if its not empty
	if(!isEmpty(mip))
	{
		printf("directory is not empty\n");
		iput(mip);
		return 1;
	}

	// deallocate its block and inode
	for(i = 0; i <= 11; i++)
	{
		if(mip->INODE.i_block[i] == 0)
			continue;
		bdealloc(mip->dev, mip->INODE.i_block[i]);
	}
	idealloc(mip->dev,mip->ino);
	iput(mip); // clears mip->refCount = 0

	// get parent dir's inode and minode
	findino(mip,&ino,&parent);
	pip = iget(mip->dev,parent);

	// get the name of the parent
	if(findparent(pathname))
	{
		my_name = basename(pathname);
	}
	else
	{
		my_name = (char *)malloc((strlen(pathname)+1)*sizeof(char));
		strcpy(my_name,pathname);
	}

	// remove child's entry from parent directory
	rm_child(pip,my_name);
	pip->INODE.i_links_count--; // decrement pip's link count
	pip->INODE.i_atime = pip->INODE.i_mtime = time(0L); // touch pips time fields
	pip->dirty = 1; // mark as dirty
	iput(pip); //cleanup
	return 0; // return success :)
}

// force remove a file
int CMD_RM()
{
	return do_unlink(1);
}

// make a hardlink of a file
int CMD_LINK()
{
	char parent[256], child[256], buf[BLOCK_SIZE], buf2[BLOCK_SIZE], *cp;
	unsigned long inumber, oldIno;
	int dev, newDev, i, rec_length, need_length, ideal_length, datab;
	MINODE *mip;
	DIR *dp;
	
	if (pathname[0] == 0)
	{
		return 1;
	}
	if (parameter[0] == 0)
	{
		return 1;
	}

	// absolute vs. relative paths
	if(pathname[0] == '/')
	{
		dev = root->dev;
	}
	else
	{
		dev = running->cwd->dev;
	}
	// get the inumber of the original file, if it exists
	oldIno = getino(&dev, pathname);
	if(!oldIno)
	{
		return 1;
	}
	mip = iget(dev,oldIno);

	// verify the original file is a REG file...DIRs are not invited :(
	if(((mip->INODE.i_mode) & 0100000) != 0100000)  
	{
		printf("%s is not REG file.\n",pathname);
		iput(mip);
		return 1;
	}

	iput(mip);

	// absolute vs. relative stuffs for new file
	if(parameter[0] == '/')
	{
		newDev = root->dev;
	}
	else
	{
		newDev = running->cwd->dev;
	}
	// find the parent inode for the new file
	if(findparent(parameter))
	{
		strcpy(parent, dirname(parameter));
		strcpy(child, basename(parameter));

		inumber = getino(&newDev, parent);

		if(!inumber)
		{
			return 1;
		}

		if(newDev != dev)
		{
			return 1;
		}

		mip = iget(newDev,inumber);

		// verify parent is a DIR
		if(is_dir(mip))  
		{
			printf("%s is not DIR file.\n", parent);
			iput(mip);
			return 1;
		}

		// verify child does not exist in the parent DIR
		if(search(mip, child))
		{
			printf("%s already exists!\n", child);
			iput(mip);
			return 1;
		}
	}
	else
	{
		strcpy(parent, ".");
		strcpy(child, parameter);

		if(running->cwd->dev != dev)
		{
			return -1;
		}

		// verify the file does not already exist
		if(search(running->cwd, child))
		{
			printf("%s already exists.\n", child);
			return -1;
		}
		mip = iget(running->cwd->dev, running->cwd->ino);
	}     

	// add an entry to the parent's data block
	i = 0;
	while(mip->INODE.i_block[i])
		i++;
	i--;  

	get_block(mip->dev, mip->INODE.i_block[i], buf);
	dp = (DIR *)buf;
	cp = buf;
	rec_length = 0;

	// step to the last entry in a data block
	while(dp->rec_len + rec_length < BLOCK_SIZE)
	{
		rec_length += dp->rec_len;
		cp += dp->rec_len;
		dp = (DIR *)cp;
	}

	need_length = 4 * ((8 + strlen(child) + 3) / 4);
	ideal_length = 4 *((8 + dp->name_len + 3) / 4);
	rec_length = dp->rec_len;
	
	// check if it can enter the new entry as the last entry
	if((rec_length - ideal_length) >= need_length)
	{
		// trim the previous entry to its ideal_length
		dp->rec_len = ideal_length;
		cp += dp->rec_len;
		dp = (DIR *)cp;
		dp->rec_len = rec_length - ideal_length;
		dp->name_len = strlen(child);
		strncpy(dp->name, child, dp->name_len);
		dp->inode = oldIno;

		// write the new block back to the disk
		put_block(mip->dev, mip->INODE.i_block[i], buf);
	}
	else
	{
		// otherwise allocate a new data block 
		i++;
		datab = balloc(mip->dev);
		mip->INODE.i_block[i] = datab;
		get_block(mip->dev, datab, buf2);

		// enter the new entry as the first entry 
		dp = (DIR *)buf2;
		dp->rec_len = BLOCK_SIZE;
		dp->name_len = strlen(child);
		strncpy(dp->name, child, dp->name_len);
		dp->inode = oldIno;
		mip->INODE.i_size += BLOCK_SIZE;
		// write the new block back to the disk
		put_block(mip->dev, mip->INODE.i_block[i], buf2);
	}

	mip->INODE.i_atime = time(0L);
	mip->dirty = 1;
	iput(mip);
	mip= iget(newDev, oldIno);
	mip->INODE.i_links_count++; // increment the i_links_count of INODE by 1
	mip->INODE.i_atime = mip->INODE.i_mtime = time(0L);
	mip->dirty = 1;
	iput(mip); // write it back to the disk

	return 0;
}

// unlink (or remove) a file
int CMD_UNLINK()
{
	return do_unlink(0);
}

// create a symbolic link of a file
int CMD_SYMLINK()
{
	unsigned long inumber;
	int dev, i, r;
	MINODE *mip, *pip;
	char *cp, *parent, *child, buf[BLOCK_SIZE];
	DIR *dp;
	
	if (pathname[0] == 0)
	{
		return 1;
	}
	if (parameter[0] == 0)
	{
		return 1;
	}

	// absolute vs. relative path stuffs
	if(pathname[0] == '/')
	{
		dev = root->dev;
	}
	else
	{
		dev = running->cwd->dev;
	}

	// check if old name exists
	inumber = getino(&dev, pathname);
	if(!inumber)
	{
		return 1;
	}
	// get the minode for the old name
	mip = iget(dev, inumber);

	// make sure it is REG file
	if(!(is_reg(mip)) && !(is_dir(mip)))  
	{
		printf("%s is not a REG file or DIR.\n",pathname);
		iput(mip);
		return -1;
	}
	iput(mip);

	// absolute vs. relative path stuff
	if(parameter[0] == '/')
	{
		dev = root->dev;
	}
	else
	{
		dev = running->cwd->dev;
	}

	// find the parent inode for the new file
	if(findparent(parameter))
	{  
		parent = dirname(parameter);
		child = basename(parameter);
		inumber = getino(&dev, parent);

		if(!inumber)
			return 1;

		pip = iget(dev, inumber);
	}
	else
	{
		pip = iget(running->cwd->dev,running->cwd->ino);
		child = (char *)malloc((strlen(parameter) + 1) * sizeof(char));
		strcpy(child, parameter);
		parent = (char *)malloc(2 * sizeof(char));
		strcpy(parent,".");
	}

	// verify that the parent is, in fact, a DIR
	if(!(is_dir(pip))) 
	{
		printf("%s is not a directory.\n", parent);
		iput(pip);
		return 1;
	}

	// verify that the new file doesn't already exist
	if(search(pip, child))
	{
		printf("%s already exists.\n", child);
		iput(pip);
		return 1;
	}

	r = my_creat(pip, child);

	// get the new path into memory
	inumber = getino(&dev, parameter);

	if(!inumber)
	{
		return 1;
	}

	mip = iget(dev, inumber);
	mip->INODE.i_mode = 0xA1FF; // change the mode of the new file to a symlink
	strcpy((char *)(mip->INODE.i_block), pathname);
	mip->INODE.i_size = strlen(pathname);
	mip->dirty = 1;
	iput(mip);
	
	return r;
}

// touch a file
int CMD_TOUCH()
{
	int dev, inumber;
	MINODE *mip;
	
	if (pathname[0] == 0)
	{
		return 1;
	}

	dev = root->dev;
	inumber = getino(&dev, pathname);

	if(!inumber)
	{
		return 1;
	}
	// load the file into memory
	mip = iget(dev, inumber);

	// adjust its timestamps
	mip->INODE.i_atime = mip->INODE.i_mtime = time(0L);
	mip->dirty = 1;

	iput(mip);

	return 0;
}

// change a file's permissions
int CMD_CHMOD()
{
	int mode, dev, inumber;
	MINODE *mip;
	
	if (pathname[0] == 0)
	{
		return 1;
	}
	if (parameter[0] == 0)
	{
		return 1;
	}

	sscanf(parameter, "%x", &mode);
	dev = root->dev;
	inumber = getino(&dev, pathname);

	if(!inumber)
	{
		return 1;
	}
	// load the file into memory
	mip = iget(dev, inumber);

	// change its permissions accordingly to those the user desires
	if(is_reg(mip)) 
	{
		mip->INODE.i_mode = 0100000 + mode;
	}
	else if (is_dir(mip))
	{
		mip->INODE.i_mode = 0040000 + mode;
	}
	else
	{
		mip->INODE.i_mode = 0120000 + mode;
	}

	// mark dirty
	mip->dirty = 1;

	iput(mip); // cleanup
	return 0;
}

// print out information about a file
int CMD_STAT()
{
	struct stat mystat;
	
	if (pathname[0] == 0)
	{
		return 1;
	}
	return do_stat(pathname, &mystat);
}

// print the current working directory (cwd)
int CMD_PWD()
{
	if(running->cwd == root)
	{
		printf("/");
	}
	else
	{
		do_pwd(running->cwd);
	}

	printf("\n");
	return 0;
}

/* helper functions */
// cycle through the specified pathname and list all files in it (or the single function)
int do_ls(char *path)
{
	unsigned long ino;
	MINODE *mip, *pip;
	int device = running->cwd->dev;
	char *child;

	if(path[0] == 0)// no parameters, so print cwd info
	{
		mip = iget(device, running->cwd->ino);
		printChild(device, mip);
	}
	else
	{
		if(path[0] == '/')//change device to the root so we can print
		{
			device = root->dev;
		}
		// get the inode for the current path part
		ino = getino(&device, path);

		if(!ino)
		{
			return 1;//bad path
		}

		mip = iget(device, ino);

		// if we're not looking at a directory...
		if(!(is_dir(mip)))
		{
			// find the parent of the path part and get its's basename
			if(findparent(path))
			{
				child = basename(pathname);
			}
			// if it doesn't exist, get the name of the child path
			else
			{
				child = (char *)malloc((strlen(pathname) + 1) * sizeof(char));
				strcpy(child, path);
			}
			// print the child path
			printFile(mip, child);
			return 0;
		}

		// print child part
		printChild(device, mip);
	}

	iput(mip);
	return 0;
}

// print information on a file
void printFile(MINODE *mip, char *namebuf)
{
	char *Time;
	unsigned short mode;
	int type;

	mode = mip->INODE.i_mode;
	// print out file info
	// print the file type
	if(is_link(mip))
	{
		printf("l");
		type = LINK;
	}
	else if(is_dir(mip))
	{
		printf("d");
		type = DIRECTORY;
	}
	else if(is_reg(mip))
	{
		printf("-");
		type = FILE;
	}

	//for the file permissions and file type
	//info adapted from labs and pg 254-256 in the textbook
	if((mode & (1 << 8)))
		printf("r");
	else
		printf("-");
	if((mode & (1 << 7)) )
		printf("w");
	else
		printf("-");
	if((mode & (1 << 6)) )
		printf("x");
	else
		printf("-");

	if((mode & (1 << 5)) )
		printf("r");
	else
		printf("-");
	if((mode & ( 1 << 4)) )
		printf("w");
	else
		printf("-");
	if((mode & (1 << 3)) )
		printf("x");
	else
		printf("-");

	if((mode & (1 << 2)) )
		printf("r");
	else
		printf("-");
	if((mode & (1 << 1)) )
		printf("w");
	else
		printf("-");
	if(mode & 1)
		printf("x");
	else
		printf("-");

	// print the file info from the inode
	printf(" %d %d %d %d", mip->INODE.i_links_count, mip->INODE.i_uid,mip->INODE.i_gid, mip->INODE.i_size);
	Time = ctime(&(mip->INODE.i_mtime));
	Time[strlen(Time) -1 ] = 0;
	printf(" %s ", Time);

	if(is_link(mip))
	{
		printf(" => %s\n",(char *)(mip->INODE.i_block));//file that the link points to
	}
	else
	{
		printf("\n");
	}
	iput(mip);
} 

// print contents of a directory
void printChild(int devicename, MINODE *mp)
{
	char buf[BLOCK_SIZE], filename[256], *cp;
	DIR *dp;
	int i, ino;
	MINODE *currentMinode;

	// cycle through 12 direct blocks and print them
	for(i = 0; i <= 11; i++)
	{
		if(mp->INODE.i_block[i])
		{
			get_block(devicename, mp->INODE.i_block[i], buf);
			cp = buf;
			dp = (DIR *)buf;

			while(cp < &buf[BLOCK_SIZE])
			{
				strncpy(filename, dp->name, dp->name_len);
				filename[dp->name_len] = 0;

				ino = dp->inode;
				currentMinode = iget(devicename, ino); 
				printFile(currentMinode, filename);		
				cp+=dp->rec_len;
				dp = (DIR *)cp;
			}
		}
	}
}

// find the parentdir of the basename in the given path
int findparent(char *pathname)
{
	int i = 0;
	while(i < strlen(pathname))
	{
		if(pathname[i] == '/')
		{
			return 1;
		}
		i++;
	}
	return 0;
}


// actually make a directory on the device
int my_mkdir(MINODE *pip, char *name)
{
	unsigned long inumber, bnumber;
	int i, datab, need_length, ideal_length, rec_length;
	char buf[BLOCK_SIZE], buf2[BLOCK_SIZE], *cp;
	DIR *dp;
	MINODE *mip;

	// allocate an inode and a disk block for the new directory
	inumber = ialloc(pip->dev);
	bnumber = balloc(pip->dev);

	mip = iget(pip->dev, inumber); // load the inode into a minode so we can write it to the disk

	// setup information for the new directory
	mip->INODE.i_mode = 0x41ED; // DIR and permissions
	mip->INODE.i_uid = running->uid; //set the owner to be the running proc uid
	mip->INODE.i_gid = running->gid; // group id
	mip->INODE.i_size = BLOCK_SIZE; // size in bytes
	mip->INODE.i_links_count = 2; //2 links for . and ..
	mip->INODE.i_atime = mip->INODE.i_ctime = mip->INODE.i_mtime = time(0L); // creation time
	mip->INODE.i_blocks = 2; // blocks count in 512-byte blocks
	mip->dirty = 1; // mark as dirty so that it is saved

	for(i = 1; i < 15; i++) // clear the data blocks
		mip->INODE.i_block[i] = 0;
	mip->INODE.i_block[0] = bnumber; // set block 0 to the allocated bnumber

	iput(mip); // cleanup

	// write the . and .. entries into a buf[] of BLOCK_SIZE
	memset(buf, 0, BLOCK_SIZE);
	dp = (DIR *)buf;
	
	dp->inode = inumber; //inode number of this directory
	strncpy(dp->name, ".", 1);
	dp->name_len = 1;
	dp->rec_len = 12; //directory entry length

	cp = buf + dp->rec_len;
	dp = (DIR *)cp;

	dp->inode = pip->ino; //inode number of parent directory
	dp->name_len = 2; 
	strncpy(dp->name, "..", 2);
	dp->rec_len = BLOCK_SIZE - 12; //record length is the rest of the block because no child directories yet

	put_block(pip->dev, bnumber, buf); // save block to disk

	// Finally enter name into parent's directory...
	// get iblock count
	i = 0;
	while(pip->INODE.i_block[i])
	{
		i++;
	}
	i--;//the last block is this one, -1 to get parent

	get_block(pip->dev, pip->INODE.i_block[i], buf);
	dp = (DIR *)buf;
	cp = buf;
	rec_length = 0;

	// step to the last entry in a data block
	while(dp->rec_len + rec_length < BLOCK_SIZE)
	{
		rec_length += dp->rec_len;
		cp += dp->rec_len;
		dp = (DIR *)cp;
	}

	// when entering a new entry with name_len = n
	need_length = 4 * ((8 + strlen(name) + 3) / 4); // a multiple of 4
	// step to the last entry in a data block...its ideal length is...
	ideal_length = 4 *((8 + dp->name_len + 3) / 4);
	rec_length = dp->rec_len; // store rec_len for a bit easier code writing

	// check if it can enter the new entry as the last entry
	if((rec_length - ideal_length) >= need_length)
	{
		// trim the previous entry to its ideal_length
		dp->rec_len = ideal_length;
		cp+=dp->rec_len;
		dp = (DIR *)cp;
		dp->rec_len = rec_length - ideal_length;
		dp->name_len = strlen(name);
		strncpy(dp->name, name, dp->name_len);
		dp->inode = inumber;

		// write the new block back to the disk
		put_block(pip->dev, pip->INODE.i_block[i], buf);
	}
	else
	{
		// otherwise allocate a new data block 
		i++;
		datab = balloc(pip->dev);
		pip->INODE.i_block[i] = datab;
		get_block(pip->dev, datab, buf2);

		// enter the new entry as the first entry in the new block
		dp = (DIR *)buf2;
		dp->rec_len = BLOCK_SIZE;
		dp->name_len = strlen(name);
		strncpy(dp->name, name, dp->name_len);
		dp->inode = inumber;

		pip->INODE.i_size += BLOCK_SIZE;
		
		// write the new block back to the disk
		put_block(pip->dev, pip->INODE.i_block[i], buf2);
	}

	// inc parent inode's link count by 1, touch its atime, and mark it dirty
	pip->INODE.i_links_count++;
	pip->INODE.i_atime = time(0L);
	pip->dirty = 1;//mark as dirty so that it will be saved
	iput(pip); //save the parent directory
	return 0;
}

// create a file
int my_creat(MINODE *pip, char *name)
{
	unsigned long inumber;
	int i = 0, datab, need_length, ideal_length, rec_length;
	char buf[BLOCK_SIZE], buf2[BLOCK_SIZE], *cp;
	DIR *dp;
	MINODE *mip;

	// allocate an inode for the new file
	inumber = ialloc(pip->dev);

	mip = iget(pip->dev, inumber);

	// setup information for the new file
	mip->INODE.i_mode = 0x81A4; // REG and permissions
	mip->INODE.i_uid = running->uid; // owner uid
	mip->INODE.i_gid = running->gid; // group id
	mip->INODE.i_size = 0; // size in bytes
	mip->INODE.i_links_count = 1; // links count
	mip->INODE.i_atime = mip->INODE.i_ctime = mip->INODE.i_mtime = time(0L); // creation time
	mip->INODE.i_blocks = 2;    /* Blocks count in 512-byte blocks */
	mip->dirty = 1; // mark dirty

	for(i = 0; i < 15; i++) // setup data blocks
		mip->INODE.i_block[i] = 0;

	iput(mip);

	//Finally enter name into parent's directory, assume all direct data blocks
	i = 0;
	while(pip->INODE.i_block[i])
	{
		i++;
	}
	i--;  

	get_block(pip->dev, pip->INODE.i_block[i], buf);
	dp = (DIR *)buf;
	cp = buf;
	rec_length = 0;

	// step to the last entry in a data block
	while(dp->rec_len + rec_length < BLOCK_SIZE)
	{
		rec_length += dp->rec_len;
		cp += dp->rec_len;
		dp = (DIR *)cp;
	}

	// when entering a new entry with name_len = n
	need_length = 4 * ((8 + strlen(name) + 3) / 4); // a multiple of 4
	// step to the last entry in a datablock, it's ideal length is...
	ideal_length = 4 * ((8 + dp->name_len + 3) / 4);
	rec_length = dp->rec_len; // set rec_len for a bit easier programming

	// check if it can enter the new entry as the last entry
	if((rec_length - ideal_length) >=need_length)
	{
		// trim the previous entry to its ideal_length
		dp->rec_len = ideal_length;
		cp += dp->rec_len;
		dp = (DIR *)cp;
		dp->rec_len = rec_length - ideal_length;
		dp->name_len = strlen(name);
		strncpy(dp->name, name, dp->name_len);
		dp->inode = inumber;

		// write the new block back to the disk
		put_block(pip->dev, pip->INODE.i_block[i], buf);
	}
	else
	{
		// otherwise allocate a new data block 
		i++;
		datab = balloc(pip->dev);
		pip->INODE.i_block[i] = datab;
		get_block(pip->dev, datab, buf2);

		pip->INODE.i_size += BLOCK_SIZE;
		// enter the new entry as the first entry 
		dp = (DIR *)buf2;
		dp->rec_len = BLOCK_SIZE;
		dp->name_len = strlen(name);
		strncpy(dp->name, name, dp->name_len);
		dp->inode = inumber;

		// write the new block back to the disk
		put_block(pip->dev, pip->INODE.i_block[i], buf2);
	}

	// touch parent's atime and mark it dirty
	pip->INODE.i_atime = time(0L);
	pip->dirty = 1;
	iput(pip);

	return 0;
}

// checks if a minode is empty or not.
int isEmpty(MINODE *mip)
{
	int i;
	char buf[BLOCK_SIZE], namebuf[256], *cp;

	// more than 2 links?
	if(mip->INODE.i_links_count > 2)
		return 0;

	// only 2 links?
	if(mip->INODE.i_links_count == 2)
	{
		// cycle through each direct block to check...
		for(i = 0; i <= 11; i++)
		{
			if(mip->INODE.i_block[i])
			{
				get_block(mip->dev, mip->INODE.i_block[i], buf); 
				cp = buf;
				dp = (DIR *)buf;

				while(cp < &buf[BLOCK_SIZE])
				{
					strncpy(namebuf, dp->name, dp->name_len);
					namebuf[dp->name_len] = 0;

					// if stuff exists, this directory isn't empty :(
					if(strcmp(namebuf, ".") && strcmp(namebuf, ".."))
						return 0;

					cp+=dp->rec_len;
					dp=(DIR *)cp;
				}
			}
		}
		return 1;
	}
	return -1;
}

// removes the entry from the parent's data block
int rm_child(MINODE *parent, char *my_name)
{
	int i, j, total_length, next_length, removed_length, previous_length;
	DIR *dNext;
	char buf[BLOCK_SIZE], namebuf[256], temp[BLOCK_SIZE], *cp, *cNext;

	// search parent inode's data blocks for the entry of my_name
	for(i = 0; i <= 11; i++)
	{
		if(parent->INODE.i_block[i])
		{
			get_block(parent->dev, parent->INODE.i_block[i], buf);
			dp = (DIR *)buf;
			cp = buf;
			j = 0;
			total_length = 0;
			while(cp < &buf[BLOCK_SIZE])
			{
				strncpy(namebuf, dp->name, dp->name_len);
				namebuf[dp->name_len] = 0;
				total_length += dp->rec_len;

				// found my_name? then we must erase it! no one must ever know!
				if(!strcmp(namebuf, my_name))
				{
					// if not first entry in data block
					if(j)
					{
						// if my_name is the last entry in the data block...
						if(total_length == BLOCK_SIZE)
						{
							removed_length = dp->rec_len;
							cp -= previous_length;
							dp =(DIR *)cp;
							dp->rec_len += removed_length;
							put_block(parent->dev, parent->INODE.i_block[i], buf);
							parent->dirty = 1;
							return 0;
						}

						// otherwise, we must move all entries after this one left
						removed_length = dp->rec_len;
						cNext = cp + dp->rec_len;
						dNext = (DIR *)cNext;
						while(total_length + dNext->rec_len < BLOCK_SIZE)
						{
							total_length += dNext->rec_len;
							next_length = dNext->rec_len;
							dp->inode = dNext->inode;
							dp->rec_len = dNext->rec_len;
							dp->name_len = dNext->name_len;
							strncpy(dp->name, dNext->name, dNext->name_len);
							cNext += next_length;
							dNext = (DIR *)cNext;
							cp+= next_length;
							dp = (DIR *)cp;
						}
						dp->inode = dNext->inode;
						// add removed rec_len to the last entry of the block
						dp->rec_len = dNext->rec_len + removed_length;
						dp->name_len = dNext->name_len;
						strncpy(dp->name, dNext->name, dNext->name_len);
						put_block(parent->dev, parent->INODE.i_block[i], buf); // save
						parent->dirty = 1;
						return 0;
					}
					// if first entry in a data block
					else
					{
						// deallocate the data block and modify the parent's file size
						bdealloc(parent->dev, parent->INODE.i_block[i]);
						memset(temp, 0, BLOCK_SIZE);
						put_block(parent->dev, parent->INODE.i_block[i], temp);
						parent->INODE.i_size -= BLOCK_SIZE;
						parent->INODE.i_block[i] = 0;
						parent->dirty = 1;
						return 0;
					}
				}
				j++;
				previous_length = dp->rec_len;
				cp+=dp->rec_len;
				dp = (DIR *)cp;
			}
		}
	}
	return 1;
}

// recursively print the cwd
void do_pwd(MINODE *wd)
{
	struct DIR *dp;
	char myname[256];
	unsigned long myino, parentino;
	int f;

	if(wd == root) // if we're at the root, then exit function
	{
		return;
	}
	findino(wd, &myino, &parentino); // find the inode of the parent
	wd = iget(wd->dev, parentino);
	do_pwd(wd); // print the parent's path
	searchname(wd, myino, myname); // find this inode's name
	printf("/%s", myname); // print this inode's name
	iput(wd); // cleanup
}

// actually get the information on the file
int do_stat(char *path, struct stat *stPtr)
{
	unsigned long ino;
	MINODE *mip;
	int device = running->cwd->dev;

	// check absolute vs. relative path names and modify accordingly
	if(path[0] == 0)
		ino = running->cwd->ino;
	else
	{
		if(path[0] == '/')
			device = root->dev;
		ino = getino(&device, path);
	}

	// no such path in the stat
	if(!ino)
		return 1;
		
	// get pointer to the specified minode
	mip = iget(device, ino);

	// copy entries of INODE into stat struct
	stPtr->st_dev = device;
	stPtr->st_ino = ino;
	stPtr->st_mode = mip->INODE.i_mode;
	stPtr->st_uid = mip->INODE.i_uid;
	stPtr->st_size = mip->INODE.i_size;
	stPtr->st_blksize = BLOCK_SIZE;
	stPtr->st_atime = mip->INODE.i_atime;
	stPtr->st_ctime = mip->INODE.i_ctime;
	stPtr->st_mtime = mip->INODE.i_mtime;
	stPtr->st_gid = mip->INODE.i_gid;
	stPtr->st_nlink = mip->INODE.i_links_count;
	stPtr->st_blocks = mip->INODE.i_blocks;

	// print the entries of the stat struct
	printf("dev=%d   ", stPtr->st_dev);
	printf("ino=%d   ", stPtr->st_ino);
	printf("mode=%4x\n", stPtr->st_mode);
	printf("uid=%d   ", stPtr->st_uid);
	printf("gid=%d   ", stPtr->st_gid);
	printf("nlink=%d\n", stPtr->st_nlink);
	printf("size=%d ", stPtr->st_size);
	printf("atime=%s", ctime(&(stPtr->st_atime)));
	printf("mtime=%s", ctime(&(stPtr->st_mtime)));
	printf("ctime=%s", ctime(&(stPtr->st_ctime)));

	iput(mip); // cleanup
	return 0;
}

// unlink or remove a file
int do_unlink(int forcerm)
{
	MINODE *mip;
	int dev, i, m, l, *k, *j, *t, block[15], symfile = 0;
	char path[256], parent[256], child[256], buf[BLOCK_SIZE], buf2[BLOCK_SIZE];
	unsigned long inumber;
	
	if (pathname[0] == 0)
	{
		printf("No pathname for a directory to remove given!\n");
		return 1;
	}

	// absolute vs. relative paths
	if(pathname[0] == '/')
	{
		dev = root->dev;
	}
	else
	{
		dev = running->cwd->dev;
	}

	// get the path's inumber
	inumber = getino(&dev, pathname);
	if(!inumber)
		return 1;

	// load the path into memory
	mip = iget(dev, inumber);

	// verify that we're dealing with a file, not a DIR
	if(is_dir(mip))
	{
		printf("%s is not a REG file.\n", pathname);
		iput(mip);
		return 1;
	}
	
	// check if its a symlink (so we can make some special exceptions on what we're doing during deallocation)
	if(((mip->INODE.i_mode) & 0xA1FF) == 0xA1FF)
	{
		symfile = 1;
		iput(mip);
	}

	// decrement INODE's i_links_count by 1
	mip->INODE.i_links_count--;

	// if i_links_count == 0 ==> remove the file
	if(mip->INODE.i_links_count == 0 || forcerm == 1)
	{
		// setup the data blocks so we can deallocate them
		for(i = 0; i < 15; i++)
			block[i] = mip->INODE.i_block[i];

		// if we're *not* dealing with a symlink file, then we need to deallocate inode blocks
		if (!symfile)
		{
			// deallocate the direct blocks
			for(i = 0; i <= 11; i++)
			{
				if(block[i])
					bdealloc(mip->dev, block[i]);
			}

			// deallocate the indirect blocks
			if(block[12])
			{
				get_block(mip->dev, block[12], buf);
				k = (int *)buf;
				for(i = 0; i < 256; i++)
				{
					if(*k)
						bdealloc(mip->dev, *k);
					k++;
				}
			}

			// deallocate the double indirect blocks
			if(block[13])
			{
				get_block(mip->dev, block[13], buf);
				t = (int *)buf;
				for(i = 0; i < 256 ; i++)
				{
					if(*t)
					{
						get_block(mip->dev, *t, buf2);
						j = (int *)buf2;
						for(m = 0; m < 256; m++)
						{
							if(*j)
								bdealloc(mip->dev, *j);
							j++;
						}
					}
					t++;
				}
			}
		}

		// deallocate its INODE
		idealloc(mip->dev, mip->ino);
	}
	mip->dirty = 1;
	iput(mip);

	// get the parent minode so we can remove it from the parent directory
	if(findparent(pathname))
	{
		strcpy(parent, dirname(pathname));
		strcpy(child, basename(pathname));
		inumber = getino(&dev, parent);
		mip = iget(dev, inumber);
	}
	else
	{
		strcpy(parent, ".");
		strcpy(child, pathname);
		mip = iget(running->cwd->dev, running->cwd->ino);
	}
 
	rm_child(mip, child); // remove the file from the parent directory
	mip->INODE.i_atime = mip->INODE.i_mtime = time(0L);
	mip->dirty = 1;
	iput(mip);
	
	return 0;
}

//helper function returns 1 if minode given is dir 0 if not
int is_dir(MINODE *mip)
{
	if((mip->INODE.i_mode & 0040000) == 0040000)
	{
		return 1;
	}
	return 0;
}

//helper checks if regular file
int is_reg(MINODE *mip)
{
	if((mip->INODE.i_mode & 0100000) == 0100000)
	{
		return 1;
	}
	return 0;
}

//helper checks if symlink
int is_link(MINODE *mip)
{
	if((mip->INODE.i_mode & 0120000) == 0120000)
	{
		return 1;
	}
	return 0;
}