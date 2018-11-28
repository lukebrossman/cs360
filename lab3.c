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
char *path[64], **env, redirectPath[100], pathtemp[100][100];
char cmdPath[100];
int pid, status, pipeCmd, redirect, continueProgram, exitcode;

int tokenizePath(char input[], int index)
{
    char temp[200];
    if (*input == '\0')
    {
        path[index] = 0;
        return index;
    }
    else
    {
        sscanf(input, "%[^:]", pathtemp[index]);
        path[index] = pathtemp[index];
        strcat(path[index], "/");
        input += strlen(pathtemp[index]);
        tokenizePath(input, index + 1);
    }
}

int GetPath()
{
    char* temp = getenv("PATH");
    temp[strlen(temp) - 1] = '\0';
    tokenizePath(temp, 0);
}

void tokenize2(char input[], int index)
{
     if (*input == '\0')
    {
        sscanf(input, "%s", tempargs2[index]);
        myargv2[index] = 0;
    }
    else 
    {
        sscanf(input, "%s ", tempargs2[index]);
        myargv2[index] = tempargs2[index];
        input += 1 + strlen(tempargs2[index]);
        tokenize2(input, index + 1);
    }
}

int tokenize(char input[], int index)
{
    if (*input == '\0')
    {
        sscanf(input, "%s", tempargs[index]);
        myargv1[index] = 0;
        return index;
    }
    else if (*input == '|')
    {
        pipeCmd = TRUE;
        myargv1[index] = 0;
        tokenize2(input + 1, 0);
        return index;
    }
    else 
    {
        sscanf(input, "%s ", tempargs[index]);
        switch (tempargs[index][0])
        {
            case '>':
                if (strcmp(tempargs[index], ">>") == 0)
                {
                    printf(">>\n");
                    sscanf(input + 3, "%s", redirectPath);
                    redirect = 3;
                    input += 3 + strlen(redirectPath);
                }
                else
                {
                    printf(">\n");
                    sscanf(input + 2, "%s", redirectPath);
                    redirect = 1;
                    input += 2 + strlen(redirectPath);
                }
                break;

            case '<':
                printf("<\n");
                sscanf(input + 2, "%s", redirectPath);
                redirect = 2;
                input += 2 + strlen(redirectPath);
                break;

            default:
                myargv1[index] = tempargs[index];
                input += 1 + strlen(tempargs[index]);
                break;
        }
        tokenize(input, index + 1);
    }
}

void handleRedirect()
{
    switch (redirect)
    {
        case 1:
            close(1);
            open(redirectPath, O_WRONLY | O_CREAT);
            break;

        case 2:
            close(0);
            open(redirectPath, O_RDONLY);
            break;

        case 3:
            close(1);
            open(redirectPath, O_APPEND | O_WRONLY | O_CREAT);   
            break;

        default:
            break;
    }
}

void runStdCmd(char * myargv[64])
{
    int i;
    pid = fork();
    if (pid){
        pid=wait(&status);
        printf("child %d dies, exitcode: %d \n", getpid(), exitcode);
    }
    else
    {
        handleRedirect();
        exitcode = execve(myargv[0], myargv, env);
        if (exitcode)
        {  
            for (i = 0; exitcode == -1 || (path[i] != NULL); i++)
            {
                strcpy(cmdPath, path[i]);
                strcat(cmdPath, myargv[0]); 
				printf("%s\n", cmdPath);
                exitcode = execve(cmdPath, myargv, env);
            }
            exit(1);
        }
    }
}

int runPipeCmd()
{
    int pd[2], i, ret1, ret2;
    pipeCmd = 0;
    pipe(pd);            
                    
    if (fork() == 0)
    {       
        dup2(pd[1], 1); 
        close(pd[0]);   
        close(pd[1]); 
        handleRedirect();
        exitcode = execve(myargv1[0], myargv1, env);
        if (exitcode)
        {  
            for (i = 0; exitcode == -1 || (path[i] != NULL); i++)
            {
                strcpy(cmdPath, path[i]);
                strcat(cmdPath, myargv1[0]); 
                exitcode = execve(cmdPath, myargv1, env);
            }
        }
    }
    if (fork() == 0)
    {            
        dup2(pd[0], 0); 
        close(pd[0]);   
        close(pd[1]); 
        handleRedirect();  
        exitcode = execve(myargv2[0], myargv2, env);
        if (exitcode)
        {  
            for (i = 0; exitcode == -1 || (path[i] != NULL); i++)
            {
                strcpy(cmdPath, path[i]);
                strcat(cmdPath, myargv2[0]); 
                exitcode = execve(cmdPath, myargv2, env);
            }
        } 
    }
    close(pd[0]);   
    close(pd[1]); 
    wait(&ret1);
    wait(&ret2);
}

void runNoPipe()
{
    if (strcmp(myargv1[0], "exit") == 0)
    {
        continueProgram = FALSE;
    }
    else if (strcmp(myargv1[0], "cd") == 0)
    {
    if (myargv1[1] == 0)
    {
        printf("HOME: %s \n", getenv("HOME"));
        printf("CWD: %d \n", chdir(getenv("HOME")));
    } 
    else
    {
        printf("CWD: %d \n", chdir(myargv1[1]));
    }
    }
    else 
    {
    runStdCmd(myargv1);
    redirect = 0;
    }
    
}

int main(int argc, char *argv[], char **envp)
{
    GetPath();
    int i;
    char line[128];
    continueProgram = TRUE;
    env = envp;
	while (continueProgram)
	{
		printf("Input a command : ");
		fgets(line, 127, stdin);
		line[strlen(line) - 1] = '\0';
        tokenize(line, 0);
        if (pipeCmd)
        {
            printf("pipe\n");
            runPipeCmd();
        }
        else
        {
            runNoPipe();
        }
	}
	return 0;
}