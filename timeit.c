// timeit: run a command and report how many ticks it took.
//   usage: timeit command [args ...]
// Used to compare schedulers, e.g.  timeit stressfs   or   timeit ls
#include "types.h"
#include "stat.h"
#include "user.h"

int
main(int argc, char *argv[])
{
  int start, pid;

  if(argc < 2){
    printf(2, "usage: timeit command [args ...]\n");
    exit();
  }
  start = uptime();
  pid = fork();
  if(pid < 0){
    printf(2, "timeit: fork failed\n");
    exit();
  }
  if(pid == 0){
    exec(argv[1], argv + 1);
    printf(2, "timeit: exec %s failed\n", argv[1]);
    exit();
  }
  wait();
  printf(1, "timeit: %s took %d ticks\n", argv[1], uptime() - start);
  exit();
}
