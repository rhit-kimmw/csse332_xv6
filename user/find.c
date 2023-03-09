#include "kernel/types.h"
#include "user/user.h"
#include "kernel/fs.h"
#include "kernel/stat.h"

void find(char *dir_name, char *file_name);

int main(int argc, char **argv)
{
  /* TODO: Insert your code here. */
  if(argc != 3)
  {
    printf("usage: find <directory> <filename>\n");
    exit(1);
  }

  find(argv[1], argv[2]);

  exit(0);
}

void find(char *dir_name, char *file_name)
{
  char buf[512], *p; 
  int fd;
  struct stat st;
  struct dirent de;
  
  if((fd = open(dir_name, 0)) < 0)
  {
    fprintf(2, "find: cannot open %s\n", dir_name);
    return;
  }

  if(fstat(fd, &st) < 0){
    fprintf(2, "ls: cannot stat %s\n", dir_name);
    close(fd);
    return;
  }

  switch(st.type)
  {

    case T_DEVICE:
      break;
    case T_FILE:
      break;
    case T_DIR:
      if(strlen(dir_name) + 1 + DIRSIZ + 1 > sizeof buf){
        fprintf(2, "find: path too long\n");
        break;
      }
      strcpy(buf, dir_name);
      p = buf+strlen(buf);
      *p++ = '/';
      while(read(fd, &de, sizeof(de)) == sizeof(de)){
        if(de.inum == 0)
          continue;
        memmove(p, de.name, DIRSIZ);
        p[DIRSIZ] = 0;
        if(stat(buf, &st) < 0){
          fprintf(2, "find: cannot stat %s\n", buf);
          continue;
        }
        if(strcmp(de.name, ".") == 0 || strcmp(de.name, "..") == 0)
          continue;
        if(st.type == T_FILE && strcmp(de.name, file_name) == 0)
          printf("%s\n", buf);
        else if(st.type == T_DIR)
          find(buf, file_name);
      }
      break;
  }

}