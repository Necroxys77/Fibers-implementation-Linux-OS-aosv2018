#ifndef fiberlibrary
#define fiberlibrary

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <stdbool.h>
#include <sys/ioctl.h>

#define MAGIC '7'
#define CONVERT _IO(MAGIC, 0)
#define CREATE _IO(MAGIC, 1)
#define SWITCH _IO(MAGIC, 2)
#define STACK_SIZE (4096*2)

typedef void (*entry_point)(void *param);

/**
 * Struct that describes the parameters passed to the ioctl() call in order to access kernel space 
 * @field   **args, (???)
 * @field   *sp,    pointer so the stack pointer
 * @field   *ss,    pointer to the stack segment
 * @field   fiber_id,   integer that represent the id of the fiber used for any purpose by userspace
 */
struct ioctl_params {
    void **args;
    unsigned long *sp;
    unsigned long *bp;
    entry_point user_func;
    int fiber_id;
};

/**
 * Method that converts the calling threads into a fiber. If a thread is a fiber it can create
 * new fibers and switch to other fibers using the appropriate functions. The first fiber is the caller thread
 * so inherits its registers
 * @return (???)
 */
void *convertThreadToFiber(void);

/**
 * It creates a fiber for the process that has the calling thread - that is already a fiber.
 * If the caller is not a fiber, an error is raised.
 * @param size_t    stacksize, the size of the stack 
 * @param 
 * @return return description
 */

void *createFiber(size_t, entry_point, void *param);

/**
 * Method description.
 * @param param1 param1 description 
 * @param param2 param2 description
 * @return return description
 */
void switchToFiber(int);

/**
 * Method description.
 * @param param1 param1 description 
 * @param param2 param2 description
 * @return return description
 */
long long flsGet(long);

/**
 * Method description.
 * @param param1 param1 description 
 * @param param2 param2 description
 * @return return description
 */
bool flsFree(long);

/**
 * Method description.
 * @param param1 param1 description 
 * @param param2 param2 description
 * @return return description
 */
long flsAlloc(void);

/**
 * Method description.
 * @param param1 param1 description 
 * @param param2 param2 description
 * @return return description
 */
void flsSet(long, long long);


#endif
