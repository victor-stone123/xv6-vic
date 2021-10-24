#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"

void
find_name(char *path, char *name)
{
  char buf[512], *p;
  int fd;
  struct dirent de;
  struct stat st;
  if((fd = open(path, 0)) < 0){
    printf("invalid path\n"); 
    return;
  }

  if(fstat(fd, &st) < 0){
    printf("invalid file\n");
    close(fd);
    return;
  }

  switch(st.type){
  case T_FILE:
    printf("search in file!\n");  
    close(fd);
    return;

  case T_DIR:
    if(strlen(path) + 1 + DIRSIZ + 1 > sizeof buf){
      break;
    }
    strcpy(buf, path);
    p = buf+strlen(buf);
    *p++ = '/';
    while(read(fd, &de, sizeof(de)) == sizeof(de)){
      if(de.inum == 0)
        continue;
      memmove(p, de.name, DIRSIZ);
      p[DIRSIZ] = 0;

      if(stat(buf, &st) < 0){
        continue;
      }
   
     if (st.type == T_FILE && strcmp(name, de.name)==0)
        printf("%s \n",buf);
     if ( strcmp(de.name, ".") == 0 ||  strcmp(de.name, "..") == 0 ) 
        continue;
     if (st.type == T_DIR)
        find_name(buf, name);
   }
    break;
  }
  close(fd);

}

int
main(int argc, char *argv[])
{
  if(argc < 3){
    printf("inputs are missing!\n");
    exit(0);
  }
  find_name(argv[1], argv[2]);
  exit(0);
}
