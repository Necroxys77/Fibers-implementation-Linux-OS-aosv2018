#include "fibermodule.h"
#include "macros.h"
/* 
* This will be the hashtable containing all the processes which have issued
* convertThreadToFiber(). It is indexed by the tgid 
*/
static DEFINE_HASHTABLE(processes, 10); 

static int majorNumber;
static struct class* charClass  = NULL; // The device-driver class struct pointer
static struct device* charDevice = NULL; // The device-driver device struct pointer

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


//CONVERT
static void *convertThreadToFiber(void){
    process *current_process, *new_process;
    fiber *new_fiber, *current_fiber;
    int f_index;
    unsigned long new_fiber_id = 0;
    //int *return_value; //necessary to handle return
    
    //return_value = (int *) kmalloc(sizeof(int *), GFP_KERNEL);

    hash_for_each_possible_rcu(processes, current_process, table_node, current->tgid) {
        //at least one "companion" thread of the calling thread has issued a convert before
        if (current_process->tgid == current->tgid){
            //Check if calling thread is already created a fiber --> if there exist already my pid
            //in the fibers data structure, i already issued convert
            hash_for_each_rcu(current_process->fibers, f_index, current_fiber, table_node){
                if (current_fiber->parent_pid == current->pid) return  (void *) new_fiber_id;
            }
            /*Here it means that thread has never issued convert (up to now) we need to create a new thread and fiber*/
            /*Function that creates a fiber data structure given some parameters and inserts it in the hashtable????*/
            new_fiber = kmalloc(sizeof(fiber), GFP_KERNEL);
            new_fiber->context = kmalloc(sizeof(struct pt_regs), GFP_KERNEL);
            new_fiber->fpu_regs = kmalloc(sizeof(struct fpu), GFP_KERNEL);

            spin_lock_init(&(new_fiber->lock));
            memcpy(new_fiber->context, task_pt_regs(current), sizeof(struct pt_regs));
            copy_fxregs_to_kernel(new_fiber->fpu_regs);
            new_fiber->fiber_id = (unsigned long) atomic_inc_return(&(current_process->total_fibers));
            new_fiber->is_running = current->pid;
            new_fiber->tgid = current->tgid;
            new_fiber->parent_pid = current->pid;
            hash_add_rcu(current_process->fibers, &(new_fiber->table_node), new_fiber->fiber_id);
            
            return (void *) new_fiber->fiber_id;
        }
    }

    /*Some new process has issued convert! Need to create a new process and fiber data structures*/
    new_process = kmalloc(sizeof(process), GFP_KERNEL);
    new_fiber = kmalloc(sizeof(fiber), GFP_KERNEL);
    new_fiber->context = kmalloc(sizeof(struct pt_regs), GFP_KERNEL);
    new_fiber->fpu_regs = kmalloc(sizeof(struct fpu), GFP_KERNEL);

    new_process->tgid = current->tgid; //Key
    atomic_set(&(new_process->total_fibers), 0);
    hash_init(new_process->fibers);
    hash_add_rcu(processes, &(new_process->table_node), new_process->tgid);

    spin_lock_init(&(new_fiber->lock));
    memcpy(new_fiber->context, task_pt_regs(current), sizeof(struct pt_regs));
    copy_fxregs_to_kernel(new_fiber->fpu_regs);
    new_fiber->fiber_id =  (unsigned long) atomic_inc_return(&(new_process->total_fibers));
    new_fiber->is_running = current->pid;
    new_fiber->tgid = current->tgid;
    new_fiber->parent_pid = current->pid;
    hash_add_rcu(new_process->fibers, &(new_fiber->table_node), new_fiber->fiber_id);

    printk(KERN_INFO "[-] First converted thread of tgid %d\n",current->tgid);
    return (void *) new_fiber->fiber_id;
}


