
# OS-4021
<p>xv6 is a re-implementation of Dennis Ritchie's and Ken Thompson's Unix
Version 6 (v6).<br>
This repository was created as a project for OS course in Fall of 1402.
This repository includes the following improvements which have been implemented
in separate phases.</p>

1. Completely Fair Scheduler (CFS) implementation in xv6.
2. TBC...

## Completely Fair Scheduler (CFS)

The CFS works by maintaining a model of "virtual runtime" for each process. Each process is assigned a priority value based on its dynamic behavior and the amount of CPU time it has consumed. The CFS aims to distribute the CPU fairly among all processes, regardless of their priority.

When a new process is created or an existing process becomes eligible for execution, the CFS adds it to a red-black tree data structure called the "rbtree". The position of each process in the tree is determined by its virtual runtime, with processes that have consumed less CPU time being closer to the left.

During scheduling, the CFS selects the process with the smallest virtual runtime from the rbtree for execution. The selected process is then allocated a maximum time slice (known as the "timeslice" or "quantum") to run on the CPU. The length of the time slice is calculated using the following formula:

$$timeslice_k = \frac{weight_k}{\sum_{i=0}^{n-1}weight_i} \times period$$

"sched latency" and "min granularity" are scheduler tunables which decide the scheduler period, the period in which all run queue tasks are scheduled at least once. min granularity decides the minimum time a task will be be allowed to run on CPU before being pre-empted out. The way sched latency and min_granularity determine the scheduelr period is as follows:

If $number\ of\ runnable\ processes > \frac{sched\ latency}{min\ granularity}$
then $period = number\ of\ runnable\ processes \times min\ granularity$
otherwise $period = sched\ latency$.

After a process finishes its time slice or when a process with a lower virtual runtime is in the tree, it is reinserted into the rbtree with an updated virtual runtime. This ensures that processes that have consumed more CPU time are placed deeper in the tree, giving them a lower priority for the next scheduling decision. the virtual runtime of a process is calculated by the following formula:

$$vruntime_i = vruntime_i + \frac{weight_0}{weight_i} \times runtime_i$$

The weight of each process is determined by its nice value ranging from -20 to 19. The weight is derived from the following formula:

$$weight_i = \frac{1024}{1.25^{nice}}$$

## Building and Testing

<p align="justify">
To build xv6 on an x86 ELF machine (like Linux or FreeBSD), run
<code>make</code>. On non-x86 or non-ELF machines (like OS X, even on x86),
you will need to install a cross-compiler gcc suite capable of producing x86
ELF binaries (see 
<a href="https://pdos.csail.mit.edu/6.828/">https://pdos.csail.mit.edu/6.828/</a>).
Then run <code>make TOOLPREFIX=i386-jos-elf-</code>. Now install the QEMU
PC simulator and run <code>make qemu</code>.
</p>

<p>The code in the files that constitute xv6 is
Copyright 2006-2018 Frans Kaashoek, Robert Morris, and Russ Cox.</p>
