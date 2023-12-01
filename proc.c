#include "types.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "x86.h"
#include "proc.h"
#include "spinlock.h"

struct {
  struct spinlock lock;
  struct proc proc[NPROC];
} ptable;

struct redBlackTree{
  struct proc *root;         // Root node
  struct proc *min_vruntime; // Node wih minimum vruntime for O(1) access
  struct spinlock lock;      // Spinlock for the tree
  int count;                 // Total amount of nodes in rbtree
  int rbTreeWeight;          // Total sum of node weights
  int period;                // Scheduler period
} rbtree;

static int min_granularity = 2; // Minimum time a task is allowed to run, tunable
static int sched_latency = 2*8; // Must be multiple of min_granularity, tunable

static struct proc *initproc;

int nextpid = 1;
extern void forkret(void);
extern void trapret(void);

static void wakeup1(void *chan);

// --------------------------------------------
// Red Black Tree functions
struct proc*
retriveGrandparent(struct proc *node)
{
  //TODO
  return 0;
}

struct proc*
retriveUncle(struct proc *node)
{
  //TODO
  return 0;
}

struct proc*
retriveMinimumVRuntime(struct proc *node)
{
  //TODO
  return 0;
}

void
rotateLeft(struct redBlackTree *tree, struct proc *node)
{
  //TODO
}

void
rotateRight(struct redBlackTree *tree, struct proc *node)
{
  //TODO
}

void
rbinsertFixup(struct redBlackTree *tree, struct proc *node)
{
  //TODO
}

void
rbinsert(struct redBlackTree *tree, struct proc *node)
{
  //TODO
}

void
rbdeleteFixup(struct redBlackTree *tree, struct proc *node)
{
  //TODO
}

void
rbdelete(struct redBlackTree *tree, struct proc *node)
{
  //TODO
}

// --------------------------------------------

// Nice value to weight value conversion.
// Roughly equivalent to 1024 / (1.25 ^ nice_value)
// Nice value here is in the same range as nice value in linux kernel, between -20 and 19.
static const int prio_to_weight[40] = {
 /* -20 */     88761,     71755,     56483,     46273,     36291,
 /* -15 */     29154,     23254,     18705,     14949,     11916,
 /* -10 */      9548,      7620,      6100,      4904,      3906,
 /*  -5 */      3121,      2501,      1991,      1586,      1277,
 /*   0 */      1024,       820,       655,       526,       423,
 /*   5 */       335,       272,       215,       172,       137,
 /*  10 */       110,        87,        70,        56,        45,
 /*  15 */        36,        29,        23,        18,        15,
};

// Initialize the red black tree
void
rbinit(struct redBlackTree *tree, char *lockName)
{
  initlock(&tree->lock, lockName);
  tree->root = 0;
  tree->min_vruntime = 0;
  tree->count = 0;
  tree->rbTreeWeight = 0;
  tree->period = sched_latency; // Set initial period to sched_latency
}

// Update virtual runtime based on the current runtime and set current runtime to 0.
// Since we are updating values of a process, ptable lock must be held before calling.
void
updateruntimes(struct proc *p)
{
  p->vruntime = p->vruntime +
      (prio_to_weight[20] / p->weightValue) * p->cruntime;
  p->cruntime = 0;
}

// Update scheduler period.
// Since we are updating values of a tree, tree lock must be held before calling.
void
updateperiod(struct redBlackTree *tree)
{
  if (tree->count > sched_latency / min_granularity)
    tree->period = tree->count * min_granularity;
  else
    tree->period = sched_latency;
}

