#include "fiberlib.c"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

void function(){
    printf("Starting function...\n");
    long index = flsAlloc();
    long long l1 = 123456;
    flsSet(index,l1);
    
    long long l2;
    l2 = flsGet(index);
    
    long counter = 0;
    while(1){
        printf("scum");
        if (counter==100000000)
            break;
        counter++;
    }
    printf("\nExiting fiber 2...\n");
    counter = 0;

    switchToFiber(1);

    while(1){
        printf("scum");
        if (counter==100000000)
            break;
        counter++;
    }
    printf("\nExiting fiber 2 ...\n");
    counter = 0;
    
    switchToFiber(1);
    
    while(1){
        printf("scum");
        if (counter==100000000)
            break;
        counter++;
    }
    printf("\nExiting fiber 2 ...\n");
    counter = 0;
    
    switchToFiber(1);
    
    exit(0);
}

int main(){
    unsigned long a, b;
    char c = 'c';

    printf("Starting test...\n");
    a = (unsigned long) convertThreadToFiber();
    b = (unsigned long) createFiber(STACK_SIZE, (entry_point) function, (void *) &c);

    long counter = 0;
    while(1){
        printf("yolo");
        if (counter==100000000)
            break;
        counter++;
    }
    printf("\nExiting fiber 1 ...\n");
    counter = 0;

    switchToFiber(b);
    

    while(1){
        printf("yolo");
        if (counter==100000000)
            break;
        counter++;
    }
    printf("\nExiting fiber 1 ...\n");
    counter = 0;
    
    switchToFiber(b);

    while(1){
        printf("yolo");
        if (counter==100000000)
            break;
        counter++;
    }
    printf("\nExiting fiber 1 ...\n");
    counter = 0;
    
    switchToFiber(b);
    
    
    printf("byebye!\n");


    return 0;
}