//CREATE
static void *createFiber(struct ioctl_params *params){
    process *current_process;
    int f_index;
    fiber *current_fiber, *new_fiber;
    //int *return_value;
    unsigned long new_fiber_id = 0;

    //return_value = (int *) kmalloc(sizeof(int *), GFP_KERNEL);

    hash_for_each_possible_rcu(processes, current_process, table_node, current->tgid){
        if (current_process->tgid == current->tgid){
            hash_for_each_rcu(current_process->fibers, f_index, current_fiber, table_node){
                if(current_fiber->parent_pid == current->pid){
                    new_fiber = kmalloc(sizeof(fiber), GFP_KERNEL);
                    new_fiber->context = kmalloc(sizeof(struct pt_regs), GFP_KERNEL);
                    new_fiber->fpu_regs = kmalloc(sizeof(struct fpu), GFP_KERNEL);
                    
                    spin_lock_init(&(new_fiber->lock));
                    memcpy(new_fiber->context, task_pt_regs(current), sizeof(struct pt_regs));
                    copy_fxregs_to_kernel(new_fiber->fpu_regs);
                    new_fiber->context->sp = (unsigned long) (params->sp + STACK_SIZE - 1);
                    new_fiber->context->bp = new_fiber->context->sp;
                    new_fiber->context->ip = (unsigned long )params->user_func;
                    new_fiber->context->di = (unsigned long) params->args;
                    new_fiber->fiber_id = (unsigned long) atomic_inc_return(&(current_process->total_fibers));
                    new_fiber->is_running = -1;
                    new_fiber->tgid = current->tgid;
                    new_fiber->parent_pid = current->pid;
                    hash_add_rcu(current_process->fibers, &(new_fiber->table_node), new_fiber->fiber_id);
                    printk(KERN_INFO "[-] New fiber created [id %lu, tgid %d, parent_pid %d]\n", new_fiber->fiber_id, new_fiber->tgid, new_fiber->parent_pid);
                    //*return_value = new_fiber->fiber_id;
                    return (void *) new_fiber->fiber_id; //thread issued convert so, it can create fibers
                }
            }
            printk(KERN_INFO "[-] Any fibers of the caller thread found...\n");
            return (void *) new_fiber_id; //process found, but thread not issued convert
        }
    }
    printk(KERN_INFO "[-] Process not found...\n");
    return (void *) new_fiber_id;  //process not found
}

/*
It checks if the calling fiber can switch to an other one. In the positive case it returns a pointer to the current fiber.
Returning the current one allow us to use a simple hash_for_each_possible_rcu in the second turn since we can use
the fiber_id, directly
*/
static fiber *can_switch(struct ioctl_params *params){
    process *current_process;
    fiber *current_fiber;
    fiber *calling_fiber=NULL;
    int f_index;

    //searching the process of the caller
    hash_for_each_possible_rcu(processes, current_process, table_node, current->tgid){
        //process of the caller found
        if(current_process->tgid == current->tgid){
            //printk(KERN_INFO "process hit: %d, current tgid %d\n", current_process->tgid, current->tgid);
            //searching for the calling fiber
            hash_for_each_rcu(current_process->fibers, f_index, current_fiber, table_node){
                //calling thread found. It has already issued a convert
                //printk(KERN_INFO "current fiber - is running: %d\n", current_fiber->is_running);
                if(current_fiber->is_running == current->pid){
                    //printk(KERN_INFO "fiber hit: %d, current pid %d\n", current_fiber->parent_pid, current->pid);
                    calling_fiber = current_fiber;
                    return calling_fiber;
                }
            }
        }
    }
    //there is no caller companion thread that issued at least a convert 
    return calling_fiber;
}


/*
It searches the target fiber
*/
static fiber *search_target(unsigned long target_id){
    process *current_process;
    fiber *current_fiber;
    fiber *target_fiber=NULL;

    //searching the process of the caller
    hash_for_each_possible_rcu(processes, current_process, table_node, current->tgid){
        //process of the caller found
        if(current_process->tgid == current->tgid){
            hash_for_each_possible_rcu(current_process->fibers, current_fiber, table_node, target_id){
                //target fiber found
                if(current_fiber->fiber_id == target_id){
                    target_fiber = current_fiber;
                    return target_fiber;
                }
            }
        }
    }
    //target fiber doesn't exist
    return target_fiber;
}


/*
NOTE: maybe we can lose a fiber between some checks. check concurrency
params->id is set to 0 when the switch failed for some reasons
*/

