#include "fibermodule.h"

static int majorNumber;
static struct class* charClass  = NULL; //< The device-driver class struct pointer
static struct device* charDevice = NULL; //< The device-driver device struct pointer

static long my_ioctl(struct file *file, unsigned int cmd, unsigned long arg);

static struct file_operations fops =
{
    .owner = THIS_MODULE,
    .unlocked_ioctl= my_ioctl
    //.open = dev_open,
    //.read = dev_read,
    //.write = dev_write,
    //.release = dev_release,
};

static long my_ioctl(struct file *file, unsigned int cmd, unsigned long arg){

    struct pt_regs *reg, *reg_2;
    unsigned long old_ip, old_sp;
    printk("Command: %d\n", cmd);
    switch(cmd) {
        case convertThreadToFiber:
            //copy_from_user(&value ,(int32_t*) arg, sizeof(value));
            //printk(KERN_INFO "Value = %d\n", value);
            reg = task_pt_regs(current);
            old_ip = reg->ip;
            old_sp = reg->sp;
            reg->ip = arg;
            printk(KERN_INFO "Hello 1\n");
            break;
        case CASE_2:
            printk(KERN_INFO "Hello 2\n");
            reg_2 = task_pt_regs(current);
            reg_2->sp = old_sp;
            reg_2->ip = old_ip;
            //copy_to_user((int32_t*) arg, &value, sizeof(value));
            break;
        }
        return 0;
}

static int __init starting(void){
    printk(KERN_INFO "We are in _init!\n");
 
    /* 
        Try to dynamically allocate a major number for the device
        0: tells the kernel to allocate a free major;
        DeviceName: is the name of the device
        &fops: address of the structure containing function pointers
    */
    majorNumber = register_chrdev(0, "DeviceName", &fops); 
    if (majorNumber<0){
        printk(KERN_ALERT "Failed to register a major number!\n");
        return majorNumber;
    }
    printk(KERN_INFO "Registered correctly with major number %d!\n", majorNumber);

    // Register the device class
    charClass = class_create(THIS_MODULE, "ClassName");
    if (IS_ERR(charClass)){                // Check for error and clean up if there is
        unregister_chrdev(majorNumber, "DeviceName");
        printk(KERN_ALERT "Failed to register device class!\n");
        return PTR_ERR(charClass);          // Correct way to return an error on a pointer
    }
    printk(KERN_INFO "Device class registered correctly!\n");

    // Register the device driver
    charDevice = device_create(charClass, NULL, MKDEV(majorNumber, 0), NULL, "DeviceName");
    if (IS_ERR(charDevice)){               // Clean up if there is an error
        class_destroy(charClass);           // Repeated code but the alternative is goto statements
        unregister_chrdev(majorNumber, "DeviceName");
        printk(KERN_ALERT "Failed to create the device!\n");
        return PTR_ERR(charDevice);
    }
    printk(KERN_INFO "Device class created correctly, __init finished!\n"); // Made it! device was initialized
    return 0;
}

static void __exit exiting(void){
    printk(KERN_INFO "We are exiting!\n");
    device_destroy(charClass, MKDEV(majorNumber, 0));     // remove the device
    class_unregister(charClass);                          // unregister the device class
    class_destroy(charClass);                             // remove the device class
    unregister_chrdev(majorNumber, "DeviceName");             // unregister the major number
    printk(KERN_INFO "Goodbye from the LKM!\n");
}

MODULE_LICENSE("Dual BSD/GPL");
MODULE_AUTHOR("ric");
MODULE_DESCRIPTION("LKM prova");

module_init(starting);
module_exit(exiting);
