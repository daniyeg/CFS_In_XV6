#include "types.h"
#include "stat.h"
#include "user.h"

void
cpuproc(void)
{
  for(;;){}
}

void
nicetest()
{
  int pid1 = fork();
  if (pid1 == 0)
  {
    nice(-10);
    cpuproc();
    exit();
  }
  int pid2 = fork();
  if (pid2 == 0)
  {
    nice(10);
    cpuproc();
    exit();
  }

  printf(1, "pid1:%d pid2:%d\n", pid1, pid2);

  // Busy wait
  // About 5 seconds
  volatile int temp;
  for (int i = 0; i < 1000000000; i++)
  {
    temp *= 2;
    temp /= 2;
  }

  // Get process stats after waiting
  ps();

  // Kill forks
  kill(pid1);
  kill(pid2);
}

int
main(void)
{
  nicetest();
  exit();
}
