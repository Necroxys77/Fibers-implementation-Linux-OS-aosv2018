#ifndef ioctlm
#define ioctlm
/*
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
#include "fiber.h"
*/
#include "fiber.h"
#include "kprobehandlers.h"

#define MAGIC '7'
#define CONVERT _IO(MAGIC, 0)
#define CREATE _IO(MAGIC, 1)
#define SWITCH _IO(MAGIC, 2)
#define FLSALLOC _IO(MAGIC, 3)
#define FLSSET _IO(MAGIC, 4)
#define FLSGET _IO(MAGIC, 5)
#define FLSFREE _IO(MAGIC, 6)

struct ioctl_params {
    long pos;
    long long value;
    void *args;
    unsigned long sp;
    unsigned long bp;
    entry_point user_func;
    int fiber_id;
};

static long my_ioctl(struct file *, unsigned int, unsigned long);

static char *unlock_sudo(struct device *, umode_t *);

static int __init starting(void);

static void __exit exiting(void);




#endif