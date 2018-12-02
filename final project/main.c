
#include "functions.h"
#include "type.h"
#include "util.h"

// table of function pointers, cannot be declared in type.h under globals because type.h does not include functions.h
command cmdtable[] ={
	{"help", CMD_MENU, 2},
	{"exit", CMD_QUIT,2},
	{"cd", CMD_CD, 1},
	{"ls", CMD_LS, 1},
	{"mkdir", CMD_MKDIR, 0},
	{"creat", CMD_CREAT, 0},
	{"rmdir", CMD_RMDIR, 0},
	{"rm", CMD_UNLINK, 0},
	{"link", CMD_LINK, 0},
	{"unlink", CMD_UNLINK, 0},
	{"symlink", CMD_SYMLINK, 0},
	{"touch", CMD_TOUCH, 0},
	{"chmod", CMD_CHMOD, 0},
	{"stat", CMD_STAT, 0},
	{"pwd", CMD_PWD, 2},
	{0, 0, 0, 0, 0}
};

void main(int argc, char* argv[])
{
	char line[128], cname[64];
	int i, searching = 1;

	if (argc > 1)
		init(argv[1]);
	else
		init("");
	
	printf("===========================================\n");

	while(1)
	{
		printf("Running Process: P%d\n",running->pid);
		printf("> ");
		fgets(line, 128, stdin);
		line[strlen(line)-1] = 0;
		if(!line[0])
			continue;

		// reset input variables
		memset(pathname, 0, 256);
		memset(parameter, 0, 256);
		
		// get the command, pathname, and parameter variables
		sscanf(line, "%s %s %64c", cname, pathname, parameter);
		
		// find and execute the command
		find_and_execute_command(cname);
		
		
		if (sleepmode)
			sleep(2);
	}
}