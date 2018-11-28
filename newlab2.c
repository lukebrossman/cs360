#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libgen.h>
#include <sys/cdefs.h>

#define FALSE 0
#define TRUE 1

typedef struct node
{
	char name[64];
	char type;
	struct node *childPtr, *siblingPtr, *parentPtr;
}NODE;

NODE *root, *cwd;
char line[128], command[16], pathname[64], dname[64], bname[64], savepath[128];

// List of shell directory options.
int menu(char *pathname);						
int mkdir(char *pathname);						
int rmdir(char *pathname);						
int ls(char *pathname);					
int cd(char *pathname);						
int pwd(char *pathname);						
int creat(char *pathname);						
int rmv(char *pathname);						
int save(char *pathname);					
int reload(char *pathname);
int quit(char *pathname);						
int menu(char *pathname);						

//helper
int findCmd(char *command);						
int initialize(void);							
int isDir(NODE *temp);							
NODE *makedNode(NODE *Parent, char *pname);			
NODE *makefNode(NODE *Parent, char *pname);				
int isEmpty(NODE *rmvNode);						
int rpwd(NODE *p);							
int savefile(FILE *outfile, NODE *p);					
int resave(FILE *outfile, NODE *p);					


char *cmd[] = { "mkdir", "rmdir", "ls", "cd", "pwd", "creat", "rm", "quit", "menu", "reload", "save", 0 };

int initialize()
{
	root = (NODE *)malloc(sizeof(NODE));

	if (root != NULL)
	{
		root->name[0] = '/';
		root->name[1] = '\0';

		root->type = 'D';

		root->childPtr = NULL;
		root->siblingPtr = NULL;
		root->parentPtr = root;

		cwd = root;
		return 1;
	}
	else
	{
		printf("Unable to allocate memory for root. Exiting program\n");
		return -1;
	}
}

int findCmd(char *command)
{
	if (command[0] == '\0')
	{
		return -1;
	}
	else
	{
		int i = 0;
		while (cmd[i])
		{
			if (strcmp(command, cmd[i]) == 0)
			{
				return i;
			}

			i++;
		}

		return -1;
	}
}


int menu(char *pathname)
{
	printf("______________________________________________\n");
	printf(" mkdir   : This will make a new directory \n");
	printf(" remdir  : This will remove the directory \n");
	printf(" ls      : This will list the directory's contents \n");
	printf(" cd      : This will change to a different directory \n");
	printf(" pwd     : This will display the name of the current directory \n");
	printf(" rm      : This deletes a file \n");
	printf(" quit    : Saves file and exits program \n");
	printf(" help    : Displays the menu choices \n");
	printf(" menu    : Displays the menu \n");
	printf(" reload : Displays the file previous file system \n");
	printf(" save   : Saves the current file system \n");

	return TRUE;
}

int isDir(NODE* temp)
{
	if (!temp)
	{
		printf("Node does not exist\n");
		return FALSE;
	}
	else
	{
		if (temp->type != 'D')
		{
			return FALSE;
		}
		else
		{
			return TRUE;
		}
	}
}

int isEmpty(NODE *rmvNode)
{
	if (rmvNode->childPtr != NULL)
	{
		return FALSE;
	}
	else
	{
		return TRUE;
	}
}

