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

    On syscall entry, this is syscall#. On CPU exception, this is error code.
    On hw interrupt, it's IRQ number:
 
    unsigned long orig_ax;
    Return frame for iretq 
    unsigned long ip;
    unsigned long cs;
    unsigned long flags;
    unsigned long sp;
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

#define MAGIC 'a'
#define convertF _IO(MAGIC, 0)
#define createF _IO(MAGIC, 1)
#define switchF _IO(MAGIC, 2)

#define STACK_SIZE (4096*2)  // 8KB

/**
 * Struct that defines a fiber in the system
 * 
 * @field   struct pt_regs *regs describes the registers associated to a fiber 
 * @field   int fiber_id is the identifier of the fiber inside a specific thread
 */
struct fiber {
    struct pt_regs *regs;
    int fiber_id;
    atomic_t active;
    void *param; //unused!
};

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
};

/**
 * Struct that describe an element in the hashtable threads that contains the threads that have at least one fiber.
 * An element of the hashtable is associated to one thread - that is the creator fiber
 * 
 * @field   atomic_t total fibers is the number of fibers part of the thread pid_t parent
 * @field   pid_t parent is the pid of the thread that is now a fiber
 * @field   struct fiber fiber
 */
struct table_element_thread {
    atomic_t total_fibers;
    pid_t parent;
    struct fiber *fiber;
    struct hlist_node table_node;
    DECLARE_HASHTABLE(fibers, 10); //Per-thread hashtable containing fibers created within thread
};

struct table_element_fiber {
    struct fiber *fiber;
    struct hlist_node table_node;
};

/*struct table_element {
    atomic_t total_fibers;
    pid_t parent;
    struct fiber fiber;
    struct hlist_node table_node;
};
*/

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
static int convertThreadToFiber(void);
static int createFiber(struct ioctl_params *);

#endif