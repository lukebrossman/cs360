#include <string.h>
#include <stdlib.h>
#include <stdio.h>
		    
typedef struct node{
          char  name[64];       // node's name string
          char  type;
   struct node *child, *sibling, *parent;
}NODE;

char Directory = 'd', File = 'f';
int  numberOfTokenStrings;
char command[64], path[64], line[128];
NODE *root, *cwd, *start, *destParent;
char dname[64], bname[64];        // for dirname and basename 
char* name[100], *destNode;
                           // number of token strings in pathname 
FILE *fp;

int mkdir(char *pathname);
int rmdir(char *pathname);
int save(char *pathname);
int reload(char *pathname);
int rm(char *pathname);
int menu(char *pathname);
int creat(char *pathname);
int quit(char *pathname);
int ls(char *pathname);
int cd(char *pathname);
int pwd(char *pathname);


int (*fptr[])(char *) = {(int (*) (char *)) mkdir, rmdir, ls, cd, pwd, creat, rm, reload, save, menu, quit};

char *cmd[] = {"mkdir", "rmdir", "ls", "cd", "pwd", "creat", "rm",
 "reload", "save", "menu", "quit", NULL};

int tokenize(char *input)
{
    printf("debug1.1");
    char *s;
    int i = 0;
    printf("debug1.2");
    s = strtok(input, "/");
    strcpy(command, s);
    printf("debug1.3");
    while(s)
    {
        printf("debug1");
        
        
        printf("debug2");
        strcpy(name[i], s);
        printf("debug3");
        i++;
        printf("debug5");
        s = strtok(0, "/"); 
    }

	return i;
}


NODE *CreateNode(NODE *parent, char *name, char type)
{
    NODE* newNode = (NODE*)malloc(sizeof(NODE));
    newNode->type = type;
    strcpy(newNode->name, name);
    newNode->child = NULL;
    newNode->sibling = NULL;
    return newNode;
}

void InsertNode(NODE *parent, char *name, char type)
{
    if (parent->type == Directory)
    {
        NODE *newNode = CreateNode(parent, name, type);
        newNode->sibling = parent->child;
        parent->child = newNode;
    }
    else
    {
        printf("Path points to a file, creation canceled");
    }
}

NODE *SearchSiblings(NODE *child, char *name)
{
  NODE *foundNode = NULL;
  if (strcmp(child->name, name) == 0 || child == NULL)
  {
      foundNode = child;
  } else if (child != NULL) 
  {
    foundNode = SearchSiblings(child->sibling, name);
  }
  return foundNode;
}

NODE *SearchChild(NODE *parent, char *name)
{
    NODE* foundNode;
    foundNode = SearchSiblings(parent->child, name);
    return foundNode;
}

NODE *path2node(char *pathname)
{
   //return the node pointer of a pathname, or 0 if the node does not exist.  

   if (pathname[0] = '/') 
   {
       start = root;
   }
   else                 
   {
       start = cwd;
   }

   NODE *p = start;

   for (int i=0; i < numberOfTokenStrings; i++)
    {
        destParent = p;
        destNode = name[i];
        p = SearchChild(p, name[i]);
        if (p==0) return 0;            // if name[i] does not exist
    }
   return p;
}

void PrintTreeToFile(NODE* treeRoot)
{
    if (treeRoot != NULL)
    {
    fprintf(fp," %s \n", treeRoot->name);
    PrintTreeToFile(treeRoot->child);
    PrintTreeToFile(treeRoot->sibling);
    }
    else
    {
        fprintf(fp, "NULL \n");
    }
}

void SaveTreeToFile(char * saveFile)
{
    fp = fopen (saveFile, "w+");
    PrintTreeToFile(root);
    fclose(fp);
}

void PrintTreeFromRoot(NODE* treeRoot)
{
    if (treeRoot != NULL)
    {
        printf(" %s \n", treeRoot->name);
        PrintTreeFromRoot(treeRoot->child);
        PrintTreeFromRoot(treeRoot->sibling);

    }
    else
    {
        printf("NULL \n");
    }
}

void PrintTreeToConsole(NODE * curdir) 
{
    PrintTreeFromRoot(curdir);
}

void LoadTreeFromFile(FILE* saveFile)
{
    char* nodeName;
    while (!feof(saveFile))
    {

    }
}

