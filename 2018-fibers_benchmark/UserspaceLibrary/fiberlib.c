#include "fiberlib.h"

int fd;
int fd_opened = 0;
pthread_mutex_t mtx = PTHREAD_MUTEX_INITIALIZER;

int open_device(void){
    pthread_mutex_lock(&mtx);
    if (!fd_opened){
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
    int new_fiber_id;

    if (open_device() != 1)
        exit(0);
    new_fiber_id = ioctl(fd, CONVERT, NULL);
    return (void *) new_fiber_id;
}


void *createFiber(size_t stack_size, entry_point_t function, void *args){
    int new_fiber_id;
    void *sp;

    if (open_device() != 1)
        exit(0);
    if(!(stack_size>0))
        return 0;
    posix_memalign(&sp, 16, stack_size);
    bzero(sp, stack_size);

    struct ioctl_params params = {
        .sp = (unsigned long) sp,
        .user_func = function,
        .args = args,
    };

    new_fiber_id = ioctl(fd, CREATE, &params);
    return (void *) new_fiber_id;
}


void switchToFiber(int fiber_id){
    int ret;

    if (open_device() != 1)
        exit(0);
    if(fiber_id>0){

        struct ioctl_params params = {
            .fiber_id = fiber_id
        };

        ret = ioctl(fd, SWITCH, &params);
    }
}


long flsAlloc(){
    long pos;

    if (open_device() != 1)
        exit(0);
    pos = ioctl(fd, FLSALLOC, NULL);
    return pos;
}


void flsSet(long pos, long long value){
    int ret;

    if (open_device() != 1)
        exit(0);
    if(pos < 0)
        printf("[!] flsSet pos is negative\n");
    else {

        struct ioctl_params params = {
            .pos = pos,
            .value = value
        };

        ret = ioctl(fd, FLSSET, &params);
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

        if (ioctl(fd, FLSGET, &params))
            return params.value;
        return (long long) NULL;
    }
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

        if(ioctl(fd, FLSFREE, &params))
            success = true;
    }
    return success;
}