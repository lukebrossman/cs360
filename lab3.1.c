#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/cdefs.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>

#define FALSE 0
#define TRUE 1

char *myargv1[64], *redirectargs[64], *pipeStr[64], *myargv2[64], tempargs[64][64], tempargs2[64][64];
char *path[64], **env, redirectPath[100];
char cmdPath[100];
int pid, status, pipeCmd, redirect, continueProgram;

int tokenize(char input[], int index, char * args[])
{
    if (*input == '\0')
    {
        sscanf(input, "%s", tempargs[index]);
        args[index] = 0;
        return index;
    }
    else
    {
        sscanf(input, "%s ", tempargs[index]);
        args[index] = tempargs[index];
        printf("%s \n", args[index]);
        input += 1 + strlen(tempargs[index]);
        tokenize1(input, index + 1);
    }
}

int main(int argc, char *argv[], char **envp)
{
    char line[128], *temp;
    continueProgram = TRUE;
    env = envp;
	while (continueProgram)
	{
		printf("Input a command : ");
		fgets(line, 127, stdin);
		line[strlen(line) - 1] = '\0';
            char* temp = strtok(input, delim);
        int i = 0;
        while (temp)
        {
            strcpy(args[i], temp);
            temp = strtok(NULL, delim);
            i++;
        }
        if (pipeStr[1] != NULL)
        {
            //runPipeCmd();
        }
        else
        {
            printf("dbug");
            //runNoPipe();
        }
	}
	return 0;
}