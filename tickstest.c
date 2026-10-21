#include "types.h"
#include "stat.h"
#include "user.h"

// Burn CPU (without sleeping) for about n ticks of wall-clock time.
void
spin(int n)
{
  int start = uptime();

  while(uptime() - start < n)
    ;
}

int
main(int argc, char *argv[])
{
  int pid, me;

  me = getpid();

  printf(1, "Test 1: the calling process (pid %d)\n", me);
  printf(1, "  before spinning:     %d ticks\n", ticks_running(me));
  spin(50);
  printf(1, "  after spinning ~50:  %d ticks (expect about 50)\n", ticks_running(me));

  printf(1, "Test 2: another process\n");
  pid = fork();
  if(pid < 0){
    printf(2, "tickstest: fork failed\n");
    exit();
  }
  if(pid == 0){
    spin(30);                  // child burns about 30 ticks, then exits
    exit();
  }
  printf(1, "  child %d just after fork: %d ticks (expect 0)\n", pid, ticks_running(pid));
  sleep(40);                   // let the child finish; it stays a zombie until wait()
  printf(1, "  child %d after it ran:    %d ticks (expect about 30)\n", pid, ticks_running(pid));
  wait();
  printf(1, "  child %d after wait():    %d (expect -1, it is gone)\n", pid, ticks_running(pid));

  printf(1, "Test 3: processes that mostly sleep\n");
  printf(1, "  init (pid 1): %d ticks\n", ticks_running(1));
  printf(1, "  sh   (pid 2): %d ticks\n", ticks_running(2));

  printf(1, "Test 4: pids that do not exist\n");
  printf(1, "  pid 9999: %d (expect -1)\n", ticks_running(9999));
  printf(1, "  pid -1:   %d (expect -1)\n", ticks_running(-1));
  printf(1, "  pid 0:    %d (expect -1)\n", ticks_running(0));

  exit();
}
