#include "fiberlib.c"

//test
struct stru {
    char *name;
};

//file descriptor

/* TEST FUNCTIONS */
int matteo(void **ciao){
    float mark;
    long pos,pos2;
    long long val, val2;

    mark = 7.74473;
    mark += 7.1;
    printf("%f\n",mark);

    pos = flsAlloc();
    //pos2 = flsAlloc();
    val = (long long) 77;
    flsSet(pos,val);
    flsSet(3,(long long) 34);
    //flsSet(999,val); //CASE TO HANDLE! See flsSet description into fiber.c
    val2 = flsGet(3);
    //flsGet(21);
    flsFree(pos);
    flsFree(3);

    
    printf("Matteo\n");
    struct stru *prova;
    printf("Mariani\n");
    prova = (struct stru *) ciao;


    switchToFiber(3);
    mark += 7.1;
    printf("%f\n",mark);

    printf("Aprilia\n");

    switchToFiber(1);
    printf("%s\n", prova->name);
    
    //exit(0); //Note that fibers don't return or exit!
}

int riccardo(void **ciao){
    float mark;
    mark = 6.66666;
    mark += 0.1;
    printf("%f\n",mark);
    
    printf("Riccardo\n");
    struct stru *prova;
    printf("Charetti\n");
    prova = (struct stru *) ciao;

    switchToFiber(1);
    printf("Cascia\n");

    printf("%s\n", prova->name);
    
    //exit(0); //Note that fibers don't return or exit!
}


int fabrizio(void **ciao){

    printf("Fabrizio\n");
    struct stru *prova;
    printf("Rossi\n");
    prova = (struct stru *) ciao;

    //switchToFiber(2);
    printf("Milano\n");

    printf("%s\n", prova->name);
    
    //exit(0); //Remember that fibers do not return!
}


int main(int argc, char const *argv[]){
    struct stru str;
    str.name = "prova_in_main";
    unsigned long a, b, c, d, e;

    //should fail
    //d =  (unsigned long) createFiber(STACK_SIZE, (entry_point) matteo, (void *) &str);

    //should fail
    //switchToFiber(5);

    a = (unsigned long) convertThreadToFiber();

    //should fail
    //e = (unsigned long) convertThreadToFiber();

    b = (unsigned long) createFiber(STACK_SIZE, (entry_point) matteo, (void *) &str);

    c = (unsigned long) createFiber(STACK_SIZE, (entry_point) riccardo, (void *) &str);

    //should fail
    //switchToFiber(99);

    //let's dance...
    switchToFiber(2);
    switchToFiber(2);

    printf("Every child comes back to his parents <3 \n");
    return 0;
}