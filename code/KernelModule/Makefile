obj-m += fibermod.o

fibermod-y := ioctl.o fiber.o kprobe.o
all:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean