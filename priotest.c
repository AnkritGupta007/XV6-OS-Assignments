// priotest: exercises set_sched_priority() / get_sched_priority() and shows
// the PRIORITYRR scheduler at work.
//   usage: priotest [work_ticks]        (default 30)
// Six children, two at each priority level (0 = highest), each burn
// work_ticks of CPU.  Expected under SCHEDULER=PRIORITYRR:
//   * the level-0 pair finishes first, then level 1, then level 2;
//   * the two children inside a level finish close together because they
//     take turns (round robin); "slices" counts the times a child was taken
//     off the CPU for a while, roughly work_ticks / quantum.
// Under the default scheduler priorities are ignored and everything
// finishes together.
#include "types.h"
#include "stat.h"
#include "user.h"

#define LEVELS        3     // priority levels exercised (matches PRIORITY_LEVELS)
#define PER_LEVEL     2     // children per level
#define DEFAULT_WORK  30

struct rec {
  int pid;
  int prio;
  int tick;
  int slices;
};

static void
child(int prio, int work, int readyfd, int gofd, int wfd)
{
  struct rec r;
  char c = 'r';
  int last, now, slices = 0;

  if(set_sched_priority(prio) < 0){
    printf(1, "child: set_sched_priority(%d) failed\n", prio);
    exit();
  }
  // Tell the parent we are at our level, then sleep until it says go, so
  // that all the children become runnable at the same moment.
  write(readyfd, &c, 1);
  read(gofd, &c, 1);
  // Burn CPU until we have been RUNNING for `work` ticks.  A jump of more
  // than one tick between two looks at the clock means we were descheduled.
  last = uptime();
  while(ticks_running(getpid()) < work){
    now = uptime();
    if(now - last > 1)
      slices++;
    last = now;
  }
  r.pid = getpid();
  r.prio = get_sched_priority(getpid());
  r.tick = uptime();
  r.slices = slices;
  write(wfd, &r, sizeof(r));
  exit();
}

int
main(int argc, char *argv[])
{
  int work = DEFAULT_WORK;
  int p[2], ready[2], go[2], me = getpid();
  int i, level, n = LEVELS * PER_LEVEL;
  int rv, gv;
  char c = 'g';
  struct rec r;

  if(argc > 1)
    work = atoi(argv[1]);
  if(work < 1){
    printf(1, "usage: priotest [work_ticks]\n");
    exit();
  }

  // ---- 1. the system calls themselves ----
  // (each call is made on its own line: printf's arguments are evaluated
  // in no particular order, which would scramble the before/after values)
  gv = get_sched_priority(me);
  printf(1, "get_sched_priority(self)      = %d (default level)\n", gv);
  rv = set_sched_priority(0);
  gv = get_sched_priority(me);
  printf(1, "set_sched_priority(0)         = %d, now get = %d\n", rv, gv);
  rv = set_sched_priority(LEVELS);
  gv = get_sched_priority(me);
  printf(1, "set_sched_priority(%d)         = %d (expect -1, level unchanged: %d)\n", LEVELS, rv, gv);
  rv = set_sched_priority(-1);
  printf(1, "set_sched_priority(-1)        = %d (expect -1)\n", rv);
  rv = get_sched_priority(9999);
  printf(1, "get_sched_priority(9999)      = %d (expect -1)\n", rv);
  set_sched_priority(1);
  gv = get_sched_priority(me);
  printf(1, "back at level                 = %d\n", gv);

  // ---- 2. scheduling order ----
  if(pipe(p) < 0 || pipe(ready) < 0 || pipe(go) < 0){
    printf(1, "pipe failed\n");
    exit();
  }
  printf(1, "starting %d children, %d ticks of CPU each...\n", n, work);
  for(level = 0; level < LEVELS; level++){
    for(i = 0; i < PER_LEVEL; i++){
      int pid = fork();
      if(pid < 0){
        printf(1, "fork failed\n");
        exit();
      }
      if(pid == 0){
        close(p[0]);
        close(ready[0]);
        close(go[1]);
        child(level, work, ready[1], go[0], p[1]);
      }
    }
  }
  close(p[1]);
  close(ready[1]);
  close(go[0]);
  for(i = 0; i < n; i++)          // wait until every child is at its level
    read(ready[0], &c, 1);
  for(i = 0; i < n; i++)          // ...then release them all together
    write(go[1], &c, 1);
  for(i = 0; i < n; i++)
    wait();

  printf(1, "finishing order (first to last):\n");
  while(read(p[0], &r, sizeof(r)) == sizeof(r))
    printf(1, "  pid %d  priority %d  finished at tick %d  slices %d\n",
           r.pid, r.prio, r.tick, r.slices);
  close(p[0]);
  rv = get_sched_priority(me + 1);
  printf(1, "after wait, get_sched_priority(first child) = %d (expect -1)\n", rv);
  exit();
}
