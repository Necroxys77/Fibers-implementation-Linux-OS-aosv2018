#include "fiber.h"

DEFINE_HASHTABLE(processes, 10);
//EXPORT_SYMBOL(processes);

//CONVERT
int convertThreadToFiber(void){
    fiber *current_fiber, *new_fiber;
    process *current_process, *new_process;
    int f_index;
    int new_fiber_id = 0;

    hash_for_each_possible_rcu(processes, current_process, table_node, current->tgid){
        if(current_process->tgid == current->tgid){
            hash_for_each_rcu(current_process->fibers, f_index, current_fiber, table_node){
                if(current_fiber->parent_pid == current->pid)
                    return new_fiber_id; //error
            }
            new_fiber = kzalloc(sizeof(fiber), GFP_KERNEL);
            new_fiber->context = kzalloc(sizeof(struct pt_regs), GFP_KERNEL);
            new_fiber->fpu_regs = kzalloc(sizeof(struct fpu), GFP_KERNEL);
            bitmap_zero(new_fiber->fls_bitmap, MAX_SIZE_FLS);
            memset(new_fiber->fls, 0, sizeof(long long)*MAX_SIZE_FLS);

            spin_lock_init(&(new_fiber->lock));
            memcpy(new_fiber->context, task_pt_regs(current), sizeof(struct pt_regs));
            copy_fxregs_to_kernel(new_fiber->fpu_regs);
            new_fiber->fiber_id = atomic_inc_return(&(current_process->total_fibers));
            new_fiber->running_by = current->pid;
            new_fiber->parent_pid = current->pid;
            new_fiber->finalized_activations = 1;
            new_fiber->failed_activations = 0;

            hash_add_rcu(current_process->fibers, &(new_fiber->table_node), new_fiber->fiber_id);
            
            return new_fiber->fiber_id;
        }
    }
    //if the process is new, allocate the process too
    new_process = kzalloc(sizeof(process), GFP_KERNEL);
    new_process->tgid = current->tgid; //Key
    atomic_set(&(new_process->total_fibers), 0);
    hash_init(new_process->fibers);
    hash_add_rcu(processes, &(new_process->table_node), new_process->tgid);

    new_fiber = kzalloc(sizeof(fiber), GFP_KERNEL);
    new_fiber->context = kzalloc(sizeof(struct pt_regs), GFP_KERNEL);
    new_fiber->fpu_regs = kzalloc(sizeof(struct fpu), GFP_KERNEL);
    bitmap_zero(new_fiber->fls_bitmap, MAX_SIZE_FLS);
    memset(new_fiber->fls, 0, sizeof(long long)*MAX_SIZE_FLS);

    spin_lock_init(&(new_fiber->lock));
    memcpy(new_fiber->context, task_pt_regs(current), sizeof(struct pt_regs));
    copy_fxregs_to_kernel(new_fiber->fpu_regs);
    new_fiber->fiber_id = atomic_inc_return(&(new_process->total_fibers));
    new_fiber->running_by = current->pid;
    new_fiber->parent_pid = current->pid;
    new_fiber->finalized_activations = 1;
    new_fiber->failed_activations = 0;
    
    hash_add_rcu(new_process->fibers, &(new_fiber->table_node), new_fiber->fiber_id);
    
    printk(KERN_INFO "[-] First converted thread of tgid %d\n",current->tgid);
    return new_fiber->fiber_id;
}


