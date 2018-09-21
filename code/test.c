#include "test.h"

int fd;

int convertThreadToFiber(void){

    struct ioctl_params params = {
        .fiber_id = 0
    };

    ioctl(fd, convertF, &params);

    printf("Fiber id: %d\n", params.fiber_id);
    return (params.fiber_id);
}

struct stru {
    char *name;
};

int createFiber(unsigned long entry_point, void **args){

    void *sp, *ss; //Stack pointer, stack segment
    sp = malloc(STACK_SIZE); //Allocating 8KB of stack
    bzero(sp, sizeof(sp)); //Zeroing the stack of the fiber
    ss = sp; //At the beginning they are the same

    //Populating the data structure containing all info for creating a new fiber
    struct ioctl_params params = {
        .sp = sp,
        .ss = ss,
        .user_func = entry_point,
        .args = args
    };

    ioctl(fd, createF, &params);

    printf("New fiber id: %d!\n", params.fiber_id);

    return (params.fiber_id);
}

void switchToFiber(void){


}

int prova (void **ciao){

    printf("Sopra a prova\n");
    struct stru *prova;
    printf("In mezzo a prova\n");
    prova = (struct stru *) ciao;

    printf("Sotto prova\n");

    printf("%s\n", prova->name);
    exit(0); //Remember that fibers do not return!
}

int main(int argc, char const *argv[])
{
    struct stru str;
    str.name = "prova";
    int a, b, c;

    fd = open("/dev/DeviceName", O_RDWR);
    if (fd < 0){
        perror("Failed to open the device...");
        return errno;
    }
    c = createFiber((unsigned long) prova, (void *) &str);

    a = convertThreadToFiber();

    a = convertThreadToFiber();

    b = createFiber((unsigned long) prova, (void *) &str);

    c = createFiber((unsigned long) prova, (void *) &str);
    return 0;
}