int mkdir(char *pathname)
{
	char temp[128], *str;
	NODE *Parent = NULL, *Child = NULL;
	int flag = FALSE;

	if ((pathname[0] == '/') && (pathname[1] == '\0'))
	{
		printf("Cannot make root directory\n");
		return FALSE;
	}

	strcpy(temp, pathname);
	strcpy(dname, dirname(temp));   // dname="/a/b"

	strcpy(temp, pathname);
	strcpy(bname, basename(temp));   // bname="c"

	str = strtok(dname, "/");
	// If the first place to add a directory is right off the root or cwd.
	if (str == NULL)
	{
		// If the path is absolute.
		if (pathname[0] == '/')
		{
			NODE *pMem = makedNode(root, bname);

			// If the location to insert is the child location.
			if (root->childPtr == NULL)
			{
				root->childPtr = pMem;
				return TRUE;
			}
			// Otherwise we should insert at the end of our siblings.
			else
			{
				NODE *pTemp = root->childPtr;

				while ((pTemp->siblingPtr != NULL) && (strcmp(pTemp->name, bname) != 0))
				{
					pTemp = pTemp->siblingPtr;
				}

				if ((strcmp(pTemp->name, bname) == 0))
				{
					printf("%s already exists in this location\n", bname);
					free(pMem);
					return FALSE;
				}

				pTemp->siblingPtr = pMem;
				return TRUE;
			}
		}
	}
	// Otherwise it is relative.
	else if ((strcmp(dname, ".\0") == 0))
	{
		NODE *pMem = makedNode(cwd, bname);

		if (cwd->childPtr == NULL)
		{
			cwd->childPtr = pMem;
		}
		else
		{
			NODE *pTemp = cwd->childPtr;

			while ((pTemp->siblingPtr != NULL) && (strcmp(pTemp->name, bname) != 0))
                        {
                        	pTemp = pTemp->siblingPtr;
                        }

                        if ((strcmp(pTemp->name, bname) == 0))
                        {
                        	printf("%s already exists in this location\n", bname);
				free(pMem);
                        	return FALSE;
                        }

			pTemp->siblingPtr = pMem;
                        return TRUE;

		}
	}
	else
	{
		// If the path starts absolute.
		if (pathname[0] == '/')
		{
			Parent = root;

			while ((str) && (strcmp(str, bname) != 0))
			{
				Child = Parent->childPtr;

				if ((Child != NULL) && (strcmp(Child->name, str) != 0) && (Child->siblingPtr == NULL))
				{
					// error message and return.
					printf("%s: No such directory exists\n", str);
					return FALSE;
				}
				else if ((Child != NULL) && (strcmp(Child->name, str) == 0) && (!isDir(Child)))
				{
					// error message and return.
					printf("Node %s, is not a directory\n", Child->name);
					return FALSE;
				}
				else if ((Child != NULL) && (strcmp(Child->name, str) == 0) && (isDir(Child)))
				{
					// Found the node move to it and continue.
					Parent = Child;
					flag = TRUE;
				}
				else if ((Child != NULL) && (strcmp(Child->name, str) != 0) && (Child->siblingPtr != NULL))
				{
					// Traverse the list to find the correct sibling.
					while ((Child->siblingPtr != NULL) && (strcmp(Child->name, str) != 0))
					{
						Child = Child->siblingPtr;
					}

					if ((strcmp(Child->name, str) == 0) && (isDir(Child)))
					{
						// Found the node move to it and continue.
						Parent = Child;
						flag = TRUE;
					}
					else if ((Child->siblingPtr == NULL) && (strcmp(Child->name, str) != 0))
					{
						// error message and return.
						printf("Node does not exist in directory\n");
						return FALSE;
					}
					else if ((strcmp(Child->name, str) == 0) && (!isDir(Child)))
					{
						// error message and return.
						printf("Node %s, is not a directory\n", Child->name);
						return FALSE;
					}
					else
					{
						// odd error message and return.
						printf("Something unexpected occurred\n");
						return FALSE;
					}
				}
				else
				{
					// odd error message and return.
					printf("No subdirectory exists\n");
					return FALSE;
				}
				str = strtok(NULL, "/");
			}
		}
		// Otherwise the path starts relative to where we are.
		else
		{
			Parent = cwd;

			while ((str) && strcmp(str, bname) != 0)
			{
				Child = Parent->childPtr;

				if ((Child != NULL) && (strcmp(Child->name, str) != 0) && (Child->siblingPtr == NULL))
				{
					// error message and return.
					printf("%s: No such directory exists\n", str);
					return FALSE;
				}
				else if ((Child != NULL) && (strcmp(Child->name, str) == 0) && (!isDir(Child)))
				{
					// error message and return.
					printf("Node %s, is not a directory\n", Child->name);
					return FALSE;
				}
				else if ((Child != NULL) && (strcmp(Child->name, str) == 0) && (isDir(Child)))
				{
					// Found the node move to it and continue.
					Parent = Child;
					flag = TRUE;
				}
				else if ((Child != NULL) && (strcmp(Child->name, str) != 0) && (Child->siblingPtr != NULL))
				{
					// Traverse the list to find the correct sibling.
					while ((Child->siblingPtr != NULL) && (strcmp(Child->name, str) != 0))
					{
						Child = Child->siblingPtr;
					}

					if ((strcmp(Child->name, str) == 0) && (isDir(Child)))
					{
						// Found the node move to it and continue.
						Parent = Child;
						flag = TRUE;
					}
					else if ((Child->siblingPtr == NULL) && ((strcmp(Child->name, str) != 0)))
					{
						// error message and return.
						printf("Node does not exist in directory\n");
						return FALSE;
					}
					else if ((strcmp(Child->name, str) == 0) && (!isDir(Child)))
					{
						// error message and return.
						printf("Node %s, is not a directory\n", Child->name);
						return FALSE;
					}
					else
					{
						// odd error message and return.
						printf("Something unexpected occurred\n");
						return FALSE;
					}
				}
				else
				{
					// odd error message and return.
					printf("No subdirectory exists\n");
					return FALSE;
				}
				str = strtok(NULL, "/");
			}
		}
	}

	// We are now in the correct location to add.
	if (flag)
	{
		NODE *pMem = makedNode(Parent, bname);
		Child = Parent->childPtr;

		// If there is no child below the parent. Add at the child.
		if (!Child)
		{
			Parent->childPtr = pMem;
			return TRUE;
		}
		// Otherwise add at the end of the sibling location.
		else
		{
			while ((Child->siblingPtr != NULL) && (strcmp(Child->name, bname) != 0))
			{
				Child = Child->siblingPtr;
			}

			if (strcmp(Child->name, bname) == 0)
			{
				printf("%s already exists in directory\n", bname);
				free(pMem);
				return FALSE;
			}

			Child->siblingPtr = pMem;
			return TRUE;
		}
	}

	return TRUE;
}

