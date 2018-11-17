#include "fiberlib.h"

int fd;
int fd_opened = 0;
pthread_mutex_t mtx = PTHREAD_MUTEX_INITIALIZER;
/*
If the calling thread successfully converts into a fiber, it returns the fiber id. -1 otherwise.
*/
/*To remove warnings of wrong casting, we should let the module to return the fiber id inside the ioctl_params data structure*/

int open_device(void){

    pthread_mutex_lock(&mtx);
    if (!fd_opened) {
        fd = open("/dev/DeviceName", O_RDWR);
        if (fd < 0){
            perror("[-] Failed to open the device...\n");
            return errno;
        }
        fd_opened = 1;
    }
    pthread_mutex_unlock(&mtx);
    return 1;
}

void *convertThreadToFiber(void){

    if (open_device() != 1)
        exit(0);

    int new_fiber_id;
    //printf("[-] Converting thread into a fiber... \n");

    new_fiber_id = ioctl(fd, CONVERT, NULL);

    /*
    if(new_fiber_id)
        printf("[+] Thread successfully converted into fiber!\n");
    else 
        printf("[!] Thread not able to convert\n");
    */
    return (void *) new_fiber_id; //to remove warnign, fiber_id should be uns long
}

void *createFiber(size_t stack_size, entry_point function, void *args){
    int new_fiber_id;
    void *sp;

    if (open_device() != 1)
        exit(0);

    if(!(stack_size>0))
        return 0; //error (TO CHECK)

    //printf("[-] Creating a fiber... \n");
    posix_memalign(&sp, 16, stack_size);
    bzero(sp, stack_size);

    struct ioctl_params params = {
        .sp = (unsigned long) sp,
        .user_func = function,
        .args = args,
    };

    new_fiber_id = ioctl(fd, CREATE, &params);

    /*
    if(new_fiber_id)
        printf("[+] Fiber %d created!\n", new_fiber_id);
    else
        printf("[!] Impossible to create a fiber!\n");
    */
    return (void *) new_fiber_id;
}

void switchToFiber(int fiber_id){
    int success;

    if (open_device() != 1)
        exit(0);

    if(fiber_id>0){
        //printf("[-] Switching to fiber %d...\n", fiber_id);

        struct ioctl_params params = {
            .fiber_id = fiber_id
        };

        success = ioctl(fd, SWITCH, &params);
        /*
        if(!success)
            printf("[!] Impossible to switch into fiber %d\n", fiber_id);
        */
    }
}

long flsAlloc(){
    long pos;

    if (open_device() != 1)
        exit(0);
    
    pos = ioctl(fd, FLSALLOC, NULL);
    /*
    if(pos != -1)
        printf("[+] Retrieved position %ld from the fls\n", pos);
    else
        printf("[!] Failed to retrieve a position in the fls...");
    */
    return pos;
}

void flsSet(long pos, long long value){
    int success;

    if (open_device() != 1)
        exit(0);

    if(pos < 0)
        printf("[!] flsSet pos is negative\n");
    else{
        struct ioctl_params params = {
            .pos = pos,
            .value = value
        };

        success = ioctl(fd, FLSSET, &params);
        /*
        if(success)
            printf("[+] Value %lld is put at position %ld\n", value, pos);
        else
            printf("[!] Error while setting the value\n");
        */
    }
}

long long flsGet(long pos){

    if (open_device() != 1)
        exit(0);

    if(pos < 0)
        printf("[!] flsGet pos is negative\n");
    else {

        struct ioctl_params params = {
            .pos = pos,
            .value = -1
        };

        if (ioctl(fd, FLSGET, &params)){
            //printf("[+] Value %lld is retrieved from position %ld\n", params.value, pos);
            return params.value;
        }
        //printf("[!] Error while retrieving the value\n");
        return (long long) NULL;

        /*value = (void *) ioctl(fd, FLSGET, &params);
        if(value!=NULL)
            printf("[+] Value %lld is retrieved from position %ld\n", (long long) value, pos);
        else
            printf("[!] Error while retrieving the value\n");
        */
    }
    
    //return (long long) value;
}

bool flsFree(long pos){
    bool success = false;

    if (open_device() != 1)
        exit(0);

    if(pos < 0)
        printf("[!] flsGet: pos is negative\n");
    else {

        struct ioctl_params params = {
            .pos = pos,
        };

        if(ioctl(fd, FLSFREE, &params)){
            //printf("[+] Position %ld freed from fls!\n", pos);
            success = true;
        }// else
           // printf("[!] Error while freeing the value in position %ld\n", pos);
    }

    return success;
}
