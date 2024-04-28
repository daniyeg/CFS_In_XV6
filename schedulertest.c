#include "types.h"
#include "stat.h"
#include "user.h"

// Busy wait in milliseconds, not accurate at all but will do
void
busywait(int ms)
{
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

// Short burst time
void
ioproc(void)
{
  busywait(10);
}

// Do processes with higher priority get more cpu time?
void
nicetest()
{
  printf(1, "nice test!\n");

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
  sleep(50000);

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

// Do processes with the same priority get the same amount of cpu time?
void
fairnesstest()
{
  printf(1, "fariness test!\n");

  // Spawn a lot of processes
  int pid[60];
  for (int i = 0; i < 60; i++)
  {
    pid[i] = fork();
    if (pid[i] == 0)
      cpuproc();
  }

  // Wait for processes to run for a little bit
  sleep(10000);
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

// Do processes that comes with a short burst time get serviced?
void
bursttest()
{
  printf(1, "burst test!\n");

  // CPU intensive process in the background
  int cproc = fork();
  if (cproc == 0)
  {
    cpuproc();
    exit();
  }
  // Spawn io tasks and measure how long it takes for them to get serviced
  for (int i = 0; i < 50; i++)
  {
    int iproc = fork();
    if (iproc == 0)
    {
      ioproc();
      exit();
    }
    // Measure service time
    int start = uptime();
    // Wait until io process gets serviced
    wait();
    // Manually check how long it took
    printf(1, "io %d got serviced in %d ticks!\n", i, uptime() - start);
  }

  // kill cpu process
  kill(cproc);
  wait();

  printf(1, "burst test done!\n");
}

int
main(void)
{
  fairnesstest();
  nicetest();
  bursttest();
  exit();
}