int creat(char *pathname)
{
	char temp[128], *str;
	NODE *Parent = NULL, *Child = NULL;
	int flag = FALSE;

	strcpy(temp, pathname);
	strcpy(dname, dirname(temp));   // dname="/a/b"

	strcpy(temp, pathname);
	strcpy(bname, basename(temp));   // bname="c"

	str = strtok(dname, "/");
	// If the first place to add a directory is right off the root or cwd.
	if (str == NULL)
	{
		// If the path is absolute.
		if (pathname[0] == '/')
		{
			NODE *pMem = makefNode(root, bname);

			// If the location to insert is the child location.
			if (root->childPtr == NULL)
			{
				root->childPtr = pMem;
				return TRUE;
			}
			// Otherwise we should insert at the end of our siblings.
			else
			{
				NODE *pTemp = root->childPtr;

				while ((pTemp->siblingPtr != NULL) && (strcmp(pTemp->name, bname) != 0))
				{
					pTemp = pTemp->siblingPtr;
				}

				if ((strcmp(pTemp->name, bname) == 0))
				{
					printf("%s already exists in this location\n", bname);
					free(pMem);
					return FALSE;
				}

				pTemp->siblingPtr = pMem;
				return TRUE;
			}
		}
	}
	// Otherwise it is relative.
	else if ((strcmp(dname, ".\0") == 0))
	{
		NODE *pMem = makefNode(cwd, bname);

		if (cwd->childPtr == NULL)
		{
			cwd->childPtr = pMem;
		}
		else
		{
			NODE *pTemp = cwd->childPtr;

			while ((pTemp->siblingPtr != NULL) && (strcmp(pTemp->name, bname) != 0))
                        {
                        	pTemp = pTemp->siblingPtr;
                        }

                        if ((strcmp(pTemp->name, bname) == 0))
                        {
                        	printf("%s already exists in this location\n", bname);
				free(pMem);
                        	return FALSE;
                        }

			pTemp->siblingPtr = pMem;
                        return TRUE;

		}
	}
	else
	{
		// If the path starts absolute.
		if (pathname[0] == '/')
		{
			Parent = root;

			while ((str) && (strcmp(str, bname) != 0))
			{
				Child = Parent->childPtr;

				if ((Child != NULL) && (strcmp(Child->name, str) != 0) && (Child->siblingPtr == NULL))
				{
					// error message and return.
					printf("%s: No such directory exists\n", str);
					return FALSE;
				}
				else if ((Child != NULL) && (strcmp(Child->name, str) == 0) && (!isDir(Child)))
				{
					// error message and return.
					printf("Node %s, is not a directory\n", Child->name);
					return FALSE;
				}
				else if ((Child != NULL) && (strcmp(Child->name, str) == 0) && (isDir(Child)))
				{
					// Found the node move to it and continue.
					Parent = Child;
					flag = TRUE;
				}
				else if ((Child != NULL) && (strcmp(Child->name, str) != 0) && (Child->siblingPtr != NULL))
				{
					// Traverse the list to find the correct sibling.
					while ((Child->siblingPtr != NULL) && (strcmp(Child->name, str) != 0))
					{
						Child = Child->siblingPtr;
					}

					if ((strcmp(Child->name, str) == 0) && (isDir(Child)))
					{
						// Found the node move to it and continue.
						Parent = Child;
						flag = TRUE;
					}
					else if ((Child->siblingPtr == NULL) && (strcmp(Child->name, str) != 0))
					{
						// error message and return.
						printf("Node does not exist in directory\n");
						return FALSE;
					}
					else if ((strcmp(Child->name, str) == 0) && (!isDir(Child)))
					{
						// error message and return.
						printf("Node %s, is not a directory\n", Child->name);
						return FALSE;
					}
					else
					{
						// odd error message and return.
						printf("Something unexpected occurred\n");
						return FALSE;
					}
				}
				else
				{
					// odd error message and return.
					printf("No subdirectory exists\n");
					return FALSE;
				}
				str = strtok(NULL, "/");
			}
		}
		// Otherwise the path starts relative to where we are.
		else
		{
			Parent = cwd;

			while ((str) && (strcmp(str, bname) != 0))
			{
				Child = Parent->childPtr;

				if ((Child != NULL) && (strcmp(Child->name, str) != 0) && (Child->siblingPtr == NULL))
				{
					// error message and return.
					printf("%s: No such directory exists\n", str);
					return FALSE;
				}
				else if ((Child != NULL) && (strcmp(Child->name, str) == 0) && (!isDir(Child)))
				{
					// error message and return.
					printf("Node %s, is not a directory\n", Child->name);
					return FALSE;
				}
				else if ((Child != NULL) && (strcmp(Child->name, str) == 0) && (isDir(Child)))
				{
					// Found the node move to it and continue.
					Parent = Child;
					flag = TRUE;
				}
				else if ((Child != NULL) && (strcmp(Child->name, str) != 0) && (Child->siblingPtr != NULL))
				{
					// Traverse the list to find the correct sibling.
					while ((Child->siblingPtr != NULL) && (strcmp(Child->name, str) != 0))
					{
						Child = Child->siblingPtr;
					}

					if ((strcmp(Child->name, str) == 0) && (isDir(Child)))
					{
						// Found the node move to it and continue.
						Parent = Child;
						flag = TRUE;
					}
					else if ((Child->siblingPtr == NULL) && ((strcmp(Child->name, str) != 0)))
					{
						// error message and return.
						printf("Node does not exist in directory\n");
						return FALSE;
					}
					else if ((strcmp(Child->name, str) == 0) && (!isDir(Child)))
					{
						// error message and return.
						printf("Node %s, is not a directory\n", Child->name);
						return FALSE;
					}
					else
					{
						// odd error message and return.
						printf("Something unexpected occurred\n");

						return FALSE;
					}
				}
				else if (Child == NULL)
				{
					// odd error message and return.
					printf("No subdirectory exists\n");
					return FALSE;
				}
				str = strtok(NULL, "/");
			}
		}
	}

	// We are now in the correct location to add.
	if (flag)
	{
		Child = Parent->childPtr;
		NODE *pMem = makefNode(Parent, bname);

		// If there is no child below the parent. Add at the child.
		if (!Child)
		{
			Parent->childPtr = pMem;
			return TRUE;
		}
		// Otherwise add at the end of the sibling location.
		else
		{
			while ((Child->siblingPtr != NULL) && (strcmp(Child->name, bname) != 0))
			{
				Child = Child->siblingPtr;
			}

			if (strcmp(Child->name, bname) == 0)
			{
				printf("%s already exists in directory\n", bname);
				free(pMem);
				return FALSE;
			}

			Child->siblingPtr = pMem;
			return TRUE;
		}
	}

	return TRUE;
}

