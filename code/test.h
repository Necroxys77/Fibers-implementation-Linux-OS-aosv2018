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
#define convertThreadToFiber _IO(MAGIC, 0)
#define createFiber _IO(MAGIC, 1)
#define switchToFiber _IO(MAGIC, 2)

void ConvertThreadToFiber(void);

void CreateFiber(void);

void SwitchToFiber(void);

#endif