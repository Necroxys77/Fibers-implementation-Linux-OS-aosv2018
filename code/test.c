#include "test.h"

int fd;

void ConvertThreadToFiber(void){

    ioctl(fd, convertThreadToFiber, NULL);
}

void CreateFiber(void){

}

void SwitchToFiber(void){

}

int main(int argc, char const *argv[])
{
    fd = open("/dev/DeviceName", O_RDWR);
    if (fd < 0){
        perror("Failed to open the device...");
        return errno;
    }
    ioctl(fd, convertThreadToFiber, 0);
    ioctl(fd, convertThreadToFiber, 0);
    return 0;
}
