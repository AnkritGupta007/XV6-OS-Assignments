#include "types.h"
#include "stat.h"
#include "user.h"

#define MAXBUF  65536
#define MAXLINES 2048

char buf[MAXBUF];
char *lines[MAXLINES];
int nlines = 0;

int rflag = 0, nflag = 0, uflag = 0;

int
mystrcmp(char *a, char *b, int numeric)
{
  if(numeric)
    return atoi(a) - atoi(b);
  return strcmp(a, b);
}

// Simple insertion sort over the array of line pointers.
// Fine for the sizes this assignment tests; swaps pointers, not text.
void
sortlines(void)
{
  int i, j;
  char *key;

  for(i = 1; i < nlines; i++){
    key = lines[i];
    j = i - 1;
    while(j >= 0 && mystrcmp(lines[j], key, nflag) * (rflag ? -1 : 1) > 0){
      lines[j+1] = lines[j];
      j--;
    }
    lines[j+1] = key;
  }
}

int
main(int argc, char *argv[])
{
  int fd = 0;     // default: stdin
  char *fname = 0;
  int i, n, total = 0;

  for(i = 1; i < argc; i++){
    if(strcmp(argv[i], "-r") == 0)
      rflag = 1;
    else if(strcmp(argv[i], "-n") == 0)
      nflag = 1;
    else if(strcmp(argv[i], "-u") == 0)
      uflag = 1;
    else
      fname = argv[i];        // the one non-flag argument is the filename
  }

  if(fname){
    if((fd = open(fname, 0)) < 0){
      printf(2, "sort: cannot open %s\n", fname);
      exit();
    }
  }

  // Read the whole input into buf.
  while((n = read(fd, buf + total, MAXBUF - total)) > 0)
    total += n;
  if(fname)
    close(fd);

  if(total >= MAXBUF){
    printf(2, "sort: input too large\n");
    exit();
  }
  buf[total] = 0;

  // Split into lines.
  lines[nlines++] = buf;
  for(i = 0; i < total; i++){
    if(buf[i] == '\n'){
      buf[i] = 0;
      if(i + 1 < total && nlines < MAXLINES)
        lines[nlines++] = buf + i + 1;
    }
  }
  // Drop a trailing empty "line" caused by a final newline.
  if(nlines > 0 && lines[nlines-1][0] == 0)
    nlines--;

  sortlines();

  for(i = 0; i < nlines; i++){
    if(uflag && i > 0 && strcmp(lines[i], lines[i-1]) == 0)
      continue;               // skip duplicate of the previous (sorted) line
    printf(1, "%s\n", lines[i]);
  }

  exit();
}
