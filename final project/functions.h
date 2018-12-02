#ifndef FUNCTIONS_H
#define FUNCTIONS_H

#include "type.h"

extern command cmdtable[];

/* startup functions */
void init(char *tempcmd);
void mount_root(char *tempcmd);
void find_and_execute_command(char *tempcmd);

/* Commands */
int CMD_MENU();
void CMD_QUIT();
int CMD_CD();
int CMD_LS();
int CMD_MKDIR();
int CMD_CREAT();
int CMD_RMDIR();
int CMD_RM();
int CMD_LINK();
int CMD_UNLINK();
int CMD_SYMLINK();
int CMD_TOUCH();
int CMD_CHMOD();
int CMD_STAT();
int CMD_PWD();

int do_ls(char *path);
void do_pwd(MINODE *wd);
int my_mkdir(MINODE *pip, char *name);
int my_creat(MINODE *pip, char *name);
int do_stat(char *path, struct stat *stPtr);
int do_unlink(int forcerm);

/* helper functions */
void printFile(MINODE *mip, char *namebuf);
void printChild(int devicename, MINODE *mp);
int findparent(char *pathn);
int isEmpty(MINODE *mip);
int rm_child(MINODE *parent, char *my_name);
int is_dir(MINODE *mip);
int is_reg(MINODE *mip);


#endif
