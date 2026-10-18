// Simple grep.  Supports the ^ . * $ operators plus the -i, -n and -v flags.

#include "types.h"
#include "stat.h"
#include "user.h"

#define BUFSZ 1024               // longest line we can handle (minus one)

char buf[BUFSZ];
char lbuf[BUFSZ];                // lower-cased copy of the current line (for -i)
int iflag = 0;                   // -i: ignore case
int nflag = 0;                   // -n: prefix matches with their line number
int vflag = 0;                   // -v: print lines that do NOT match
int showname = 0;                // set when searching more than one file
char *fname = 0;                 // name of the file being searched
int lineno;                      // line number within the current file

int match(char*, char*);

// Copy src to dst, converting A-Z to a-z.
void
tolowerstr(char *dst, char *src)
{
  while(*src){
    if(*src >= 'A' && *src <= 'Z')
      *dst++ = *src + ('a' - 'A');
    else
      *dst++ = *src;
    src++;
  }
  *dst = 0;
}

// Handle one complete line (already NUL-terminated).
void
doline(char *pattern, char *line)
{
  int hit;

  lineno++;
  if(iflag){
    tolowerstr(lbuf, line);       // match against the lower-cased copy...
    hit = match(pattern, lbuf);
  } else {
    hit = match(pattern, line);
  }
  if(vflag)
    hit = !hit;
  if(!hit)
    return;
  if(showname)
    printf(1, "%s:", fname);
  if(nflag)
    printf(1, "%d:", lineno);
  printf(1, "%s\n", line);        // ...but always print the original line
}

void
grep(char *pattern, int fd)
{
  int n, m;
  char *p, *q;

  lineno = 0;
  m = 0;
  while((n = read(fd, buf+m, BUFSZ-m-1)) > 0){
    m += n;
    buf[m] = '\0';
    p = buf;
    while((q = strchr(p, '\n')) != 0){
      *q = 0;
      doline(pattern, p);
      p = q+1;
    }
    // Keep any unfinished line at the front of buf for the next read.
    m -= p - buf;
    memmove(buf, p, m);
    if(m >= BUFSZ-1){             // a single line longer than the buffer
      buf[m] = '\0';
      doline(pattern, buf);
      m = 0;
    }
  }
  if(m > 0){                      // last line had no trailing newline
    buf[m] = '\0';
    doline(pattern, buf);
  }
}

int
main(int argc, char *argv[])
{
  int fd, i, first;
  char *pattern, *c;
  char lpat[BUFSZ];

  // Leading arguments like -i, -n, -v, or clusters such as -in, are flags.
  for(first = 1; first < argc && argv[first][0] == '-' && argv[first][1] != 0; first++){
    for(c = argv[first] + 1; *c; c++){
      if(*c == 'i')
        iflag = 1;
      else if(*c == 'n')
        nflag = 1;
      else if(*c == 'v')
        vflag = 1;
      else{
        printf(2, "grep: unknown option -%c\n", *c);
        exit();
      }
    }
  }

  if(first >= argc){
    printf(2, "usage: grep [-inv] pattern [file ...]\n");
    exit();
  }
  pattern = argv[first++];
  if(strlen(pattern) >= BUFSZ){
    printf(2, "grep: pattern too long\n");
    exit();
  }
  if(iflag){
    tolowerstr(lpat, pattern);    // lower-case the pattern once, up front
    pattern = lpat;
  }

  if(first >= argc){              // no file given: read standard input
    grep(pattern, 0);
    exit();
  }

  showname = (argc - first > 1);
  for(i = first; i < argc; i++){
    if((fd = open(argv[i], 0)) < 0){
      printf(1, "grep: cannot open %s\n", argv[i]);
      exit();
    }
    fname = argv[i];
    grep(pattern, fd);
    close(fd);
  }
  exit();
}

// Regexp matcher from Kernighan & Pike,
// The Practice of Programming, Chapter 9.

int matchhere(char*, char*);
int matchstar(int, char*, char*);

int
match(char *re, char *text)
{
  if(re[0] == '^')
    return matchhere(re+1, text);
  do{  // must look at empty string
    if(matchhere(re, text))
      return 1;
  }while(*text++ != '\0');
  return 0;
}

// matchhere: search for re at beginning of text
int matchhere(char *re, char *text)
{
  if(re[0] == '\0')
    return 1;
  if(re[1] == '*')
    return matchstar(re[0], re+2, text);
  if(re[0] == '$' && re[1] == '\0')
    return *text == '\0';
  if(*text!='\0' && (re[0]=='.' || re[0]==*text))
    return matchhere(re+1, text+1);
  return 0;
}

// matchstar: search for c*re at beginning of text
int matchstar(int c, char *re, char *text)
{
  do{  // a * matches zero or more instances
    if(matchhere(re, text))
      return 1;
  }while(*text!='\0' && (*text++==c || c=='.'));
  return 0;
}
