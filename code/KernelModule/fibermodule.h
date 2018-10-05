#ifndef FIBER
#define FIBER

/*
struct pt_regs {
    C ABI says these regs are callee-preserved. They aren't saved on kernel entry
    unless syscall needs a complete, fully filled "struct pt_regs".
    unsigned long r15;
    unsigned long r14;
    unsigned long r13;
    unsigned long r12;
    unsigned long bp;
    unsigned long bx;
    These regs are callee-clobbered. Always saved on kernel entry. 
    unsigned long r11;
    unsigned long r10;
    unsigned long r9;
    unsigned long r8;
    unsigned long ax;
    unsigned long cx;
    unsigned long dx;
    unsigned long si;
    unsigned long di;
    unsigned long sp;
    unsigned long ip;
    unsigned long flags;

    On syscall entry, this is syscall#. On CPU exception, this is error code.
    On hw interrupt, it's IRQ number:
 
    unsigned long orig_ax;
    Return frame for iretq 
    unsigned long cs;
    unsigned long ss;
    top of stack page 
};
*/

#include <linux/init.h>           // Macros used to mark up functions e.g. __init __exit
#include <linux/module.h>         // Core header for loading LKMs into the kernel
#include <linux/device.h>         // Header to support the kernel Driver Model
#include <linux/kernel.h>         // Contains types, macros, functions for the kernel
#include <linux/fs.h>             // Header for the Linux file system support
#include <linux/uaccess.h>      // Required for the copy to user function
#include <linux/sched/task_stack.h>
#include <linux/hashtable.h>    //Required for using linux implementation of hashtables 
#include <linux/slab.h>
#include <asm/atomic.h>     //Required for using safe concurrency control
#include <linux/spinlock.h>

#define MAGIC '7'
#define CONVERT _IO(MAGIC, 0)
#define CREATE _IO(MAGIC, 1)
#define SWITCH _IO(MAGIC, 2)
#define STACK_SIZE (4096*2)

typedef void (*entry_point)(void *param);

typedef struct struct_process {
    pid_t tgid; //Key
    atomic_t total_fibers;
    //DECLARE_HASHTABLE(threads, 10);
    DECLARE_HASHTABLE(fibers, 10);
    struct hlist_node table_node;
} process;

/**
 * Struct that defines a fiber in the system
 * 
 * @field   struct pt_regs *regs describes the registers associated to a fiber 
 * @field   int fiber_id is the identifier of the fiber inside a specific thread
 */
typedef struct struct_fiber {
    spinlock_t lock;
    struct pt_regs *context;
    int fiber_id;
    int is_running; //Pid of the thread running the fiber or -1
    pid_t tgid, parent_pid; //ridondanti?
    struct hlist_node table_node;  
} fiber;

/**
 * Struct that handle the parameters that are passed from/to user space and kernel space through ioctl call
 * 
 * @field   void **args is the address in which (???)
 * @field   unsigned long *sp is the pointer to the stack pointer
 * @field   unsigned long *bp is the pointer to the base pointer
 * @field   int fiber_id is the id of the fiber that is filled in order to give infos to userspace
 *          from kernel space
 */
struct ioctl_params {
    void **args;
    unsigned long *sp, *bp;
    unsigned long user_func;
    int fiber_id;
    pid_t thread_pid;
};
/**
 * Method description.
 * @param param1 param1 description 
 * @param param2 param2 description
 * @return return description
 * @since date since method is created
 * @author who have made this method.
 */
static long my_ioctl(struct file *, unsigned int, unsigned long);
static char *unlock_sudo(struct device *, umode_t *);
static int __init starting(void);
static void __exit exiting(void);
static void *convertThreadToFiber(void);
static void *createFiber(struct ioctl_params *);

#endif