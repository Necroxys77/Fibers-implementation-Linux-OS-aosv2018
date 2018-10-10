/*
#ifndef fls
#define fls

//REFACTORING INCLUDE
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
#include <linux/types.h>
#include "fiber.h"


long long flsGet(long);

bool flsFree(long);

long flsAlloc(void);

void flsSet(long, long long);


#endif

*/