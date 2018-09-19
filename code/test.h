#ifndef TEST
#define TEST

#include<stdio.h>
#include<stdlib.h>
#include<errno.h>
#include<fcntl.h>
#include<string.h>
#include<unistd.h>
#include<sys/ioctl.h>

#define MAGIC 'a'
#define convertF _IO(MAGIC, 0)
#define createF _IO(MAGIC, 1)
#define switchF _IO(MAGIC, 2)

#define STACK_SIZE (4096*2)

struct ioctl_params {
    void **args;
    unsigned long *sp, *ss;
    unsigned long user_func;
};

void convertThreadToFiber(void);

void createFiber(unsigned long, void **);

void switchToFiber(void);

#endif