int rmdir(char *pathname)
{
	char temp[128], *str;
	NODE *Parent = NULL, *Child = NULL;
	int flag = 0;

	if ((pathname[0] == '/') && (pathname[1] == '\0'))
	{
		printf("Cannot remove root directory\n");
		return -1;
	}

	strcpy(temp, pathname);
	strcpy(dname, dirname(temp));   // dname="/a/b"

	strcpy(temp, pathname);
	strcpy(bname, basename(temp));   // bname="c"

	str = strtok(dname, "/");

	if (str == NULL)
	{
		if (pathname[0] == '/')
		{
			Child = root->childPtr;

			if ((Child != NULL) && (strcmp(Child->name, bname) == 0))
			{
				if (!isDir(Child))
				{
					printf("%s is not a directory\n", bname);
					return FALSE;
				}
				else if (!isEmpty(Child))
				{







					printf("Directory: %s is not empty. Cannot delete from non empty directory\n", Child->name);
					return FALSE;
				}
				else
				{
					printf("Deleting Directory\n");
					NODE *pTemp = Child;

					Child = Child->siblingPtr;
					root->childPtr = Child;

					free(pTemp);
					printf("Directory successfully deleted\n");
					return TRUE;
				}
			}
			else if ((Child != NULL) && (strcmp(Child->name, bname) != 0))
			{
				NODE *pPrev = NULL;

				while((Child->siblingPtr != NULL) && (strcmp(Child->name, bname) != 0))
				{
					pPrev = Child;
					Child = Child->siblingPtr;
				}

				if ((Child->siblingPtr == NULL) && (strcmp(Child->name, bname) != 0))
				{
					printf("No such directory exists to remove\n");
					return FALSE;
				}
				else if ((strcmp(Child->name, bname) == 0) && (!isEmpty(Child)))
				{
					printf("Directory: %s is not empty. Cannot delete from non empty directory\n", Child->name);
					return FALSE;
				}
				else if ((strcmp(Child->name, bname) == 0) && (!isDir(Child)))
                                {
                                        printf("%s is not a directory\n", Child->name);
                                        return FALSE;
                                }
				else
				{
					printf("Deleting Directory\n");
					NODE *pTemp = Child;

                                        Child = Child->siblingPtr;
                                        pPrev->siblingPtr = Child;

                                        free(pTemp);
                                        printf("Directory successfully deleted\n");
				}
			}
			else if (Child == NULL)
			{
				printf("Directory %s does not exist\n", bname);
                        	return FALSE;
			}
		}
	}
	else if ((strcmp(dname, ".\0") == 0))
	{
		Child = cwd->childPtr;

		if ((Child != NULL) && (strcmp(Child->name, bname) == 0))
                {
                	if (!isDir(Child))
                        {
                        	printf("%s is not a directory\n", Child->name);
				return FALSE;
                        }
                        else if (!isEmpty(Child))
                        {
                        	printf("Directory: %s is not empty. Cannot delete from non empty directory\n", Child->name);
                                return FALSE;
                        }
                        else
                        {
				printf("Deleting Directory\n");
                        	NODE *pTemp = Child;

                                Child = Child->siblingPtr;
                                cwd->childPtr = Child;

                                free(pTemp);
                                printf("Directory successfully deleted\n");
                                return TRUE;
                        }
		}
		else if ((Child != NULL) && (strcmp(Child->name, bname) != 0) && (isDir(Child)))
                {
	                NODE *pPrev = NULL;

                        while((Child->siblingPtr != NULL) && (strcmp(Child->name, bname) != 0))
                        {
        	                pPrev = Child;
                                Child = Child->siblingPtr;
                        }
			if ((Child->siblingPtr == NULL) && (strcmp(Child->name, bname) != 0))
                        {
                        	printf("No such directory exists to remove\n");
                                return FALSE;
                        }
                        else if ((strcmp(Child->name, bname) == 0) && (!isEmpty(Child)))
                        {
                        	printf("Directory: %s is not empty. Cannot delete from non empty directory\n", Child->name);
                                return FALSE;
                        }
                        else if ((strcmp(Child->name, bname) == 0) && (!isDir(Child)))
                        {
                        	printf("%s is not a directory\n", Child->name);
                                return FALSE;
                        }
                        else
                        {
				printf("Deleting Directory\n");
                        	NODE *pTemp = Child;

                                Child = Child->siblingPtr;
                                pPrev->siblingPtr = Child;

                                free(pTemp);
                                printf("Directory successfully deleted\n");
                        }
		}
		else if (Child == NULL)
		{
			printf("Directory %s does not exist\n", bname);
			return FALSE;
		}
	}
	else
	{
		if (pathname[0] == '/')
		{
			Parent = root;

			while (str)
			{
				Child = Parent->childPtr;

				if ((Child != NULL) && (strcmp(Child->name, str) != 0) && (Child->siblingPtr == NULL))
				{
					// error message and return.
					printf("%s: No such directory exists\n", str);
					return FALSE;
				}
				else if ((Child != NULL) && (strcmp(Child->name, str) == 0) && (!isDir(Child)))
				{
					// error message and return.
					printf("Node %s, is not a directory\n", Child->name);
					return FALSE;
				}
				else if ((Child != NULL) && (strcmp(Child->name, str) == 0) && (isDir(Child)))
				{
					// Found the node move to it and continue.
					Parent = Child;
					flag = TRUE;
				}
				else if ((Child != NULL) && (strcmp(Child->name, str) != 0) && (Child->siblingPtr != NULL) && (isDir(Child)))
				{
					// Traverse the list to find the correct sibling.
					while ((Child->siblingPtr != NULL) && (strcmp(Child->name, str) != 0))
					{
						Child = Child->siblingPtr;
					}

					if ((strcmp(Child->name, str) == 0) && (isDir(Child)))
					{
						// Found the node move to it and continue.
						Parent = Child;
						flag = TRUE;
					}
					else if ((Child->siblingPtr == NULL) && (strcmp(Child->name, str) != 0))
					{
						// error message and return.
						printf("Node does not exist in directory\n");
						return FALSE;
					}
					else if ((strcmp(Child->name, str) == 0) && (!isDir(Child)))
					{
						// error message and return.
						printf("Node %s, is not a directory\n", Child->name);
						return FALSE;
					}
					else
					{
						// odd error message and return.
						printf("Something unexpected occurred\n");
						return FALSE;
					}
				}
				else
				{
					// odd error message and return.
					printf("No subdirectory exists\n");
					return FALSE;
				}
				str = strtok(NULL, "/");
			}
		}
		// Otherwise the path starts relative to where we are.
		else
		{
			Parent = cwd;

			while (str)
			{
				Child = Parent->childPtr;

				if ((Child != NULL) && (strcmp(Child->name, str) != 0) && (Child->siblingPtr == NULL))
				{
					// error message and return.
					printf("%s: No such directory exists\n", str);
					return FALSE;
				}
				else if ((Child != NULL) && (strcmp(Child->name, str) == 0) && (!isDir(Child)))
				{
					// error message and return.
					printf("Node %s, is not a directory\n", Child->name);
					return FALSE;
				}
				else if ((Child != NULL) && (strcmp(Child->name, str) == 0) && (isDir(Child)))
				{
					// Found the node move to it and continue.
					Parent = Child;
					flag = TRUE;
				}
				else if ((Child != NULL) && (strcmp(Child->name, str) != 0) && (Child->siblingPtr != NULL) && (isDir(Child)))
				{
					// Traverse the list to find the correct sibling.
					while ((Child->siblingPtr != NULL) && (strcmp(Child->name, str) != 0))
					{
						Child = Child->siblingPtr;
					}

					if ((strcmp(Child->name, str) == 0) && (isDir(Child)))
					{
						// Found the node move to it and continue.
						Parent = Child;
						flag = TRUE;
					}
					else if ((Child->siblingPtr == NULL) && ((strcmp(Child->name, str) != 0)))
					{
						// error message and return.
						printf("Node does not exist in directory\n");
						return FALSE;
					}
					else if ((strcmp(Child->name, str) == 0) && (!isDir(Child)))
					{
						// error message and return.
						printf("Node %s, is not a directory\n", Child->name);
						return FALSE;
					}
					else
					{
						// odd error message and return.
						printf("Something unexpected occurred\n");
						return FALSE;
					}
				}
				else
				{
					// odd error message and return.
					printf("No subdirectory exists\n");
					return FALSE;
				}
				str = strtok(NULL, "/");
			}
		}

		if (flag)
		{
			Child = Parent->childPtr;
			// If the child is the node we must delete.
			if ((Child != NULL) && (strcmp(Child->name, bname) == 0))
			{
				// If the child is indeed a directory.
				if (isDir(Child))
				{
					// If the child is not empty.
					if (!isEmpty(Child))
					{
						printf("%s is not empty, directory must be empty before deleting\n", Child->name);
						return FALSE;
					}
					// If the child is empty.
					else if (isEmpty(Child))
					{
						printf("Deleting Directory\n");
						NODE *pTemp = Child;
						Child = Child->siblingPtr;
						Parent->childPtr = Child;

						free(pTemp);
						printf("Directory Successfully Deleted\n");
						return TRUE;
					}
				}
				// Otherwise it is not a directory.
				else if (!isDir(Child))
				{
					printf("%s: is not a directory\n", bname);
					return FALSE;
				}
			}
			else if ((Child != NULL) && (strcmp(Child->name, bname) != 0) && (Child->siblingPtr != NULL))
			{
				NODE *pPrev = NULL;

				// Traverse the list, keep track of the previous pointer
				while ((Child->siblingPtr != NULL) && (strcmp(Child->name, bname) != 0))
				{
					pPrev = Child;
					Child = Child->siblingPtr;
				}

				// If we reach the end of the siblings and have not found the bname, Directory does not exist.
				if ((Child->siblingPtr == NULL) && (strcmp(Child->name, bname) != 0))
				{
					printf("%s Directory does not exist\n", bname);
					return FALSE;
				}
				// Found the correct node to delete.
				else if (strcmp(Child->name, bname) == 0)
				{
					// Is a directory and is empty, permitted to delete.
					if ((isDir(Child)) && (isEmpty(Child)))
					{
						printf("Deleting Directory\n");
						NODE *pTemp = Child;

						Child = Child->siblingPtr;
						pPrev->siblingPtr = Child;

						free(pTemp);
						printf("Directory Successfully Deleted\n");
					}
					// Not a directory, cannot delete.
					else if (!isDir(Child))
					{
						printf("%s is not a directory\n", Child->name);
						return FALSE;
					}
					// Is a directory but is not empty, cannot delete.
					else if (!isEmpty(Child))
					{
						printf("Directory: %s is not empty. Cannot delete from non empty directory\n", Child->name);
						return FALSE;
					}
					else
					{
						printf("Something unexpected occurred\n");
						return FALSE;
					}
				}
			}
			else if (Child == NULL)
			{
				// odd error message and return.
				printf("No subdirectory exists\n");
				return FALSE;
			}
		}
	}

	return TRUE;
}

