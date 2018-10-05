#include "fiberlibrary.c"

//test
struct stru {
    char *name;
};

//file descriptor
int fd;

/* TEST FUNCTIONS */
int matteo(void **ciao){

    printf("Matteo\n");
    struct stru *prova;
    printf("Mariani\n");
    prova = (struct stru *) ciao;

    switchToFiber(3);

    printf("Aprilia\n");

    switchToFiber(1);
    printf("%s\n", prova->name);
    
    exit(0); //Note that fibers don't return or exit!
}

int riccardo(void **ciao){

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

    switchToFiber(2);
    printf("Milano\n");

    printf("%s\n", prova->name);
    
    //exit(0); //Remember that fibers do not return!
}


int main(int argc, char const *argv[]){
    struct stru str;
    str.name = "prova_in_main";
    int *a, *b, *c, *d, *e;

    fd = open("/dev/DeviceName", O_RDWR);
    if (fd < 0){
        perror("[-] Failed to open the device...\n");
        return errno;
    }

    //should fail
    //d =  (int *) createFiber(STACK_SIZE, (entry_point) matteo, (void *) &str);

    //should fail
    //switchToFiber(5);

    a = (int *) convertThreadToFiber();

    //should fail
    //e = (int *) convertThreadToFiber();

    b = (int *) createFiber(STACK_SIZE, (entry_point) matteo, (void *) &str);

    c = (int *) createFiber(STACK_SIZE, (entry_point) riccardo, (void *) &str);

    //should fail
    //switchToFiber(99);

    //let's dance...
    switchToFiber(2);
    switchToFiber(2);

    printf("Every child comes back to his parents <3 \n");
    return 0;
}