//SWITCH
static void switchToFiber(struct ioctl_params *params){

    fiber *calling_fiber, *target_fiber;
    struct pt_regs *old_context;
    unsigned long flags;
    
    //checking if the calling fiber can switch
    calling_fiber = can_switch(params);
    if (calling_fiber!=NULL){
        printk(KERN_INFO "[-] Calling fiber [id %lu, tgid %d, parent_pid %d, pid_running_thread %d] \n", calling_fiber->fiber_id, calling_fiber->tgid, calling_fiber->parent_pid, calling_fiber->is_running);
        //searching the target fiber
        target_fiber = search_target(params->fiber_id);
        if(target_fiber!=NULL){
            printk(KERN_INFO "[-] Target fiber [id %lu, tgid %d, parent_pid %d, pid_running_thread %d] \n", target_fiber->fiber_id, target_fiber->tgid, target_fiber->parent_pid, target_fiber->is_running);
            spin_lock_irqsave(&(target_fiber->lock), flags);
            if(target_fiber->is_running == -1){
                //switching...
                target_fiber->is_running = current->pid;
                old_context = task_pt_regs(current);
                
                /* copying registers from current to the calling fiber */
                calling_fiber->context->r15 = old_context->r15;
                calling_fiber->context->r14 = old_context->r14;
                calling_fiber->context->r13 = old_context->r13;
                calling_fiber->context->r12 = old_context->r12;
                calling_fiber->context->r11 = old_context->r11;
                calling_fiber->context->r10 = old_context->r10;
                calling_fiber->context->r9 = old_context->r9;
                calling_fiber->context->r8 = old_context->r8;
                calling_fiber->context->ax = old_context->ax;
                calling_fiber->context->bx = old_context->bx; 
                calling_fiber->context->cx = old_context->cx; 
                calling_fiber->context->dx = old_context->dx; 
                calling_fiber->context->si = old_context->si; 
                calling_fiber->context->di = old_context->di; 
                calling_fiber->context->sp = old_context->sp; 
                calling_fiber->context->ip = old_context->ip; 
                calling_fiber->context->bp = old_context->bp; 
                calling_fiber->context->flags = old_context->flags;
                copy_fxregs_to_kernel(calling_fiber->fpu_regs);

                /* copying registers from the target fiber to current*/
                old_context->r15 = target_fiber->context->r15;
                old_context->r14 = target_fiber->context->r14;
                old_context->r13 = target_fiber->context->r13;
                old_context->r12 = target_fiber->context->r12;
                old_context->r11 = target_fiber->context->r11;
                old_context->r10 = target_fiber->context->r10;
                old_context->r9 = target_fiber->context->r9;
                old_context->r8 = target_fiber->context->r8;
                old_context->ax = target_fiber->context->ax;
                old_context->bx = target_fiber->context->bx;
                old_context->cx = target_fiber->context->cx;
                old_context->dx = target_fiber->context->dx;
                old_context->si = target_fiber->context->si;
                old_context->di = target_fiber->context->di;
                old_context->sp = target_fiber->context->sp;
                old_context->ip = target_fiber->context->ip;
                old_context->bp = target_fiber->context->bp;
                old_context->flags = target_fiber->context->flags;
                copy_kernel_to_fxregs(&(target_fiber->fpu_regs->state.fxsave));

                //releasing the calling fiber
                calling_fiber->is_running = -1;

            } else {
                printk(KERN_INFO "[-] Target fiber is already running somewhere\n");
                params->fiber_id = 0; //if the target fiber is already running
            }
            spin_unlock_irqrestore(&(target_fiber->lock), flags);
        } else {
            printk(KERN_INFO "[-] Target fiber %lu doesn't exist\n", params->fiber_id);
            params->fiber_id = 0; //if the target fiber doesn't exist
        } 
    } else {
        printk(KERN_INFO "[-] Calling thread %d is not a fiber \n", current->pid);
        params->fiber_id = 0; //if the calling thread is not a fiber
    }
}

