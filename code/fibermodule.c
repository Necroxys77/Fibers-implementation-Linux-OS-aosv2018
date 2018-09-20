#include "fibermodule.h"
#include "macros.h"

DECLARE_HASHTABLE(threads, 10); //Defining hashtable named 'threads' sized 2^10 elements containing 
                                //only "primary" fibers
//DECLARE_HASHTABLE(fibers, 10); //Defining hashtable named 'fibers' sized 2^10 elements containing 
                                //only fibers created using createFibers()
//struct hlist_head fibers[2<<10] = {...}

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

    struct table_element_thread *obj;

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

static int can_create(pid_t pid, struct creator_thread *creator){

    struct table_element_thread *obj;

    hash_for_each_possible_rcu(threads, obj, table_node, pid){

        if (obj == NULL){
            printk(KERN_INFO "Hashtable is finished!\n");
            return 0;
        }

        if (obj->parent == pid){
            printk(KERN_INFO "Main thread has issued convertThreadToFiber! We are enabled to create a fiber!\n");
            creator->total_fibers = atomic_inc_return(&(obj->total_fibers));
            creator->creator_thread = obj; //Da unire queste due linee dic codice 
            //total_fibers = atomic_inc_return(&(obj->total_fibers));
            return 1;
        }
    }

    return 0;
}

static int convertThreadToFiber(void){

    struct pt_regs *regs;
    struct fiber *fiber;

    if (!is_authorized(current->pid))
        return 0;

    fiber = kmalloc(sizeof(struct fiber), GFP_KERNEL);
    atomic_set(&(fiber->active), 1);
    fiber->fiber_id = 0;
    regs = task_pt_regs(current);
    fiber->regs = regs; //Assigning thread regs to the primary fiber

    hashtable_thread_add(threads, current->pid, *fiber);

    return 1;
}

static int createFiber(struct ioctl_params *params){

    struct pt_regs *regs;
    struct fiber *fiber;
    int ret_can_create;
    struct creator_thread *creator;

    //I'm authorized if in the hashtable we find a struct fiber corresponding to the "main thread"
    //is the way around w.r.t. the is_authorized()

    creator = kmalloc(sizeof(struct creator_thread *), GFP_KERNEL);

    if (!(ret_can_create = can_create(current->pid, creator)))
        return 0;
        
    
    fiber = kmalloc(sizeof(struct fiber), GFP_KERNEL);
    regs = kmalloc(sizeof(struct pt_regs), GFP_KERNEL);

    //Copying into struct pt_regs some register in order to make the switch when the fiber 
    //will be executed
    regs->ip = params->user_func;
    regs->di = (unsigned long) params->args;
    regs->sp = (unsigned long) ((params->sp) + STACK_SIZE);
    regs->ss = regs->sp;

    //Now, populating the struct fiber. When created it is not active and we save the registers
    //for later execution
    fiber->fiber_id = creator->total_fibers;
    atomic_set(&(fiber->active), 0);
    fiber->regs = regs;

    hashtable_fiber_add(creator->creator_thread->fibers, fiber);

    return (fiber->fiber_id);
}

static long my_ioctl(struct file *file, unsigned int cmd, unsigned long arg){

    struct ioctl_params *params;
    int fiber_id;

    params = kmalloc(sizeof(struct ioctl_params), GFP_KERNEL);
    copy_from_user(params, (char *) arg, sizeof(struct ioctl_params));

    switch(cmd) {
        case convertF:

            if (!convertThreadToFiber()){
                params->fiber_id = -1; //Error
                printk(KERN_ALERT "Thread has already issued convertThreadToFiber!\n");
            } else{
                params->fiber_id = 0;
                printk(KERN_INFO "convertThreadToFiber: succeded!\n");
            }
            copy_to_user((char *) arg, params, sizeof(struct ioctl_params));

            kfree(params);

            break;
        case createF:

            if (!(fiber_id = createFiber(params))){
                params->fiber_id = -1;
                printk(KERN_ALERT "Fiber not authorized to create fibers! Main thread has not issued convertThreadToFiber yet!\n");
            } else{
                params->fiber_id = fiber_id;
                printk(KERN_INFO "createFiber: succeded!\n");
            }
            copy_to_user((char *) arg, params, sizeof(struct ioctl_params));

            kfree(params);

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
        printk(KERN_ALERT "Failed to createF the device!\n");
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
