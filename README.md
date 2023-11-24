
# OS-4021
<p>xv6 is a re-implementation of Dennis Ritchie's and Ken Thompson's Unix
Version 6 (v6).<br>
This repository was created as a project for OS course in Fall of 1402.
This repository includes the following improvements which have been implemented
in separate phases.</p>

1. Completely Fair Scheduler (CFS) implementation in xv6.
2. TBC...

## Completely Fair Scheduler (CFS)


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