// Get the process to schedule next.
// The returned process is removed from the tree.
// Returns 0 (null) if failed to get min_vruntime.
struct proc*
getproc(struct redBlackTree *tree)
{
  struct proc *next_process;

  acquire(&tree->lock);

  if (tree->count != 0) // If the tree isn't empty
  {
    // Get process with minimum vruntime and "pop" it out of the tree
    next_process = tree->min_vruntime;
    rbdelete(tree, tree->min_vruntime);
    // Determine new process with smallest vruntime.
    tree->min_vruntime = retriveMinimumVRuntime(tree->root);

    // Return NULL if the process is not runnable
    if(next_process->state != RUNNABLE)
    {
      release(&tree->lock);
      return 0;
    }

    // Update sched period before calculating the maximum timeslice.
    updateperiod(tree);
    // Calculate maximum timesclie of the process
    next_process->timeslice = tree->period *
        (next_process->weightValue / tree->rbTreeWeight);

    release(&tree->lock);
    return next_process;
  }

  release(&tree->lock);
  return 0;
}

// Insert runnable process into the red black tree.
// Returns 0 if insert was successful, -1 if not.
int
insertproc(struct redBlackTree *tree, struct proc *p)
{
  acquire(&tree->lock);

  if (tree->count < NPROC) // If the tree isn't full
  {
    // Determine the weight of the process based on its nice value.
    p->weightValue = prio_to_weight[p->niceValue + 20];

    // Insert process into the tree.
    // All properties that change about the tree is done within rbinsert.
    rbinsert(tree, p);

    release(&tree->lock);
    return 0;
  }

  release(&tree->lock);
  return -1;
}

// Check whether or not we should preempt the current task.
// Return 0 if we don't need to preempt, otherwise return 1.
int
checkpreempt(struct proc *curproc, struct proc *min_vruntime)
{
  // If the process has run less than min_granularity, don't preempt
  if (curproc->cruntime < min_granularity)
    return 0;

  // Check if the current process has exceeded its timeslice
  if (curproc->cruntime >= curproc->timeslice)
    return 1;

  // Check if we should switch to min_vruntime in the tree.
  // Happens if min_vruntime is a runnable task with a lower virtual runtime.
  if (min_vruntime != 0 && min_vruntime->state == RUNNABLE &&
      curproc->vruntime > min_vruntime->vruntime)
      return 1;

  return 0;
}

void
pinit(void)
{
  initlock(&ptable.lock, "ptable");
  rbinit(&rbtree, "rbtree");
}

// Must be called with interrupts disabled
int
cpuid() {
  return mycpu()-cpus;
}

// Must be called with interrupts disabled to avoid the caller being
// rescheduled between reading lapicid and running through the loop.
struct cpu*
mycpu(void)
{
  int apicid, i;

  if(readeflags()&FL_IF)
    panic("mycpu called with interrupts enabled\n");

  apicid = lapicid();
  // APIC IDs are not guaranteed to be contiguous. Maybe we should have
  // a reverse map, or reserve a register to store &cpus[i].
  for (i = 0; i < ncpu; ++i) {
    if (cpus[i].apicid == apicid)
      return &cpus[i];
  }
  panic("unknown apicid\n");
}

// Disable interrupts so that we are not rescheduled
// while reading proc from the cpu structure
struct proc*
myproc(void) {
  struct cpu *c;
  struct proc *p;
  pushcli();
  c = mycpu();
  p = c->proc;
  popcli();
  return p;
}

//PAGEBREAK: 32
// Look in the process table for an UNUSED proc.
// If found, change state to EMBRYO and initialize
// state required to run in the kernel.
// Otherwise return 0.
static struct proc*
allocproc(void)
{
  struct proc *p;
  char *sp;

  acquire(&ptable.lock);

  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++)
    if(p->state == UNUSED)
      goto found;

  release(&ptable.lock);
  return 0;

found:
  p->state = EMBRYO;
  p->pid = nextpid++;

  release(&ptable.lock);

  // Allocate kernel stack.
  if((p->kstack = kalloc()) == 0){
    p->state = UNUSED;
    return 0;
  }
  sp = p->kstack + KSTACKSIZE;

  // Leave room for trap frame.
  sp -= sizeof *p->tf;
  p->tf = (struct trapframe*)sp;

  // Set up new context to start executing at forkret,
  // which returns to trapret.
  sp -= 4;
  *(uint*)sp = (uint)trapret;

  sp -= sizeof *p->context;
  p->context = (struct context*)sp;
  memset(p->context, 0, sizeof *p->context);
  p->context->eip = (uint)forkret;

  // Set up variables that are used by CFS.
  p->vruntime = 0;
  p->cruntime = 0;
  p->timeslice = 0;
  p->niceValue = 0;
  p->weightValue = 0;

  p->left = 0;
  p->right = 0;
  p->rbparent = 0;
  p->color = RED;

  return p;
}