int rm(char *pathname)
{
	char temp[128], *str;
	NODE *Parent = NULL, *Child = NULL;
	int flag = 0;

	if ((pathname[0] == '/') && (pathname[1] == '\0'))
	{
		printf("Cannot remove root directory\n");
		return -1;
	}

	strcpy(temp, pathname);
	strcpy(dname, dirname(temp));   // dname="/a/b"

	strcpy(temp, pathname);
	strcpy(bname, basename(temp));   // bname="c"

	str = strtok(dname, "/");

	if (str == NULL)
	{
		if (pathname[0] == '/')
		{
			Child = root->childPtr;

			if ((Child != NULL) && (strcmp(Child->name, bname) == 0))
			{
				if (isDir(Child))
				{
					printf("%s is a directory, cannot use rm to remove a directory\n", Child->name);
					return FALSE;
				}
				else
				{
					printf("Deleting File\n");
					NODE *pTemp = Child;

					Child = Child->siblingPtr;
					root->childPtr = Child;

					free(pTemp);
					printf("File successfully deleted\n");
					return TRUE;
				}
			}
			else if ((Child != NULL) && (strcmp(Child->name, bname) != 0))
			{
				NODE *pPrev = NULL;

				while((Child->siblingPtr != NULL) && (strcmp(Child->name, bname) != 0))
				{
					pPrev = Child;
					Child = Child->siblingPtr;
				}

				if ((Child->siblingPtr == NULL) && (strcmp(Child->name, bname) != 0))
				{
					printf("No such file exists to remove\n");
					return FALSE;
				}
				else if ((strcmp(Child->name, bname) == 0) && (isDir(Child)))
                                {
                                        printf("%s is a directory, cannot use rm to remove a directory\n", Child->name);
                                        return FALSE;
                                }
				else if (strcmp(Child->name, bname) == 0)
				{
					printf("Deleting File\n");
					NODE *pTemp = Child;

                                        Child = Child->siblingPtr;
                                        pPrev->siblingPtr = Child;

                                        free(pTemp);
                                        printf("Directory successfully deleted\n");
				}
			}
			else if (Child == NULL)
			{
				printf("File %s does not exist\n", bname);
                        	return FALSE;
			}
		}
	}
	else if ((strcmp(dname, ".\0") == 0))
	{
		Child = cwd->childPtr;

		if ((Child != NULL) && (strcmp(Child->name, bname) == 0))
                {
                	if (isDir(Child))
                        {
                        	printf("%s is a directory, cannot use rm to remove a directory\n", Child->name);
				return FALSE;
                        }
                        else
                        {
				printf("Deleting File\n");
                        	NODE *pTemp = Child;

                                Child = Child->siblingPtr;
                                cwd->childPtr = Child;

                                free(pTemp);
                                printf("File successfully deleted\n");
                                return TRUE;
                        }
		}
		else if ((Child != NULL) && (strcmp(Child->name, bname) != 0) && (isDir(Child)))
                {
	                NODE *pPrev = NULL;

                        while((Child->siblingPtr != NULL) && (strcmp(Child->name, bname) != 0))
                        {
        	                pPrev = Child;
                                Child = Child->siblingPtr;
                        }

			if ((Child->siblingPtr == NULL) && (strcmp(Child->name, bname) != 0))
                        {
                        	printf("No such file exists to remove\n");
                                return FALSE;
                        }
                        else if ((strcmp(Child->name, bname) == 0) && (isDir(Child)))
                        {
                        	printf("%s is a directory, cannot use rm to remove a directory\n", Child->name);
                                return FALSE;
                        }
                        else if (strcmp(Child->name, bname) == 0)
                        {
				printf("Deleting File\n");
                        	NODE *pTemp = Child;

                                Child = Child->siblingPtr;
                                pPrev->siblingPtr = Child;

                                free(pTemp);
                                printf("File successfully deleted\n");
                        }
		}
		else if (Child == NULL)
		{
			printf("File %s does not exist\n", bname);
			return FALSE;
		}
	}
	else
	{
		if (pathname[0] == '/')
		{
			Parent = root;

			while (str)
			{
				Child = Parent->childPtr;

				if ((Child != NULL) && (strcmp(Child->name, str) != 0) && (Child->siblingPtr == NULL))
				{
					// error message and return.
					printf("No such file exists\n");
					return FALSE;
				}
				else if ((Child != NULL) && (strcmp(Child->name, str) == 0) && (!isDir(Child)))
				{
					// error message and return.
					printf("Node %s, is not a directory\n", Child->name);
					return FALSE;
				}
				else if ((Child != NULL) && (strcmp(Child->name, str) == 0) && (isDir(Child)))
				{
					// Found the node move to it and continue.
					Parent = Child;
					flag = TRUE;
				}
				else if ((Child != NULL) && (strcmp(Child->name, str) != 0) && (Child->siblingPtr != NULL) && (isDir(Child)))
				{
					// Traverse the list to find the correct sibling.
					while ((Child->siblingPtr != NULL) && (strcmp(Child->name, str) != 0))
					{
						Child = Child->siblingPtr;
					}

					if ((strcmp(Child->name, str) == 0) && (isDir(Child)))
					{
						// Found the node move to it and continue.
						Parent = Child;
						flag = TRUE;
					}
					else if ((Child->siblingPtr == NULL) && (strcmp(Child->name, str) != 0))
					{
						// error message and return.
						printf("Node does not exist in directory\n");
						return FALSE;
					}
					else if ((strcmp(Child->name, str) == 0) && (!isDir(Child)))
					{
						// error message and return.
						printf("Node %s, is not a directory\n", Child->name);
						return FALSE;
					}
					else
					{
						// odd error message and return.
						printf("Something unexpected occurred\n");
						return FALSE;
					}
				}
				else
				{
					// odd error message and return.
					printf("No subdirectory exists\n");
					return FALSE;
				}
				str = strtok(NULL, "/");
			}
		}
		// Otherwise the path starts relative to where we are.
		else
		{
			Parent = cwd;

			while (str)
			{
				Child = Parent->childPtr;

				if ((Child != NULL) && (strcmp(Child->name, str) != 0) && (Child->siblingPtr == NULL))
				{
					// error message and return.
					printf("%s: No such directory exists\n", str);
					return FALSE;
				}
				else if ((Child != NULL) && (strcmp(Child->name, str) == 0) && (!isDir(Child)))
				{
					// error message and return.
					printf("Node %s, is not a directory\n", Child->name);
					return FALSE;
				}
				else if ((Child != NULL) && (strcmp(Child->name, str) == 0) && (isDir(Child)))
				{
					// Found the node move to it and continue.
					Parent = Child;
					flag = TRUE;
				}
				else if ((Child != NULL) && (strcmp(Child->name, str) != 0) && (Child->siblingPtr != NULL) && (isDir(Child)))
				{
					// Traverse the list to find the correct sibling.
					while ((Child->siblingPtr != NULL) && (strcmp(Child->name, str) != 0))
					{
						Child = Child->siblingPtr;
					}

					if ((strcmp(Child->name, str) == 0) && (isDir(Child)))
					{
						// Found the node move to it and continue.
						Parent = Child;
						flag = TRUE;
					}
					else if ((Child->siblingPtr == NULL) && ((strcmp(Child->name, str) != 0)))
					{
						// error message and return.
						printf("Node does not exist in directory\n");
						return FALSE;
					}
					else if ((strcmp(Child->name, str) == 0) && (!isDir(Child)))
					{
						// error message and return.
						printf("Node %s, is not a directory\n", Child->name);
						return FALSE;
					}
					else
					{
						// odd error message and return.
						printf("Something unexpected occurred\n");
						return FALSE;
					}
				}
				else
				{
					// odd error message and return.
					printf("No subdirectory exists\n");
					return FALSE;
				}
				str = strtok(NULL, "/");
			}
		}

		if (flag)
		{
			Child = Parent->childPtr;

			// If the child is the node we must delete.
			if ((Child != NULL) && (strcmp(Child->name, bname) == 0))
			{
				// If the child is indeed a directory.
				if (isDir(Child))
				{

					printf("%s is a directory, cannot use rm to remove a directory\n", Child->name);
					return FALSE;
				}
				else if (!isDir(Child))
				{
					printf("Deleting File\n");
					NODE *pTemp = Child;
					Child = Child->siblingPtr;
					Parent->childPtr = Child;

					free(pTemp);
					printf("File Successfully Deleted\n");
					return TRUE;
				}
			}
			else if ((Child != NULL) && (strcmp(Child->name, bname) != 0) && (Child->siblingPtr != NULL))
			{
				NODE *pPrev = NULL;

				// Traverse the list, keep track of the previous pointer
				while ((Child->siblingPtr != NULL) && (strcmp(Child->name, bname) != 0))
				{
					pPrev = Child;
					Child = Child->siblingPtr;
				}

				// If we reach the end of the siblings and have not found the bname, Directory does not exist.
				if ((Child->siblingPtr == NULL) && (strcmp(Child->name, bname) != 0))
				{
					printf("%s File does not exist\n", bname);
					return FALSE;
				}
				// Found the correct node to delete.
				else if (strcmp(Child->name, bname) == 0)
				{
					// Is a file, permitted to delete.
					if (!isDir(Child))
					{
						printf("Deleting File\n");
						NODE *pTemp = Child;

						Child = Child->siblingPtr;
						pPrev->siblingPtr = Child;

						free(pTemp);
						printf("File Successfully Deleted\n");
					}
					// Not a directory, cannot delete.
					else if (isDir(Child))
					{
						printf("%s is a directory, cannot use rm to remove a directory\n", Child->name);
						return FALSE;
					}
					else
					{
						printf("Something unexpected occurred\n");
						return FALSE;
					}
				}
			}
			else if (Child == NULL)
			{
				// odd error message and return.
				printf("No subdirectory exists\n");
				return FALSE;
			}
		}
	}

	return TRUE;
}