//ENTRY POINT FROM USER SPACE
static long my_ioctl(struct file *file, unsigned int cmd, unsigned long arg){

    struct ioctl_params *params;
    //int *return_value, 
    unsigned long fiber_id;

    params = kmalloc(sizeof(struct ioctl_params), GFP_KERNEL);
    copy_from_user(params, (char *) arg, sizeof(struct ioctl_params));

    switch(cmd) {
        case CONVERT:
            printk(KERN_INFO "[-] Converting thread into a fiber... [tgid %d, pid %d]\n", current->tgid, current->pid);
            fiber_id = (unsigned long) convertThreadToFiber();

            if (!fiber_id){
                params->fiber_id = 0; //error
                printk(KERN_ALERT "[!] Thread has already issued convertThreadToFiber! [tgid %d, pid %d]\n", current->tgid, current->pid);
            } else {
                //value_to_user = *return_value;
                params->fiber_id = fiber_id;
                printk(KERN_INFO "[+] convertThreadToFiber: succeded!\n");
            }
            //passing to user space some useful information
            copy_to_user((char *) arg, params, sizeof(struct ioctl_params));
            
            //kfree(return_value);
            kfree(params);

            break;

        case CREATE:
            printk(KERN_INFO "[-] Creating a fiber... [tgid %d, pid %d]\n", current->tgid, current->pid);
            fiber_id = (unsigned long) createFiber(params);

            if (!fiber_id){
                params->fiber_id = 0; //error
                printk(KERN_ALERT "[!] Thread not authorized to create fibers! convertThreadToFiber not issued yet!\n");
            } else {
                //value_to_user = *return_value;
                params->fiber_id = fiber_id;
                printk(KERN_INFO "[+] createFiber: succeded!\n");
            }
            //passing to user space some useful information
            copy_to_user((char *) arg, params, sizeof(struct ioctl_params));

            //kfree(return_value);
            kfree(params);  

            break;

        case SWITCH:
            printk(KERN_INFO "[-] Switching to Fiber %lu ...\n", params->fiber_id);
            switchToFiber(params);
            //printk(KERN_INFO "params fiber id: %d\n", params->fiber_id);
            
            if (!params->fiber_id) {
                printk(KERN_ALERT "[!] Impossible to switch to requested fiber!\n");
            } else printk(KERN_INFO "[+] switchToFiber: succeded!\n");
            //passing to user space some useful information
            copy_to_user((char *) arg, params, sizeof(struct ioctl_params));

            kfree(params);
    }
    return 0;
}


/*
When the module is removed all the related data structures are deallocated
*/
static void clean_up(void){
    process *current_process;
    fiber *current_fiber;
    int f_index, p_index;

    hash_for_each_rcu(processes, p_index, current_process, table_node){
        printk(KERN_INFO "[-] Deleting fibers of process %d...", current_process->tgid);
        hash_for_each_rcu(current_process->fibers, f_index, current_fiber, table_node){
            printk(KERN_INFO "[-] Deleting fiber %lu...", current_fiber->fiber_id);
            kfree(current_fiber->context);
            kfree(current_fiber->fpu_regs);
            hash_del_rcu(&(current_fiber->table_node));
            kfree(current_fiber);
        }
        hash_del_rcu(&(current_process->table_node));
        kfree(current_process);
        printk(KERN_INFO "[+] Fiber hashtable of process %d is empty!", current_process->tgid);
    }
    printk(KERN_INFO "[+] Process hashtable is empty!");
}



/*
static int clean_up_data(void){

    int bkt_thread, bkt_fiber;
    struct table_element_thread *obj_thread;
    struct table_element_fiber *obj_fiber;

    hash_for_each_rcu(threads, bkt_thread, obj_thread, table_node){
        
        if (obj_thread == NULL){
            printk(KERN_INFO "Thread hashtable is finished!\n");
            return 0;
        }
        printk(KERN_INFO "Deleting table_element_fibers of thread: %d\n", obj_thread->parent);

        kfree(obj_thread->fiber);

        hash_for_each_rcu(obj_thread->fibers, bkt_fiber, obj_fiber, table_node){
                    
            if (obj_fiber == NULL){
                printk(KERN_INFO "Hashtable is finished!\n");
                return 0;
            }
            printk(KERN_INFO "Deleting info about fiber: %d\n", obj_fiber->fiber->fiber_id);

            kfree(obj_fiber->fiber->regs);
            kfree(obj_fiber->fiber);

            hash_del_rcu(&(obj_fiber->table_node));
        }
        printk(KERN_INFO "Fibers hashtable of thread %d empty: %d\n", obj_thread->parent, hash_empty(obj_thread->fibers));
        
        printk(KERN_INFO "Deleting info about thread: %d\n", obj_thread->parent);
        hash_del_rcu(&(obj_thread->table_node));
    }
    printk(KERN_INFO "Threads hashtable empty: %d\n", hash_empty(threads));
    return 1;
}
*/
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
    
    printk(KERN_INFO "We are exiting..\n");
    clean_up();
    //printk(KERN_INFO "Finished to clean up all data structures: %d\n", clean_up_err);
    device_destroy(charClass, MKDEV(majorNumber, 0));     // remove the device
    class_unregister(charClass);                          // unregister the device class
    class_destroy(charClass);                             // remove the device class
    unregister_chrdev(majorNumber, "DeviceName");             // unregister the major number
    printk(KERN_INFO "Goodbye from the LKM!\n");
}

MODULE_LICENSE("Dual BSD/GPL");
MODULE_AUTHOR("ricmat");
MODULE_DESCRIPTION("fiber module");

module_init(starting);
module_exit(exiting);