//CREATE
int createFiber(unsigned long sp, entry_point user_function, void *args){
    fiber *current_fiber, *new_fiber;
    process *current_process;
    int f_index;
    int new_fiber_id = 0;

    hash_for_each_possible_rcu(processes, current_process, table_node, current->tgid){
        if(current_process->tgid == current->tgid){
            hash_for_each_rcu(current_process->fibers, f_index, current_fiber, table_node){
                if(current_fiber->parent_pid == current->pid){
                    new_fiber = kzalloc(sizeof(fiber), GFP_KERNEL);
                    new_fiber->context = kzalloc(sizeof(struct pt_regs), GFP_KERNEL);
                    new_fiber->fpu_regs = kzalloc(sizeof(struct fpu), GFP_KERNEL);
                    bitmap_zero(new_fiber->fls_bitmap, MAX_SIZE_FLS);
                    memset(new_fiber->fls, 0, sizeof(long long)*MAX_SIZE_FLS);

                    spin_lock_init(&(new_fiber->lock));
                    memcpy(new_fiber->context, task_pt_regs(current), sizeof(struct pt_regs));
                    copy_fxregs_to_kernel(new_fiber->fpu_regs);

                    new_fiber->context->sp = (unsigned long) (sp + STACK_SIZE - 8);
                    new_fiber->context->bp = new_fiber->context->sp;
                    new_fiber->context->ip = (unsigned long) user_function;
                    new_fiber->context->di = (unsigned long) args;
                    new_fiber->fiber_id = atomic_inc_return(&(current_process->total_fibers));
                    new_fiber->running_by = -1; //pid_t can be -1?
                    new_fiber->parent_pid = current->pid;
                    new_fiber->finalized_activations = 0;
                    new_fiber->failed_activations = 0;

                    hash_add_rcu(current_process->fibers, &(new_fiber->table_node), new_fiber->fiber_id);
                    printk(KERN_INFO "[-] New fiber created [id %d, tgid %d, parent_pid %d]\n", new_fiber->fiber_id, current_process->tgid, new_fiber->parent_pid);

                    return new_fiber->fiber_id;
                }
            }
            printk(KERN_INFO "[-] Any fibers of the caller thread found...\n");
            return new_fiber_id; //process found, but thread not issued convert
        }
    }
    printk(KERN_INFO "[-] Process not found...\n"); 
    return new_fiber_id;  //process not found
}

// SWITCH

