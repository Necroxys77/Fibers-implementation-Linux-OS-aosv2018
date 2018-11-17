#ifndef ioctlm
#define ioctlm

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