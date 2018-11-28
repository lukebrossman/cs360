#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <time.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>

char** TokenizeFP(char input[], int index, char **result)
{
    char strtemp[200];
    char temp[index + 1][32];
    int i = 0;
    printf("debug \n");
    printf("%x \n", input);
    for (i = 0; i < index; i++)
    {
        strcpy(temp[i], result[i]);
    }
    if (*input == '\0')
    {
        printf("end tokenize\n");
        return temp;
    }
    else
    {
        printf("debugI \n"); 
        sscanf(input, "%[^/]", strtemp);
        strcpy(temp[i], strtemp);
        input += 1;
        printf("%s \n", temp[index]);
        if (*input != '\0')
        {
            input += strlen(strtemp);
        }
        printf("debug \n");
        index += 1;
        printf("%d \n", index);
        return TokenizeFP(input, index, temp);
    }
}

char ** filePathTokenize(char input[])
{
    char temp[1][32] = {'\0'};
    return TokenizeFP(input, 0, temp);
}

int main()
{
    char test[16] = "a/b/c/d";
    char **test2;
    test2 = filePathTokenize(test);
    int i =0;

    for (i = 0; i < 4; i++)
    {
        printf("%s \n", test2[i]);
    }
}