#include "types.h"
#include "stat.h"
#include "user.h"
#include "fs.h"

int showall = 0;   // set to 1 when -a is given

char*
fmtname(char *path, int isdir)
{
  static char buf[DIRSIZ+2];   // +1 for the '/', +1 for the terminator
  char *p;
  int len;

  // Find first character after last slash.
  for(p=path+strlen(path); p >= path && *p != '/'; p--)
    ;
  p++;

  len = strlen(p);
  if(len > DIRSIZ)
    len = DIRSIZ;
  memmove(buf, p, len);
  if(isdir)
    buf[len++] = '/';          // slash goes right after the name...
  while(len < DIRSIZ)
    buf[len++] = ' ';          // ...then pad with spaces
  buf[len] = 0;
  return buf;
}

void
ls(char *path)
{
  char buf[512], *p;
  int fd;
  struct dirent de;
  struct stat st;

  if((fd = open(path, 0)) < 0){
    printf(2, "ls: cannot open %s\n", path);
    return;
  }

  if(fstat(fd, &st) < 0){
    printf(2, "ls: cannot stat %s\n", path);
    close(fd);
    return;
  }

  switch(st.type){
  case T_FILE:
    printf(1, "%s %d %d %d\n", fmtname(path, 0), st.type, st.ino, st.size);
    break;

  case T_DIR:
    if(strlen(path) + 1 + DIRSIZ + 1 > sizeof buf){
      printf(1, "ls: path too long\n");
      break;
    }
    strcpy(buf, path);
    p = buf+strlen(buf);
    *p++ = '/';
    while(read(fd, &de, sizeof(de)) == sizeof(de)){
      if(de.inum == 0)
        continue;
      if(!showall && de.name[0] == '.')   // hide dotfiles unless -a
        continue;
      memmove(p, de.name, DIRSIZ);
      p[DIRSIZ] = 0;
      if(stat(buf, &st) < 0){
        printf(1, "ls: cannot stat %s\n", buf);
        continue;
      }
      printf(1, "%s %d %d %d\n", fmtname(buf, st.type == T_DIR), st.type, st.ino, st.size);
    }
    break;
  }
  close(fd);
}

int
main(int argc, char *argv[])
{
  int i, npaths = 0;

  // Pass 1: find the flag, so "ls -a dir" and "ls dir -a" both work.
  for(i = 1; i < argc; i++){
    if(strcmp(argv[i], "-a") == 0)
      showall = 1;
  }
  // Pass 2: list every argument that isn't the flag.
  for(i = 1; i < argc; i++){
    if(strcmp(argv[i], "-a") == 0)
      continue;
    ls(argv[i]);
    npaths++;
  }
  if(npaths == 0)      // no path given: list the current directory
    ls(".");
  exit();
}