//PAGEBREAK: 32
// Set up first user process.
void
userinit(void)
{
  struct proc *p;
  extern char _binary_initcode_start[], _binary_initcode_size[];

  p = allocproc();

  initproc = p;
  if((p->pgdir = setupkvm()) == 0)
    panic("userinit: out of memory?");
  inituvm(p->pgdir, _binary_initcode_start, (int)_binary_initcode_size);
  p->sz = PGSIZE;
  memset(p->tf, 0, sizeof(*p->tf));
  p->tf->cs = (SEG_UCODE << 3) | DPL_USER;
  p->tf->ds = (SEG_UDATA << 3) | DPL_USER;
  p->tf->es = p->tf->ds;
  p->tf->ss = p->tf->ds;
  p->tf->eflags = FL_IF;
  p->tf->esp = PGSIZE;
  p->tf->eip = 0;  // beginning of initcode.S

  safestrcpy(p->name, "initcode", sizeof(p->name));
  p->cwd = namei("/");

  // this assignment to p->state lets other cores
  // run this process. the acquire forces the above
  // writes to be visible, and the lock is also needed
  // because the assignment might not be atomic.
  acquire(&ptable.lock);

  // Make the process runnable.
  p->state = RUNNABLE;
  // Insert the process into the tree.
  insertproc(&rbtree, p);

  release(&ptable.lock);
}

// Grow current process's memory by n bytes.
// Return 0 on success, -1 on failure.
int
growproc(int n)
{
  uint sz;
  struct proc *curproc = myproc();

  sz = curproc->sz;
  if(n > 0){
    if((sz = allocuvm(curproc->pgdir, sz, sz + n)) == 0)
      return -1;
  } else if(n < 0){
    if((sz = deallocuvm(curproc->pgdir, sz, sz + n)) == 0)
      return -1;
  }
  curproc->sz = sz;
  switchuvm(curproc);
  return 0;
}

// Create a new process copying p as the parent.
// Sets up stack to return as if from system call.
// Caller must set state of returned proc to RUNNABLE.
int
fork(void)
{
  int i, pid;
  struct proc *np;
  struct proc *curproc = myproc();

  // Allocate process.
  if((np = allocproc()) == 0){
    return -1;
  }

  // Copy process state from proc.
  if((np->pgdir = copyuvm(curproc->pgdir, curproc->sz)) == 0){
    kfree(np->kstack);
    np->kstack = 0;
    np->state = UNUSED;
    return -1;
  }
  np->sz = curproc->sz;
  np->parent = curproc;
  *np->tf = *curproc->tf;

  // Clear %eax so that fork returns 0 in the child.
  np->tf->eax = 0;

  for(i = 0; i < NOFILE; i++)
    if(curproc->ofile[i])
      np->ofile[i] = filedup(curproc->ofile[i]);
  np->cwd = idup(curproc->cwd);

  safestrcpy(np->name, curproc->name, sizeof(curproc->name));

  pid = np->pid;

  acquire(&ptable.lock);

  // Make the process runnable.
  np->state = RUNNABLE;
  // Nice value of the parent gets copied to the child.
  np->niceValue = curproc->niceValue;
  // Insert the process into the tree.
  insertproc(&rbtree, np);

  release(&ptable.lock);

  return pid;
}

