#include<stdio.h>
#include<stdlib.h>
#include<errno.h>
#include<fcntl.h>
#include<string.h>
#include<unistd.h>
#include<sys/ioctl.h>

#define MAGIC 'a'
#define case_1 _IO(MAGIC, 0)
#define case_2 _IO(MAGIC, 1)

int main(int argc, char const *argv[])
{
    int fd = open("/dev/DeviceName", O_RDWR);
    if (fd < 0){
        perror("Failed to open the device...");
        return errno;
    }

    ioctl(fd, case_1, 2);
    printf("First one\n");
    ioctl(fd, case_2, 2);
    printf("Second one\n");
    close(fd);
    printf("Closed\n");
    return 0;
}