int pwd(char *pathname)
{
	rpwd(cwd);
	printf("\n");
	return TRUE;
}

int rpwd(NODE *p)
{
	if (root == p)
	{
		printf("/");
		return TRUE;
	}
	else if (p->parentPtr != root)
	{
		rpwd(p->parentPtr);
		printf("/%s", p->name);
	}
	else
	{
		rpwd(p->parentPtr);
		printf("%s", p->name);
	}
}

int cd(char *pathname)
{
	char temp[128], *str;
	NODE *Parent = NULL, *Child = NULL;

	strcpy(temp, pathname);
	str = strtok(temp, "/");
	
	if (pathname[0] == '\0')
	{
		cwd = root;
		printf("Root Directory: %s\n", cwd->name);
		return TRUE;
	}
	else if ((pathname[0] == '.') && (pathname[1] == '\0'))
	{
		printf("Current Working Directory: \n");
		rpwd(cwd);
		printf("\n");
		return TRUE;
	}
	else if ((pathname[0] == '.') && (pathname[1] == '.'))
	{
		cwd = cwd->parentPtr;
		printf("Current Working Directory: \n");
		rpwd(cwd);
		printf("\n");
		return TRUE;
	}
	else if ((pathname[0] != '/') && (pathname[1] != '.'))
	{
		Parent = cwd;

		while (str)
		{
			Child = Parent->childPtr;

			if ((Child != NULL) && (strcmp(Child->name, str) == 0) && (isDir(Child)))
			{
				Parent = Child;
			}
			else if ((Child != NULL) && (strcmp(Child->name, str) == 0) && (!isDir(Child)))
			{
				printf("%s is a file\n", str);
				return FALSE;
			}
			else if ((Child != NULL) && (strcmp(Child->name, str) != 0) && (Child->siblingPtr == NULL))
			{
				printf("%s no such directory exists\n", str);
				return FALSE;
			}
			else if ((Child != NULL) && (strcmp(Child->name, str) != 0) && (Child->siblingPtr != NULL))
			{
				NODE *pTemp = Child;

				while ((pTemp->siblingPtr != NULL) && (strcmp(pTemp->name, str) != 0))
				{
					pTemp = pTemp->siblingPtr;
				}

				if ((pTemp->siblingPtr == NULL) && (strcmp(pTemp->name, str) != 0))
				{
					printf("%s no such directory exists\n", str);
					return FALSE;
				}
				else if ((strcmp(pTemp->name, str) == 0) && (!isDir(pTemp)))
				{
					printf("%s is a file\n", str);
					return FALSE;
				}
				else if ((strcmp(pTemp->name, str) == 0) && (isDir(pTemp)))
				{
					Parent = pTemp;
				}
				else
				{
					printf("Unexpected Error\n");
					return FALSE;
				}
			}
			else
			{
				printf("No subdirectory exists\n");
				return FALSE;
			}
			str = strtok(NULL, "/");
		}
		
		cwd = Parent;
		printf("Current Working Directory: ");
		rpwd(Parent);
		printf("\n");

		return TRUE;
	}
	else if (pathname[0] == '/')
	{
		Parent = root;
		while (str)
		{
			Child = Parent->childPtr;

			if ((Child != NULL) && (strcmp(Child->name, str) == 0) && (isDir(Child)))
			{
				Parent = Child;
			}
			else if ((Child != NULL) && (strcmp(Child->name, str) != 0) && (Child->siblingPtr == NULL))
			{
				printf("%s no such directory exists\n", str);
				return FALSE;
			}
			else if ((Child != NULL) && (strcmp(Child->name, str) != 0) && (Child->siblingPtr != NULL))
			{
				NODE *pTemp = Child;

				while ((pTemp->siblingPtr != NULL) && (strcmp(pTemp->name, str) != 0))
				{
					pTemp = pTemp->siblingPtr;
				}

				if ((pTemp->siblingPtr == NULL) && (strcmp(pTemp->name, str) != 0))
				{
					printf("%s no such directory exists\n", str);
					return FALSE;
				}
				else if ((strcmp(pTemp->name, str) == 0) && (!isDir(pTemp)))
				{
					printf("%s is not a directory\n", str);
					return FALSE;
				}
				else if ((strcmp(pTemp->name, str) == 0) && (isDir(pTemp)))
				{
					Parent = pTemp;
				}
				else
				{
					printf("Something unexpected occurred\n");
					return FALSE;
				}
			}
			else
			{
				printf("No subdirectory exists\n");
				return FALSE;
			}
			str = strtok(NULL, "/");
		}

		cwd = Parent;
		printf("You are now in this directory: ");
		rpwd(cwd);
		printf("\n");

		return TRUE;
	}
	else
	{
		printf("An unexpected error has occurred\n");
		return FALSE;
	}

	return TRUE;
}

