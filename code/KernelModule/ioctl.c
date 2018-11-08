#include "ioctl.h"

//static DEFINE_HASHTABLE(processes, 10); //QUI?
static int majorNumber;
static struct class* charClass  = NULL; // The device-driver class struct pointer
static struct device* charDevice = NULL; // The device-driver device struct pointer
struct kprobe kp_do_exit;
struct kretprobe kp_schedule, kp_proc, kp_proc_lookup;

static long my_ioctl(struct file *file, unsigned int cmd, unsigned long arg);

static struct file_operations fops =
{
    .owner = THIS_MODULE,
    .unlocked_ioctl= my_ioctl
    //.release = my_release
};


static char *unlock_sudo(struct device *dev, umode_t *mode){
    if (mode){
        *mode = 0666;
    }
    return kasprintf(GFP_KERNEL, "%s", dev_name(dev));
}


static long my_ioctl(struct file *file, unsigned int cmd, unsigned long arg){
    int fiber_id, success;
    long pos;
    long long value;
    struct ioctl_params *params;

    switch(cmd){
        case CONVERT:
            //printk(KERN_INFO "[-] Converting thread into a fiber... [tgid %d, pid %d]\n", current->tgid, current->pid);
            fiber_id = convertThreadToFiber();
            printk("%d\n", current->pid);
            /*
            if(fiber_id)
                printk(KERN_INFO "[+] convertThreadToFiber: succeded!\n");
            else
                printk(KERN_ALERT "[!] Thread has already issued convertThreadToFiber! [tgid %d, pid %d]\n", current->tgid, current->pid);
            */
            return fiber_id;
        
        case CREATE:
            params = kzalloc(sizeof(struct ioctl_params), GFP_KERNEL);
            if(copy_from_user(params, (void *) arg, sizeof(struct ioctl_params))){
                kfree(params);
                return 0; //error copy failed
            }

            //printk(KERN_INFO "[-] Creating a fiber... [tgid %d, pid %d]\n", current->tgid, current->pid);
            fiber_id = createFiber(params->sp, params->user_func, params->args);

            /*
            if(fiber_id)
                printk(KERN_INFO "[+] createFiber: succeded!\n");
            else
                printk(KERN_ALERT "[!] Thread not authorized to create fibers! convertThreadToFiber not issued yet!\n");
            */
            kfree(params);
            return fiber_id;

        case SWITCH:
            params = kzalloc(sizeof(struct ioctl_params), GFP_KERNEL);
            if(copy_from_user(params, (void *) arg, sizeof(struct ioctl_params))){
                kfree(params);
                return 0; //error copy failed
            }

            //printk(KERN_INFO "[-] Switching to Fiber %d ...\n", params->fiber_id);
            success = switchToFiber(params->fiber_id);
            /*
            if(success)
                printk(KERN_INFO "[+] switchToFiber: succeded!\n");
            else
                printk(KERN_ALERT "[!] Impossible to switch to fiber %d!\n", params->fiber_id);
            */
            kfree(params);
            return success;

        case FLSALLOC:
            pos = flsAlloc();
            /*
            if(pos!=-1)
                printk(KERN_INFO "[+] flsAlloc: succeded! pos %ld\n",pos);
            else
                printk(KERN_ALERT "[!] No more space in fiber fls...\n");
            */
            return pos;

        case FLSSET:
            params = kzalloc(sizeof(struct ioctl_params), GFP_KERNEL);
            if(copy_from_user(params, (void *) arg, sizeof(struct ioctl_params))){
                kfree(params);
                return 0; //error copy failed
            }

            success = flsSet(params->pos, params->value);
            /*
            if(success)
                printk(KERN_INFO "[+] flsSet: succeded! value %lld to pos %ld\n", params->value, params->pos);
            else
                printk(KERN_INFO "[!] flsSet: failed! value %lld to pos %ld\n", params->value, params->pos);
            */
            kfree(params);
            return success;

        case FLSGET:
            params = kzalloc(sizeof(struct ioctl_params), GFP_KERNEL);
            if(copy_from_user(params, (void *) arg, sizeof(struct ioctl_params))){
                kfree(params);
                return 0; //error copy failed
            }
            
            value = flsGet(params->pos);
            /*if(value!=0){
                printk(KERN_INFO "[+] flsGet: succeded! value %lld from pos %ld\n", (long long) value, params->pos);
                params->value = (long long) value; success = 1;
            } else{
                printk(KERN_INFO "[!] flsGet: failed! pos %ld\n",params->pos);
                success = 0;
            }*/

            //printk(KERN_INFO "[+] flsGet: succeded! value %lld from pos %ld\n", (long long) value, params->pos);
            params->value = value; 

            if (copy_to_user((void *) arg, params, sizeof(struct ioctl_params)) != 0){
                kfree(params);
                return 0; //error copy failed
            }
            success = 1;
            
            kfree(params);
            return success;

        case FLSFREE:

            params = kzalloc(sizeof(struct ioctl_params), GFP_KERNEL);
            if(copy_from_user(params, (void *) arg, sizeof(struct ioctl_params))){
                kfree(params);
                return 0; //error copy failed
            }

            success = flsFree(params->pos);
            /*
            if(success)
                 printk(KERN_INFO "[+] flsFree: succeded! pos %ld\n", params->pos);
            else
                printk(KERN_INFO "[!] flsFree: failed! pos %ld\n", params->pos);
            */
            kfree(params);
            return success;
    }

    return -1;
}


static int __init starting(void){

    int ret_probe;

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
        printk(KERN_ALERT "Failed to createF the device!\n");
        return PTR_ERR(charDevice);
    }

    /*Registering Kprobes*/
    memset(&kp_do_exit, 0, sizeof(kp_do_exit));
    if ((ret_probe = register_kp(&kp_do_exit)) < 0){
        printk(KERN_ALERT "In init, failed to register probe!\n");
    }

    memset(&kp_schedule, 0, sizeof(kp_schedule));
    if ((ret_probe = register_kretp(&kp_schedule)) < 0){
        printk(KERN_ALERT "In init, failed to register kretprobe!\n");
    }

    memset(&kp_proc, 0, sizeof(kp_proc));
    if ((ret_probe = register_kretp_proc_readdir(&kp_proc)) < 0){
        printk(KERN_ALERT "In init, failed to register kretprobe!\n");
    }

    memset(&kp_proc_lookup, 0, sizeof(kp_proc_lookup));
    if ((ret_probe = register_kretp_proc_lookup(&kp_proc_lookup)) < 0){
        printk(KERN_ALERT "In init, failed to register kretprobe!\n");
    }

    printk(KERN_INFO "Device class created correctly, __init finished!\n"); // Made it! device was initialized
    return 0;
}


static void __exit exiting(void){
    printk(KERN_INFO "We are exiting..\n");
    device_destroy(charClass, MKDEV(majorNumber, 0));     // remove the device
    class_unregister(charClass);                          // unregister the device class
    class_destroy(charClass);                             // remove the device class
    unregister_chrdev(majorNumber, "DeviceName");             // unregister the major number
    unregister_kp(&kp_do_exit);
    unregister_kretp(&kp_schedule);
    unregister_kretp(&kp_proc);
    unregister_kretp(&kp_proc_lookup);
    printk(KERN_INFO "Goodbye from the LKM!\n");
}

MODULE_LICENSE("Dual BSD/GPL");
MODULE_AUTHOR("ricmat");
MODULE_DESCRIPTION("fiber module");

module_init(starting);
module_exit(exiting);