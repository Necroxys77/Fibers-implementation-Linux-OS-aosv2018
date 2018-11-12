#include "fiberlib.c"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

void function (void* parameters)
{
        printf("Starting function...\n");
        long index = flsAlloc();
        long long l1 = 123456;
        flsSet(index,l1);
        long long l2;
        l2 = flsGet(index);
        /*printf("Parameters is %c and value is %c\n", *((char*)parameters), b);
        FlsFree(index);*/
        long counter = 0;
        //double x = 0.0;
        while(1) {
                if (counter % 1000000000 == 0){
                      //printf("ciao, %f\n", x);
                      printf("ciao\n");
                      break;
                      //x+=0.5;
                }
                counter++;
        }
        switchToFiber(1);
        counter = 0;
        float x __attribute__ ((aligned (16))) = 0.0;
        while(1) {
                if (counter % 1000000000 == 0){
                      //printf("ciao, %f\n", x);
                      //printf("ciao\n");
                      x+=0.5;
                }
                if (x > 1){
                  break;
                }
                counter++;
        }
        switchToFiber(1);
        x += 0.6;
        if (x > 1.5){
          printf("bellaaaaaaaaaaa\n");
        }
        for (counter = 0; counter < 1000000000; counter ++){
          printf("abcdefg\n");
        }
        /*while(1) {
                if (counter % 10000000 == 0){
                      //printf("ciao, %f\n", x);
                      //printf("ciao\n");
                      x+=0.6;
                }
                if (x > 1.5){
                  break;
                }
                counter++;
        }*/
        switchToFiber(1);
        exit(0);
}

int main()
{
        printf("Starting main...\n");
        void *my_fiber = convertThreadToFiber();
        printf("ConvertThreadToFiber done\n");
        char c = 'c';
        unsigned long new_fiber = (unsigned long) createFiber(6,function, &c);
        switchToFiber(new_fiber);
        long counter = 0;
        double x = 0.0;
        while(1) {
                if (counter % 1000000000 == 0){
                      printf("bella, %f\n", x);
                      //printf("ciao\n");
                      x+=0.5;
                      break;
                }
                counter++;
        }
        switchToFiber(new_fiber);
        while(1) {
                if (counter % 1000000000 == 0){
                      printf("bella, %f\n", x);
                      //printf("ciao\n");
                      x+=0.5;
                      break;
                }
                counter++;
        }
        switchToFiber(new_fiber);
        for (counter = 0; counter < 1000000000; counter ++){
          printf("hilmnopq\n");
        }
        switchToFiber(new_fiber);
        exit(0);
}
