#include "types.h"
#include "stat.h"
#include "user.h"

// Busy wait in milliseconds, not accurate at all but will do
void busywait(int ms) {
  int wait = ms * 200000;
  volatile int temp;
  for (int i = 0; i < wait; i++)
  {
    temp *= 2;
    temp /= 2;
  }
}

// Really long burst time (infinite busy wait)
void
cpuproc(void)
{
  for(;;){}
}

void
nicetest()
{
  printf(1, "nice test done!\n");

  // Spawn high priority process
  int pid1 = fork();
  if (pid1 == 0)
  {
    nice(-10);
    cpuproc();
    exit();
  }
  // Spawn low priority process
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

  // Manually check high priority one has ran longer
  ps();

  // Kill children
  kill(pid1);
  kill(pid2);

  // Wait for children (prevent zombies)
  wait();
  wait();

  printf(1, "nice test done!\n");
}

void
fairnesstest() {
  printf(1, "fariness test!\n");

  // Spawn a lot of processes
  int pid[60];
  for (int i = 0; i < 60; i++) {
    pid[i] = fork();
    if (pid[i] == 0)
      cpuproc();
  }

  // Wait for processes to run for a little bit
  busywait(1000);
  // Manually check how long each one has run
  ps();
  // Kill children
  for (int i = 0; i < 60; i++)
    kill(pid[i]);
  // Wait for childen
  for (int i = 0; i < 60; i++)
    wait();

  printf(1, "fariness test done!\n");
}

int
main(void)
{
  fairnesstest();
  nicetest();
  exit();
}