int ls(char *pathname)
{
	char temp[128], *str;
	NODE *Parent = NULL, *Child = NULL;
	int flag = FALSE;

	strcpy(temp, pathname);
	strcpy(dname, dirname(temp));

	str = strtok(temp, "/");
	
	if (pathname[0] != '/')
	{
		Parent = cwd;
		Child = Parent->childPtr;

		if (Child == NULL)
		{
			printf("TYPE\tNAME\n");
			printf("D\t.\n");
			printf("D\t..\n");
			return TRUE;
		}
		else if((Child != NULL) && (Child->siblingPtr == NULL))
		{
			printf("TYPE\tNAME\n");
			printf("D\t.\n");
			printf("D\t..\n");
			printf("%c\t%s\n", Child->type, Child->name);
			return TRUE;
		}
		else if ((Child != NULL) && (Child->siblingPtr != NULL))
		{
			NODE *pTemp = Child;
			printf("TYPE\tNAME\n");
			printf("D\t.\n");
			printf("D\t..\n");

			while (pTemp != NULL)
			{
				printf("%c\t%s\n", pTemp->type, pTemp->name);
				pTemp = pTemp->siblingPtr;
			}
			return TRUE;
		}
		else
		{
			printf("Unexpected Error\n");
			return FALSE;
		}
	}
	else if (pathname[0] == '/')
	{
		Parent = root;
		printf("Why");
		while (str)
		{
			Child = Parent->childPtr;

			if ((Child != NULL) && (strcmp(Child->name, str) == 0) && (isDir(Child)))
			{
				Parent = Child;
				flag = TRUE;
			}
			else if ((Child != NULL) && (strcmp(Child->name, str) != 0) && (Child->siblingPtr == NULL))
			{
				printf("%s is not a valid directory\n", str);
				return FALSE;
			}
			else if ((Child != NULL) && (strcmp(Child->name, str) != 0) && (Child->siblingPtr != NULL))
			{
				NODE *pTemp = Child;

				while ((pTemp->siblingPtr != NULL) && (strcmp(pTemp->name, str) != 0))
				{
					pTemp = pTemp->siblingPtr;
				}

				if ((pTemp->siblingPtr == NULL) && (strcmp(pTemp->name, str) != 0))
				{
					printf("%s no such directory exists\n", str);
					return FALSE;
				}
				else if ((strcmp(pTemp->name, str) == 0) && (!isDir(pTemp)))
				{
					printf("%s is not a directory\n", str);
					return FALSE;
				}
				else if ((strcmp(pTemp->name, str) == 0) && (isDir(pTemp)))
				{
					Parent = pTemp;
					flag = TRUE;
				}
				else
				{
					printf("Something unexpected occurred\n");
					return FALSE;
				}
			}
			else
			{
				printf("No subdirectory exists\n");
				return FALSE;
			}

			str = strtok(NULL, "/");
		}
	}

	if (flag)
	{
		Child = Parent->childPtr;

		if (Child == NULL)
		{
			printf("TYPE\tNAME\n");
			printf("D\t.\n");
			printf("D\t..\n");
			return TRUE;
		}
		else if((Child != NULL) && (Child->siblingPtr == NULL))
		{
			printf("TYPE\tNAME\n");
			printf("D\t.\n");
			printf("D\t..\n");
			printf("%c\t%s\n", Child->type, Child->name);
			return TRUE;
		}
		else if ((Child != NULL) && (Child->siblingPtr != NULL))
		{
			NODE *pTemp = Child;
			printf("TYPE\tNAME\n");
			printf("D\t.\n");
			printf("D\t..\n");

			while (pTemp != NULL)
			{
				printf("%c\t%s\n", pTemp->type, pTemp->name);
				pTemp = pTemp->siblingPtr;
			}

			return TRUE;
		}
		else
		{
			printf("Something unexpected happened\n");
			return FALSE;
		}
	}
	else
	{
		printf("Unable to print directory\n");
	}

	return TRUE;
}