// Exit the current process.  Does not return.
// An exited process remains in the zombie state
// until its parent calls wait() to find out it exited.
void
exit(void)
{
  struct proc *curproc = myproc();
  struct proc *p;
  int fd;

  if(curproc == initproc)
    panic("init exiting");

  // Close all open files.
  for(fd = 0; fd < NOFILE; fd++){
    if(curproc->ofile[fd]){
      fileclose(curproc->ofile[fd]);
      curproc->ofile[fd] = 0;
    }
  }

  begin_op();
  iput(curproc->cwd);
  end_op();
  curproc->cwd = 0;

  acquire(&ptable.lock);

  // Parent might be sleeping in wait().
  wakeup1(curproc->parent);

  // Pass abandoned children to init.
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if(p->parent == curproc){
      p->parent = initproc;
      if(p->state == ZOMBIE)
        wakeup1(initproc);
    }
  }

  // Jump into the scheduler, never to return.
  curproc->state = ZOMBIE;
  sched();
  panic("zombie exit");
}

// Wait for a child process to exit and return its pid.
// Return -1 if this process has no children.
int
wait(void)
{
  struct proc *p;
  int havekids, pid;
  struct proc *curproc = myproc();

  acquire(&ptable.lock);
  for(;;){
    // Scan through table looking for exited children.
    havekids = 0;
    for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
      if(p->parent != curproc)
        continue;
      havekids = 1;
      if(p->state == ZOMBIE){
        // Found one.
        pid = p->pid;
        kfree(p->kstack);
        p->kstack = 0;
        freevm(p->pgdir);
        p->pid = 0;
        p->parent = 0;
        p->name[0] = 0;
        p->killed = 0;
        p->state = UNUSED;
        release(&ptable.lock);
        return pid;
      }
    }

    // No point waiting if we don't have any children.
    if(!havekids || curproc->killed){
      release(&ptable.lock);
      return -1;
    }

    // Wait for children to exit.  (See wakeup1 call in proc_exit.)
    sleep(curproc, &ptable.lock);  //DOC: wait-sleep
  }
}

//PAGEBREAK: 42
// Per-CPU process scheduler.
// Each CPU calls scheduler() after setting itself up.
// Scheduler never returns.  It loops, doing:
//  - choose a process to run
//  - swtch to start running that process
//  - eventually that process transfers control
//      via swtch back to the scheduler.
void
scheduler(void)
{
  struct proc *p;
  struct cpu *c = mycpu();
  c->proc = 0;

  for(;;){
    // Enable interrupts on this processor.
    sti();

    // Get processes from the tree until one of them is runnable.
    acquire(&ptable.lock);

    p = getproc(&rbtree);
    // Enter the loop if we could get a procces.
    while (p != 0)
    {
      // If the process is runnable switch to it.
      if (p->state == RUNNABLE)
      {
        // Switch to chosen process.  It is the process's job
        // to release ptable.lock and then reacquire it
        // before jumping back to us.
        c->proc = p;
        switchuvm(p);
        p->state = RUNNING;

        swtch(&(c->scheduler), p->context);
        switchkvm();

        // Process is done running for now.
        // It should have changed its p->state before coming back.
        c->proc = 0;
      }
      // Get another process from the tree.
      p = getproc(&rbtree);
    }

    release(&ptable.lock);
  }
}

// Enter scheduler.  Must hold only ptable.lock
// and have changed proc->state. Saves and restores
// intena because intena is a property of this
// kernel thread, not this CPU. It should
// be proc->intena and proc->ncli, but that would
// break in the few places where a lock is held but
// there's no process.
void
sched(void)
{
  int intena;
  struct proc *p = myproc();

  if(!holding(&ptable.lock))
    panic("sched ptable.lock");
  if(mycpu()->ncli != 1)
    panic("sched locks");
  if(p->state == RUNNING)
    panic("sched running");
  if(readeflags()&FL_IF)
    panic("sched interruptible");
  intena = mycpu()->intena;
  swtch(&p->context, mycpu()->scheduler);
  mycpu()->intena = intena;
}

// Determine whether the current process should be preempted or not.
void
yield(void)
{
  acquire(&ptable.lock);  //DOC: yieldlock

  // If the current process should be preempted, update its runtimes and reschedule.
  if (checkpreempt(myproc(), rbtree.min_vruntime))
  {
    updateruntimes(myproc());
    myproc()->state = RUNNABLE;
    insertproc(&rbtree, myproc());
    sched();
  }

  release(&ptable.lock);
}