int FindCommand(char * command)
{
    int commandIndex;
    for (commandIndex = 0; cmd[commandIndex] != NULL; commandIndex++)
    {
        if (strcmp(command, cmd[commandIndex]) == 0)
        {
            return commandIndex;
        }
    }
    return -1; // not found: return -1
}

void reset()
{
    memset(line,0,sizeof(line));
    memset(command,0,sizeof(command));
    memset(path,0,sizeof(path));

}

void Initialize()
{
    root = CreateNode(root, "/", Directory);
    cwd = root;
    start = cwd;   
    reset();              
}

void RemoveNode(NODE* node)
{
    if (node->child != NULL)
    {
        printf("%s is not empty", node->name);
    } 
    else
    {
        if (node->parent->child == node)
        {
            node->parent->child = node->sibling;
        }
        free(node);
    }
}
int creat(char * pathname)
{
    if (pathname[0])
    {
        NODE* path = path2node(pathname);
        if (path != NULL)
        {
            printf("Directory already exists");
        }
        else if (path->type == Directory)
        {
            InsertNode(destParent, destNode, File);
        }
        else{
            printf("Path is not a directory");
        }
    }
    return 1;
}

int menu(char *pathname)
{
    printf("mkdir  pathname : make a new directory for a given pathname\nrmdir  pathname : remove the directory, if it is empty.\ncd    [pathname]: change CWD to pathname, or to / if no pathname.\nls    [pathname]: list the directory contents of pathname or CWD\npwd             : print the (absolute) pathname of CWDcreat  pathname : create a FILE node.\nrm     pathname : remove the FILE node.\nsave   filename : save the current file system tree as a file\nreload filename : construct a file system tree from a file\nmenu            : show a menu of valid commands\nquit            : save the file system tree, then terminate the program.\n");
    return 1;
}

int pwd(char *pathname)
{
    printf("Current working directory: %s \n", cwd->name);
    return 1;
}

int ls(char *pathname)
{
    NODE* path = path2node(pathname);
    if (path != 0)
    {
        PrintTreeToConsole(path);
    }
    else
    {
        printf("Directory doesn't exist"); 
    }
    return 1;
}

int cd(char *pathname)
{
    if (pathname[0])
    {
        NODE* path = path2node(pathname);
        if (path != 0 && path->type == Directory)
        {
            cwd = path;
            printf("CWD changed to: %s \n", cwd->name);
        }
        else
        {
            printf("Directory doesn't exist"); 
        }
        start = cwd;
    }  
    else
    {
        cwd = root;
        printf("CWD changed to root \n");
    }
    return 1;
}

int rm(char *pathname)
{
    if (pathname[0])
    {
        NODE * path = path2node(pathname);
        if (path != NULL && path->type == File)
        {
            RemoveNode(path);
        }
        else
        {
            printf("File does not exist");
        }
    }
    return 1;
}

int reload(char *pathname)
{
    return 1;
}

int mkdir(char *pathname)
{
    if (pathname[0])
    {
        NODE* path = path2node(pathname);
        if (destNode != NULL)
        {
            printf("Directory already exists");
        }
        else if (path->type == Directory)
        {
            InsertNode(destParent, destNode, Directory);
        }
        else{
            printf("Path is not a directory");
        }
    }
    return 1;
}

int rmdir(char *pathname)
{
    if (pathname[0])
    {
        NODE * path = path2node(pathname);
        if (path != NULL && path->type == Directory)
        {
            RemoveNode(path);
        }
        else
        {
            printf("Directory does not exist");
        }
    }
    return 1;
}

int save(char *pathname)
{
    return 1;
}

int quit(char *pathname)
{
    return 0;
}

int main()
{                  // for gettting user input line                // token string pointers 
    int continueProgram = 1, commandIndex;
    char input[100];
    Initialize();
    while(continueProgram)
    {
        printf("input a commad line : ");
        fgets(line,128,stdin);
        printf("debug1.0");
        line[strlen(line)-1] = 0;
        sscanf(line, "%s %s", command, path);
        strcpy(input, path);
        printf("debug1.0");
        numberOfTokenStrings = tokenize(input);
        
        commandIndex = FindCommand(command);
        if (commandIndex != -1)
        {
            continueProgram = fptr[commandIndex](path);
        }
        reset();

      /*get user input line = [command pathname];
      identify the command;
      execute the command;
      break if command=“quit”; */
    }
}
