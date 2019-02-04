#include "ioctl.h"

static int majorNumber;
static struct class* charClass  = NULL;
static struct device* charDevice = NULL;
struct kprobe kp_do_exit;
struct kretprobe kretp_finish_task_switch, kretp_proc_readdir, kretp_proc_lookup;

static long my_ioctl(struct file *file, unsigned int cmd, unsigned long arg);

static struct file_operations fops = {
    .owner = THIS_MODULE,
    .unlocked_ioctl= my_ioctl
};


static char *unlock_sudo(struct device *dev, umode_t *mode){
    if (mode){
        *mode = 0666;
    }
    return kasprintf(GFP_KERNEL, "%s", dev_name(dev));
}


static long my_ioctl(struct file *file, unsigned int cmd, unsigned long arg){
    int fiber_id, ret;
    long pos;
    long long value;
    struct ioctl_params *params;

    switch(cmd){
        case CONVERT:
            fiber_id = convertThreadToFiber();
            printk("[-] Convert: tgid %d, pid %d\n", current->tgid, current->pid);
            if(!fiber_id)
                printk(KERN_ALERT "[!] Convert: thread has already issued convertThreadToFiber! [tgid %d, pid %d]\n", current->tgid, current->pid);
            
            return fiber_id;

        case CREATE:
            params = kzalloc(sizeof(struct ioctl_params), GFP_KERNEL);
            if(copy_from_user(params, (void *) arg, sizeof(struct ioctl_params))){
                kfree(params);
                return 0;
            }
            fiber_id = createFiber(params->sp, params->user_func, params->args);
            if(!fiber_id)
                printk(KERN_ALERT "[!] Create: thread not authorized to create fibers! convertThreadToFiber not issued yet!\n");
            
            kfree(params);
            return fiber_id;

        case SWITCH:
            params = kzalloc(sizeof(struct ioctl_params), GFP_KERNEL);
            if(copy_from_user(params, (void *) arg, sizeof(struct ioctl_params))){
                kfree(params);
                return 0;
            }
            ret = switchToFiber(params->fiber_id);
            /*
            if(!ret)
                printk(KERN_ALERT "[!] Impossible to switch to fiber %d!\n", params->fiber_id);
            */
            kfree(params);
            return ret;

        case FLSALLOC:
            pos = flsAlloc();

            return pos;

        case FLSSET:
            params = kzalloc(sizeof(struct ioctl_params), GFP_KERNEL);
            if(copy_from_user(params, (void *) arg, sizeof(struct ioctl_params))){
                kfree(params);
                return 0;
            }
            ret = flsSet(params->pos, params->value);
            
            kfree(params);
            return ret;

        case FLSGET:
            params = kzalloc(sizeof(struct ioctl_params), GFP_KERNEL);
            if(copy_from_user(params, (void *) arg, sizeof(struct ioctl_params))){
                kfree(params);
                return 0;
            }
            value = flsGet(params->pos);
            params->value = value; 
            if (copy_to_user((void *) arg, params, sizeof(struct ioctl_params)) != 0){
                kfree(params);
                return 0;
            }
            
            kfree(params);
            return 1;

        case FLSFREE:
            params = kzalloc(sizeof(struct ioctl_params), GFP_KERNEL);
            if(copy_from_user(params, (void *) arg, sizeof(struct ioctl_params))){
                kfree(params);
                return 0;
            }
            ret = flsFree(params->pos);
            
            kfree(params);
            return ret;
    }
    return -1;
}


static int __init starting(void){
    int ret_probe;

    printk(KERN_INFO "We are in _init!\n");
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

    
    /*Registering Kprobes and Kretprobes*/
    
    memset(&kp_do_exit, 0, sizeof(kp_do_exit));
    if ((ret_probe = register_kp_doExit(&kp_do_exit)) < 0){
        printk(KERN_ALERT "In init, failed to register kprobe for do exit!\n");
    }

    memset(&kretp_finish_task_switch, 0, sizeof(kretp_finish_task_switch));
    if ((ret_probe = register_kretp_finishTaskSwitch(&kretp_finish_task_switch)) < 0){
        printk(KERN_ALERT "In init, failed to register kretprobe for finish_task_switch!\n");
    }

    
    memset(&kretp_proc_readdir, 0, sizeof(kretp_proc_readdir));
    if ((ret_probe = register_kretp_procReaddir(&kretp_proc_readdir)) < 0){
        printk(KERN_ALERT "In init, failed to register kretprobe for proc_readdir!\n");
    }

    memset(&kretp_proc_lookup, 0, sizeof(kretp_proc_lookup));
    if ((ret_probe = register_kretp_procLookup(&kretp_proc_lookup)) < 0){
        printk(KERN_ALERT "In init, failed to register kretprobe for proc_lookup!\n");
    }

    printk(KERN_INFO "Device class created correctly, __init finished!\n");
    return 0;
}


static void __exit exiting(void){
    printk(KERN_INFO "We are exiting..\n");
    device_destroy(charClass, MKDEV(majorNumber, 0));     // remove the device
    class_unregister(charClass);                          // unregister the device class
    class_destroy(charClass);                             // remove the device class
    unregister_chrdev(majorNumber, "DeviceName");             // unregister the major number
    unregister_kp_doExit(&kp_do_exit);
    unregister_kretp(&kretp_finish_task_switch);
    unregister_kretp(&kretp_proc_readdir);
    unregister_kretp(&kretp_proc_lookup);
    printk(KERN_INFO "Goodbye from the LKM!\n");
}

MODULE_LICENSE("Dual BSD/GPL");
MODULE_AUTHOR("Matteo Mariani (1815188), Riccardo Chiaretti (1661390)");
MODULE_DESCRIPTION("fiber module");

module_init(starting);
module_exit(exiting);