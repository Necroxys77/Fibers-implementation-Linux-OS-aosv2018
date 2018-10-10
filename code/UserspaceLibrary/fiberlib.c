#include "fiberlib.h"

int fd;

/*
If the calling thread successfully converts into a fiber, it returns the fiber id. -1 otherwise.
*/
void *convertThreadToFiber(void){
    int new_fiber_id;
    printf("[-] Converting thread into a fiber... \n");

    new_fiber_id = ioctl(fd, CONVERT, NULL);

    if(new_fiber_id)
        printf("[+] Thread successfully converted into fiber!\n");
    else 
        printf("[!] Thread not able to convert\n");

    return (void *) new_fiber_id; //to remove warnign, fiber_id should be uns long
}


void *createFiber(size_t stack_size, entry_point function, void *args){
    int new_fiber_id;
    void *sp;

    if(!(stack_size>0))
        return 0; //error (TO CHECK)

    printf("[-] Creating a fiber... \n");
    posix_memalign(&sp, 16, stack_size);
    bzero(sp, stack_size);

    struct ioctl_params params = {
        .sp = (unsigned long) sp,
        .user_func = function,
        .args = args,
    };

    new_fiber_id = ioctl(fd, CREATE, &params);

    if(new_fiber_id)
        printf("[+] Fiber %d created!\n", new_fiber_id);
    else
        printf("[!] Impossible to create a fiber!\n");

    return (void *) new_fiber_id;
}


void switchToFiber(int fiber_id){
    int success;
    if(fiber_id>0){
        printf("[-] Switching to fiber %d...\n", fiber_id);

        struct ioctl_params params = {
            .fiber_id = fiber_id
        };

        success = ioctl(fd, SWITCH, &params);
        if(!success)
            printf("[!] Impossible to switch into fiber %d\n", fiber_id);
    }
}



long flsAlloc(){
    long pos;
    
    pos = ioctl(fd, FLSALLOC, NULL);
    if(pos == -1)
        printf("[!] Fail...");

    return pos;
}