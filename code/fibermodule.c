#include "fibermodule.h"

DEFINE_HASHTABLE(threads, 10); //Defining hashtable named 'threads' sized 2^10 elements

static int majorNumber;
static struct class* charClass  = NULL; //< The device-driver class struct pointer
static struct device* charDevice = NULL; //< The device-driver device struct pointer

static long my_ioctl(struct file *file, unsigned int cmd, unsigned long arg);

static struct file_operations fops =
{
    .owner = THIS_MODULE,
    .unlocked_ioctl= my_ioctl
};

static char *unlock_sudo(struct device *dev, umode_t *mode){

    if (mode){
        *mode = 0666;
    }
    return kasprintf(GFP_KERNEL, "%s", dev_name(dev));
}

static int is_authorized(pid_t pid){

    struct table_element *obj;
    struct fiber *fib;

    obj = kmalloc(sizeof(struct table_element), GFP_KERNEL);
    fib = kmalloc(sizeof(struct fiber), GFP_KERNEL);
    fib = &(obj->fiber);
    obj->fiber.fiber_id = 4;

    hash_for_each_possible_rcu(threads, obj, table_node, pid){
        if (obj == NULL){
            printk(KERN_INFO "Hashtable is finished!\n");
            return 1;
        }

        if (obj->parent == pid){
            printk(KERN_INFO "Parent is already a fiber!\n");
            return 0;
        }
    }
    return 1;
}

static void hashtable_add(pid_t pid, struct fiber **fiber){

    struct table_element *new_element;
    struct fiber *new_fiber;

    new_element = kmalloc(sizeof(struct table_element), GFP_KERNEL);
    new_element->parent = pid;
    new_fiber = &(new_element->fiber);

    hash_add_rcu(threads, &(new_element->table_node), new_element->parent);
}

static int ConverThreadToFiber(struct pt_regs *regs, struct fiber *primary_fiber){
    if (!is_authorized(current->pid))
        return 0;

    primary_fiber = kmalloc(sizeof(struct fiber), GFP_KERNEL);
    primary_fiber->active = 1;
    regs = task_pt_regs(current);
    primary_fiber->regs = regs; //Assigning thread regs to the primary fiber

    hashtable_add(current->pid, &primary_fiber);

    return 1;
}

static long my_ioctl(struct file *file, unsigned int cmd, unsigned long arg){

    struct pt_regs *regs;
    struct fiber *primary_fiber;

    switch(cmd) {
        case convertThreadToFiber:

            if (!ConverThreadToFiber(regs, primary_fiber)){
                printk(KERN_ALERT "Thread not authorized to become fiber!\n");
                return 0;
            }

            printk(KERN_INFO "ConvertThreadToFiber: succeded!\n");

            break;
        case 2:
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
    charClass->devnode = unlock_sudo;
    printk(KERN_INFO "Device class registered correctly!\n");

    // Register the device driver
    charDevice = device_create(charClass, NULL, MKDEV(majorNumber, 0), NULL, "DeviceName");
    if (IS_ERR(charDevice)){               // Clean up if there is an error
        class_destroy(charClass);           // Repeated code but the alternative is goto statements
        unregister_chrdev(majorNumber, "DeviceName");
        printk(KERN_ALERT "Failed to create the device!\n");
        return PTR_ERR(charDevice);
    }

    hash_init(threads); //Initializing previously declared hashtable

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
