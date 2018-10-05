#include "fiberlibrary.h"

int fd;

/*
If the calling thread successfully converts into a fiber, it returns the fiber id. -1 otherwise.
*/
void *convertThreadToFiber(void){
    int *ret_val; //necessary to handle return
    printf("[-] Converting thread into a fiber... \n");
    
    ret_val = (int *) malloc(sizeof(int *));
    //params is used to understand if the convert ends correctly
    struct ioctl_params params = {
        .fiber_id = -1
    };
    
    //entering in kernel mode...
    ioctl(fd, CONVERT, &params);

    *ret_val = params.fiber_id;
    if (params.fiber_id!=-1) printf("[+] Thread successfully converted into fiber!\n");
    else printf("[!] Thread not able to convert\n");

    return ret_val;
}

void *createFiber(size_t stack_size, entry_point function, void *args){
    int *ret_val;

    printf("[-] Creating a fiber... \n");
    ret_val = (int *) malloc(sizeof(int *));

    void *sp; //Stack pointer
    sp = malloc(stack_size); //Allocating 8KB of stack
    bzero(sp, sizeof(sp)); //Zeroing the stack of the fiber

    //Populating the data structure containing all info for creating a new fiber
    struct ioctl_params params = {
        .sp = sp,
        .user_func = function,
        .args = args,
        .fiber_id = -1
    };

    //intering in kernel mode...
    ioctl(fd, CREATE, &params);

    *ret_val = params.fiber_id; //?
    if(params.fiber_id==-1) printf("[!] Impossible to create a fiber!\n");
    else printf("[+] Fiber %d created!\n", *ret_val);

    return ret_val;
}

void switchToFiber(int fiber_id){

    printf("[-] Switching to fiber %d...\n", fiber_id);

    struct ioctl_params params = {
        .fiber_id = fiber_id //Fiber id to switch to
    };

    ioctl(fd, SWITCH, &params);

    //printf("param fiber id: %d\n",params.fiber_id);
    if (params.fiber_id==-1) printf("[!] Impossible to switch fiber %d\n", fiber_id);
}

long long fls_get(long pos){

}

bool flsFree(long pos){

}

long flsAlloc(void){

}

void flsSet(long pos, long long to_store){

}