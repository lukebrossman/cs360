#include <stdio.h>       // for printf()
#include <stdlib.h>      // for exit()
#include <string.h>      // for strcpy(), strcmp(), etc.
#include <libgen.h>      // for basename(), dirname()
#include <fcntl.h>       // for open(), close(), read(), write()

// for stat syscalls
#include <sys/stat.h>
#include <unistd.h>

// for opendir, readdir syscalls
#include <sys/types.h>
#include <dirent.h>

char *args[16], path[256];;

int main(int argc, char *argv[])
{
  if (argc < 3)
  {
    printf("myrcp requires two arguments to run. \n");
    exit(1);
  }
    //print usage and exit;
  return myrcp(argv[1], argv[2]);
}

int myrcp(char *f1, char *f2)
{
  struct stat fileStat1, fileStat2;
  if(stat(f1,&fileStat1) != 0)
  {    
    return -1;
  } 
  else 
  {
    if (S_ISREG(fileStat1.st_mode))
    {
      if (stat(f2, &fileStat2) != 0 || S_ISREG(fileStat2.st_mode))
      {
        return cpf2f(f1, f2);
      }
      else if (stat(f2, &fileStat2) == 0 && S_ISDIR(fileStat2.st_mode))
      {
        return cpf2d(f1,f2);
      }
    } 
    else if (S_ISDIR(fileStat1.st_mode))
    {
      if (stat(f2, &fileStat2) == 0 && S_ISREG(fileStat2.st_mode))
      {
        return -1;
      } 
      else if (stat(f2, &fileStat2) != 0)
      {
        mkdir(f2, ACCESSPERMS);
      }
      printf("d2d \n");
      printf("%s    %s\n", f1, f2);
      return cpd2d(f1, f2);
    }
  }
}

// cp file to file
int cpf2f(char *f1, char *f2)
{
  char path1[256], path2[256];
  printf("%s \n", f1);
  char buf[2048] = {'\0'};
  struct stat fileStat1, fileStat2;
  int f2notexist = lstat(f2, &fileStat2); 
  lstat(f1, &fileStat1);
  if (S_ISLNK(fileStat1.st_mode))
  {
    printf("link file\n");
    if (f2notexist)
    {
      realpath(f1, path1);
      realpath(f2, path2);
      symlink(path1, path2);
      return 0;
    } 
    else
    {
      return -1;
    }
  }
  else if ((fileStat1.st_dev == fileStat2.st_dev) && (fileStat1.st_ino == fileStat2.st_ino))
  {
    printf("NO, same file \n");
    return -1;
  }
  else
  {
    int fd1 = open(f1, O_RDONLY);
    int fd2 = open(f2, O_WRONLY|O_CREAT|O_TRUNC);
    read(fd1, buf, 2048);
    printf("%s \n", buf);
    write(fd2, buf, strlen(buf));
    close(fd1);
    close(fd2);
  }
}
 
int cpf2d(char *f1, char *f2)
{
  DIR * dir = opendir(f2);
  int result;
  struct dirent* dirread;
  int contains;
  char* newfile = f2;
  /*1. search DIR f2 for basename(f1)
     (use opendir(), readdir())*/
  char*x = basename(f1);
  while (dirread = readdir(dir))
  {
    printf("%s  \n", dirread->d_name);
    if (strcmp(dirread->d_name, x) == 0)
    {
      contains = 1;
      break;
    }
  }
  if(!contains)
  {
    strcat(newfile, "/");
    strcat(newfile, x);
    return cpf2f(f1, newfile);
  }
  return -1;
}

int isASubdirectory(char* f1, char* f2)
{
  char path1[256], path2[256];
  realpath(f1, path1);
  realpath(f2, path2); 
  return strncmp(path1, path2, strlen(path1));
}

int cpd2d(char *f1, char *f2)
{
  int result = -1, contains = 0;
  char source[256], dest[256];
  struct stat fileStat1, fileStat2;
  stat(f2, &fileStat2); 
  stat(f1, &fileStat1);
  if ((fileStat1.st_dev == fileStat2.st_dev) && (fileStat1.st_ino == fileStat2.st_ino))
  {
    printf("NO, same directory \n");
    return -1;
  }

  if(isASubdirectory(f1, f2))
  {
    struct dirent* dirread;
    DIR * dir = opendir(f1);
    readdir(dir);
    readdir(dir);
    while (dirread = readdir(dir))
    {
      strcpy(source,f1);
      strcpy(dest, f2);
      strcat(source, "/");
      strcat(dest, "/");
      strcat(source, dirread->d_name);
      strcat(dest, dirread->d_name);       

      result = myrcp(source, dest);
    }
  } 
  else
  {
    printf("NO, destination is a subdirectory of the source! \n");
    return -1;
  }
  return result;
}