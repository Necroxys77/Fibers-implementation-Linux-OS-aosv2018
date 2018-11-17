#include <linux/init.h>           // Macros used to mark up functions e.g. __init __exit
#include <linux/module.h>         // Core header for loading LKMs into the kernel
#include <linux/device.h>         // Header to support the kernel Driver Model
#include <linux/kernel.h>         // Contains types, macros, functions for the kernel
#include <linux/fs.h>             // Header for the Linux file system support
#include <linux/uaccess.h>      // Required for the copy to user function
#include <linux/sched/task_stack.h>
#include <linux/hashtable.h>    //Required for using linux implementation of hashtables 
#include <linux/slab.h>
#include <asm/fpu/internal.h>
#include <asm/atomic.h>     //Required for using safe concurrency control
#include <linux/spinlock.h>
#include <linux/bitmap.h>
#include <linux/types.h>
#include <linux/threads.h>
#include <linux/timekeeping32.h>

#define MAX_SIZE_FLS 4096
#define STACK_SIZE (4096*2)
#define FIBER_NAME 512


typedef void (*entry_point)(void *param);

//check order param?
typedef struct {
    pid_t tgid; //Key
    atomic_t total_fibers, nThreads; //increment in convert and decrease in post_doExit - quando arriva a zero, remove process
    DECLARE_HASHTABLE(fibers, 10);
    DECLARE_HASHTABLE(threads, 10);
    struct hlist_node table_node;
} process;

//check order param?
typedef struct {
    spinlock_t lock;
    struct pt_regs *context;
    struct fpu *fpu_regs;
    int fiber_id, finalized_activations, failed_activations;
    unsigned long exec_time, start_time;
    long long fls[MAX_SIZE_FLS];
    DECLARE_BITMAP(fls_bitmap, MAX_SIZE_FLS);
    pid_t parent_pid;
    char running; 
    struct hlist_node table_node;
    char fiber_id_string[FIBER_NAME];
    void *initial_entry_point;
} fiber;


typedef struct {
    pid_t pid;
    fiber* runningFiber;
    struct hlist_node table_node;
} thread;


int convertThreadToFiber(void);

int createFiber(unsigned long, entry_point, void *);

int switchToFiber(int);

long long flsGet(long);

int flsFree(long);

long flsAlloc(void);

int flsSet(long, long long);

void remove_process(process *);

void update_timer(struct task_struct *, struct task_struct *);

inline process *get_process_by_tgid(pid_t);

inline thread *get_thread_by_pid(process *, pid_t);

inline fiber *get_fiber_by_id(process *, int);

#endif