/*
It checks if the calling fiber can switch to an other one. In the positive case it returns a pointer to the current fiber.
Returning the current one allow us to use a simple hash_for_each_possible_rcu in the second turn since we can use
the fiber_id, directly
*/
fiber *can_switch(void){
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
                if(current_fiber->running_by == current->pid){
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
fiber *search_target_fiber(int target_fiber_id){
    process *current_process;
    fiber *current_fiber;
    fiber *target_fiber=NULL;

    //searching the process of the caller
    hash_for_each_possible_rcu(processes, current_process, table_node, current->tgid){
        //process of the caller found
        if(current_process->tgid == current->tgid){
            hash_for_each_possible_rcu(current_process->fibers, current_fiber, table_node, target_fiber_id){
                //target fiber found
                if(current_fiber->fiber_id == target_fiber_id){
                    target_fiber = current_fiber;
                    return target_fiber;
                }
            }
        }
    }
    //target fiber doesn't exist
    return target_fiber;
}


int switchToFiber(int target_fiber_id){
    fiber *calling_fiber, *target_fiber;
    struct pt_regs *old_context;
    //struct fpu *next_fpu;
    //struct fxregs_state *next_fx_regs;
    unsigned long flags;
    int success = 0;

    //checking if the calling fiber can switch
    calling_fiber = can_switch();
    if (calling_fiber!=NULL){
        printk(KERN_INFO "[-] Calling fiber [id %d, tgid %d, parent_pid %d, pid_running_thread %d] \n", calling_fiber->fiber_id, current->tgid, calling_fiber->parent_pid, calling_fiber->running_by);
        //searching the target fiber
        target_fiber = search_target_fiber(target_fiber_id);
        if(target_fiber!=NULL){
            //preempt_disable();
            printk(KERN_INFO "[-] Target fiber [id %d, tgid %d, parent_pid %d, pid_running_thread %d] \n", target_fiber->fiber_id, current->tgid, target_fiber->parent_pid, target_fiber->running_by);
            spin_lock_irqsave(&(target_fiber->lock), flags);
            if(target_fiber->running_by == -1){
                target_fiber->running_by = current->pid;
                old_context = task_pt_regs(current);

                //copying registers from current to the calling fiber
                memcpy(calling_fiber->context, old_context, sizeof(struct pt_regs));
                copy_fxregs_to_kernel(calling_fiber->fpu_regs);

                //copying registres from target fiber to current
                memcpy(old_context, target_fiber->context, sizeof(struct pt_regs));
                copy_kernel_to_fxregs(&(target_fiber->fpu_regs->state.fxsave));

                //For statistics
                target_fiber->finalized_activations += 1;

                //realising calling fiber
                calling_fiber->running_by = -1;
                success = 1;
            } else{
                printk(KERN_INFO "[-] Target fiber is already running by %d\n", target_fiber->running_by);
                target_fiber->failed_activations += 1;
            }
            spin_unlock_irqrestore(&(target_fiber->lock), flags);
        } else
            printk(KERN_INFO "[-] Target fiber %d doesn't exist\n", target_fiber_id);
    } else
        printk(KERN_INFO "[-] Calling thread %d is not a fiber \n", current->pid);

    return success;
}


//FLS ALLOC
long flsAlloc(){
    process *current_process;
    fiber *current_fiber;
    long pos; //bit
    int f_index;

    hash_for_each_possible_rcu(processes, current_process, table_node, current->tgid){
        if(current->tgid == current_process->tgid){
            hash_for_each_rcu(current_process->fibers, f_index, current_fiber, table_node){
                if(current_fiber->running_by == current->pid){
                    pos = find_first_zero_bit(current_fiber->fls_bitmap, MAX_SIZE_FLS);
                    if (pos == MAX_SIZE_FLS){
                        printk(KERN_ALERT "[!] fls full for fiber %d\n", current_fiber->fiber_id);
                        return -1;
                    }
                    change_bit(pos, current_fiber->fls_bitmap);
                    return pos;
                }
            }
        }
    }
    printk(KERN_ALERT "[!] Process/fiber doesn't found in the hashtable...\n");
    return -1; //process/fiber not found
}

//FLS SET
// possible problems, 
// (1) if flsSet fails then the bit on the bitmap should be changed again since the position is not used anymore.
// (2) if the user pass any pos, i can overwrite that position since we don't apply any check.
// there should be something like: test_bit(pos, current_fiber->fls_bitmap) && *current_fiber->fls[pos]=**original_value_bitmap**)
//BUT, what is **original_value_bitmap**?

/*Riccardo's answer:
We cannot do (1) because that position has been allocated by flsAlloc() so, it is still allocated. By the way, flsSet() cannot fail.
For (2), we can only check if the corresponding bit is set to 1 which means that the alloc has "allocated" it.
*/
int flsSet(long pos, long long value){
    process *current_process;
    fiber *current_fiber;
    int f_index;

    hash_for_each_possible_rcu(processes, current_process, table_node, current->tgid){
        if(current->tgid == current_process->tgid){
            hash_for_each_rcu(current_process->fibers, f_index, current_fiber, table_node){
                if(current_fiber->running_by == current->pid && test_bit(pos, current_fiber->fls_bitmap)){
                    current_fiber->fls[pos] = value;
                    return 1;
                }
            }
        }
    }
    return 0;
}


//FLS GET
long long flsGet(long pos){
    //void *value = NULL;
    process *current_process;
    fiber *current_fiber;
    int f_index;

    hash_for_each_possible_rcu(processes, current_process, table_node, current->tgid){
        if(current->tgid == current_process->tgid){
            hash_for_each_rcu(current_process->fibers, f_index, current_fiber, table_node){
                if(current_fiber->running_by == current->pid && test_bit(pos, current_fiber->fls_bitmap))
                    return current_fiber->fls[pos];
                else 
                    return (long long) NULL;
            }
        }
    }
    return (long long) NULL;
}

//FLS FREE
int flsFree(long pos){
    process *current_process;
    fiber *current_fiber;
    int f_index;

    hash_for_each_possible_rcu(processes, current_process, table_node, current->tgid){
        if(current->tgid == current_process->tgid){
            hash_for_each_rcu(current_process->fibers, f_index, current_fiber, table_node){
                //Need to check if the bit is set, otherwise it has no sense to free it
                if(current_fiber->running_by == current->pid && test_bit(pos, current_fiber->fls_bitmap)){
                    change_bit(pos, current_fiber->fls_bitmap);
                    return 1;
                }
            }
        }
    }

    return 0;
}