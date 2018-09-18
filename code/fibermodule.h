#ifndef FIBER
#define FIBER

#include <linux/init.h>           // Macros used to mark up functions e.g. __init __exit
#include <linux/module.h>         // Core header for loading LKMs into the kernel
#include <linux/device.h>         // Header to support the kernel Driver Model
#include <linux/kernel.h>         // Contains types, macros, functions for the kernel
#include <linux/fs.h>             // Header for the Linux file system support
#include <linux/uaccess.h>          // Required for the copy to user function
#include <linux/sched/task_stack.h>

#define MAGIC 'a'
#define convertThreadToFiber _IO(MAGIC, 0)
#define createFiber _IO(MAGIC, 1)
#define switchToFiber _IO(MAGIC, 2

struct fiber {
    struct pt_regs regs;
    pid_t parent;
    pid_t fiber_id;
    int active;
    void *param;
};

static long my_ioctl(struct file *, unsigned int, unsigned long);

static int __init starting(void);

static int __exit exiting(void);

#endif