int resave(FILE *outfile, NODE *p)
{
	if (root == p)
	{
		fprintf(outfile, "%s", p->name);
		return TRUE;
	}
	else if (p->parentPtr != root)
	{
		resave(outfile, p->parentPtr);
		fprintf(outfile, "/%s", p->name);
	}
	else
	{
		resave(outfile, p->parentPtr);
		fprintf(outfile, "%s", p->name);
	}
}

int savefile(FILE *outfile, NODE *p)
{
	if (!p)
	{
		return TRUE;
	}
	
	fprintf(outfile, "%c\t", p->type);

	resave(outfile, p);
	fprintf(outfile, "\n");

	savefile(outfile, p->childPtr);
	savefile(outfile, p->siblingPtr);
		
	return TRUE;
}

int save(char *pathname)
{
	FILE *outfile = fopen("file_system", "w+");

	if (outfile == NULL)
	{
		printf("Could not open file\n");
		return FALSE;
	}

	fprintf(outfile, "TYPE\tNAME\n");

	savefile(outfile, root);

	fclose(outfile);

	return TRUE;
}

int reload(char *pathname)
{
	FILE *infile = fopen("file_system", "r");
	char filetype, temp[128];
	int flag = TRUE;

	if (root->childPtr != NULL)
	{
		printf("Current system already in place\n");
		return FALSE;
	}

	if (infile == NULL)
	{
		printf("Could not open file\n");
		return FALSE;
	}
	else
	{
		while (!feof(infile))
		{
			fgets(temp, sizeof(temp), infile);

			sscanf(temp, "%c\t%s", &filetype, pathname);

			if (filetype == 'T')
			{
				continue;
			}
		
			if ((filetype == 'D') && (strcmp(&pathname[0], "/") == 0) && (strcmp(&pathname[1], "\0") == 0))
			{
				continue;
			}

			if (filetype == 'D')
			{
				mkdir(pathname);
			}
			else if (filetype == 'F')
			{
				creat(pathname);
			}
			else
			{
				printf("Unable to read line\n");
				flag = FALSE;
				break;
			}
		}

		if (flag)
		{
			printf("File system reloaded\n");
			fclose(infile);
			return TRUE;
		}
		else
		{
			printf("Unexpected error\n");
			fclose(infile);
			return FALSE;
		}
	}

	fclose(infile);
	return flag;
}

int quit(char *pathname)
{
	if(save(pathname))
	{
		exit(1);
		return TRUE;
	}
	else
	{
		exit(2);
		return FALSE;
	}
}

NODE *makedNode(NODE *Parent, char *pname)
{
	NODE *pMem = (NODE *)malloc(sizeof(NODE));
	strcpy(pMem->name, pname);
	pMem->parentPtr = Parent;
	pMem->siblingPtr = NULL;
	pMem->childPtr = NULL;
	pMem->type = 'D';

	return pMem;
}

NODE *makefNode(NODE *Parent, char *pname)
{
	NODE *pMem = (NODE *)malloc(sizeof(NODE));
	strcpy(pMem->name, pname);
	pMem->parentPtr = Parent;
	pMem->siblingPtr = NULL;
	pMem->childPtr = NULL;
	pMem->type = 'F';

	return pMem;
}

int (*fptr[])(char *) = { (int(*)(char *)) mkdir, rmdir, ls, cd, pwd, creat, rm, quit, menu, reload, save };

int main(int argc, char *argv[])
{
	initialize();
	while (TRUE)
	{
		printf("Input a command : ");
		fgets(line, 128, stdin);
		line[strlen(line)] = '\0';
		sscanf(line, "%s %s", command, pathname);
		
		int ID = findCmd(command);

		if (ID < 0)
		{
			printf("Invalid command\n");
		}
		else
		{
			fptr[ID](pathname);
			strcpy(pathname, "\0");
			strcpy(command, "\0");
		}
	}
	return 0;
}
