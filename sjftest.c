// sjftest: shows the predicted job lengths handed out by the SJF scheduler
// and the order in which a batch of equally busy children finishes.
//   usage: sjftest [children]        (default 5)
// Under SJF the children finish shortest-predicted-job first.
// Under the default scheduler they take turns and finish in about pid order.
#include "types.h"
#include "stat.h"
#include "user.h"

#define DEFAULT_KIDS 5
#define MAX_KIDS     12
#define WORK_TICKS   30   // each child burns this many ticks of CPU

static void
spin(int n)
{
  int start = uptime();
  while(uptime() - start < n)
    ;
}

int
main(int argc, char *argv[])
{
  int kids = DEFAULT_KIDS;
  int pids[MAX_KIDS];
  int i, me = getpid();

  if(argc > 1)
    kids = atoi(argv[1]);
  if(kids < 1 || kids > MAX_KIDS){
    printf(1, "usage: sjftest [1-%d]\n", MAX_KIDS);
    exit();
  }

  printf(1, "self   pid %d: sjf_job_length = %d\n", me, sjf_job_length(me));
  printf(1, "bogus  pid 9999: sjf_job_length = %d (expect -1)\n", sjf_job_length(9999));

  for(i = 0; i < kids; i++){
    int pid = fork();
    if(pid < 0){
      printf(1, "fork failed\n");
      exit();
    }
    if(pid == 0){
      spin(WORK_TICKS);
      printf(1, "  finished: pid %d  predicted length %d  at tick %d\n",
             getpid(), sjf_job_length(getpid()), uptime());
      exit();
    }
    pids[i] = pid;
  }

  printf(1, "children (predicted lengths):");
  for(i = 0; i < kids; i++)
    printf(1, " %d:%d", pids[i], sjf_job_length(pids[i]));
  printf(1, "\n");

  for(i = 0; i < kids; i++)
    wait();
  printf(1, "after wait, pid %d: sjf_job_length = %d (expect -1)\n",
         pids[0], sjf_job_length(pids[0]));
  exit();
}