// A fork child's very first scheduling by scheduler()
// will swtch here.  "Return" to user space.
void
forkret(void)
{
  static int first = 1;
  // Still holding ptable.lock from scheduler.
  release(&ptable.lock);

  if (first) {
    // Some initialization functions must be run in the context
    // of a regular process (e.g., they call sleep), and thus cannot
    // be run from main().
    first = 0;
    iinit(ROOTDEV);
    initlog(ROOTDEV);
  }

  // Return to "caller", actually trapret (see allocproc).
}

// Atomically release lock and sleep on chan.
// Reacquires lock when awakened.
void
sleep(void *chan, struct spinlock *lk)
{
  struct proc *p = myproc();

  if(p == 0)
    panic("sleep");

  if(lk == 0)
    panic("sleep without lk");

  // Must acquire ptable.lock in order to
  // change p->state and then call sched.
  // Once we hold ptable.lock, we can be
  // guaranteed that we won't miss any wakeup
  // (wakeup runs with ptable.lock locked),
  // so it's okay to release lk.
  if(lk != &ptable.lock){  //DOC: sleeplock0
    acquire(&ptable.lock);  //DOC: sleeplock1
    release(lk);
  }
  // Go to sleep.
  p->chan = chan;
  p->state = SLEEPING;

  sched();

  // Tidy up.
  p->chan = 0;

  // Reacquire original lock.
  if(lk != &ptable.lock){  //DOC: sleeplock2
    release(&ptable.lock);
    acquire(lk);
  }
}

//PAGEBREAK!
// Wake up all processes sleeping on chan.
// The ptable lock must be held.
static void
wakeup1(void *chan)
{
  struct proc *p;

  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++)
    if(p->state == SLEEPING && p->chan == chan)
    {
      // Make the process runnable.
      p->state = RUNNABLE;
      // Update current and virtual runtimes.
      updateruntimes(p);
      // Insert the process into the tree.
      insertproc(&rbtree, p);
    }
}

// Wake up all processes sleeping on chan.
void
wakeup(void *chan)
{
  acquire(&ptable.lock);
  wakeup1(chan);
  release(&ptable.lock);
}

// Kill the process with the given pid.
// Process won't exit until it returns
// to user space (see trap in trap.c).
int
kill(int pid)
{
  struct proc *p;

  acquire(&ptable.lock);
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if(p->pid == pid){
      p->killed = 1;
      // Wake process from sleep if necessary.
      if(p->state == SLEEPING)
      {
        // Make the process runnable.
        p->state = RUNNABLE;
        // Update current and virtual runtimes.
        updateruntimes(p);
        // Insert the process into the tree.
        insertproc(&rbtree, p);
      }
      release(&ptable.lock);
      return 0;
    }
  }
  release(&ptable.lock);
  return -1;
}

//PAGEBREAK: 36
// Print a process listing to console.  For debugging.
// Runs when user types ^P on console.
// No lock to avoid wedging a stuck machine further.
void
procdump(void)
{
  static char *states[] = {
  [UNUSED]    "unused",
  [EMBRYO]    "embryo",
  [SLEEPING]  "sleep ",
  [RUNNABLE]  "runble",
  [RUNNING]   "run   ",
  [ZOMBIE]    "zombie"
  };
  int i;
  struct proc *p;
  char *state;
  uint pc[10];

  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if(p->state == UNUSED)
      continue;
    if(p->state >= 0 && p->state < NELEM(states) && states[p->state])
      state = states[p->state];
    else
      state = "???";
    cprintf("%d %s %s", p->pid, state, p->name);
    if(p->state == SLEEPING){
      getcallerpcs((uint*)p->context->ebp+2, pc);
      for(i=0; i<10 && pc[i] != 0; i++)
        cprintf(" %p", pc[i]);
    }
    cprintf("\n");
  }
}
