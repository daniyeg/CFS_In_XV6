#include "types.h"
#include "stat.h"
#include "user.h"

void busywait(int ms) {
  int wait = ms * 200000;
  volatile int temp;
  for (int i = 0; i < wait; i++)
  {
    temp *= 2;
    temp /= 2;
  }
}

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

  // Wait for processes to run for a little bit
  busywait(5000);

  // Get process stats after waiting
  ps();

  // Kill forks
  kill(pid1);
  kill(pid2);
}

void
fairnesstest() {
  int pid[60];
  for (int i = 0; i < 60; i++) {
    pid[i] = fork();
    if (pid[i] == 0)
      cpuproc();

    // Wait between each fork
    busywait(100);
  }

  // Wait for processes to run for a little bit
  busywait(1000);
  // Get process stats
  ps();
  // Kill children
  for (int i = 0; i < 60; i++)
    kill(pid[i]);
}

int
main(void)
{
  fairnesstest();
  nicetest();
  exit();
}
