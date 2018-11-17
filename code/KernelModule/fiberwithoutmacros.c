#include "fiber.h"

DEFINE_HASHTABLE(processes, 10);

spinlock_t process_lock = __SPIN_LOCK_UNLOCKED(process_lock);
unsigned long process_flags;


inline process *get_process_by_tgid(pid_t tgid){
    process *current_process;
    hash_for_each_possible_rcu(processes, current_process, table_node, tgid){
        if(current_process->tgid == tgid)
            return current_process;
    }
    return NULL;
}


inline thread *get_thread_by_pid(process* current_process, pid_t pid){
    thread *current_thread;
    hash_for_each_possible_rcu(current_process->threads, current_thread, table_node, pid){
        if(current_thread->pid == pid)
            return current_thread;
    }
    return NULL;
}


inline fiber *get_fiber_by_id(process* current_process, int fiber_id){
    fiber *current_fiber;
    hash_for_each_possible_rcu(current_process->fibers, current_fiber, table_node, fiber_id){
        if(current_fiber->fiber_id == fiber_id)
            return current_fiber;
    }
    return NULL;
}


//CONVERT
int convertThreadToFiber(void){
    fiber *new_fiber;
    process *current_process, *new_process;
    thread *current_thread, *new_thread;
    struct timespec current_time;

    //taking lock on process data structure in order to avoid simultaneous creation
    spin_lock_irqsave(&(process_lock), process_flags);
    current_process = get_process_by_tgid(current->tgid);
    if(current_process == NULL){
        //first thread within a process calls covert
        //printk(KERN_INFO "Convert: First process: tgid %d, pid %d\n", current->tgid, current->pid);
        //new process
        new_process = kzalloc(sizeof(process), GFP_KERNEL);
        new_process->tgid = current->tgid;
        atomic_set(&(new_process->total_fibers), 0);
        atomic_set(&(new_process->nThreads), 0);
        hash_init(new_process->fibers);
        hash_init(new_process->threads);
        hash_add_rcu(processes, &(new_process->table_node), new_process->tgid);
        spin_unlock_irqrestore(&(process_lock), process_flags);

        //new thread
        new_thread = kzalloc(sizeof(thread), GFP_KERNEL);
        new_thread->pid = current->pid;
        atomic_inc(&(new_process->nThreads));

        
        //new fiber
        new_fiber = kzalloc(sizeof(fiber), GFP_KERNEL);
        new_fiber->context = kzalloc(sizeof(struct pt_regs), GFP_KERNEL);
        new_fiber->fpu_regs = kzalloc(sizeof(struct fpu), GFP_KERNEL);
        bitmap_zero(new_fiber->fls_bitmap, MAX_SIZE_FLS);
        memset(new_fiber->fls, 0, sizeof(long long)*MAX_SIZE_FLS);
        spin_lock_init(&(new_fiber->lock));
        memcpy(new_fiber->context, task_pt_regs(current), sizeof(struct pt_regs));
        copy_fxregs_to_kernel(new_fiber->fpu_regs);
        new_fiber->fiber_id = atomic_inc_return(&(new_process->total_fibers));
        snprintf(new_fiber->fiber_id_string, 256, "%d", new_fiber->fiber_id);
        new_fiber->parent_pid = current->pid;
        new_fiber->finalized_activations = 1;
        new_fiber->failed_activations = 0;
        new_fiber->running = 1;
        new_fiber->initial_entry_point = (void *) task_pt_regs(current)->ip;
        memset(&current_time, 0, sizeof(struct timespec));
        new_fiber->exec_time = 0;
        getnstimeofday(&current_time);
        new_fiber->start_time = current_time.tv_nsec + current_time.tv_sec*1000000000;
        //printk(KERN_INFO "New id %d, new_fiber->exec_time = %ld, new_fiber->start_time = %ld", new_fiber->fiber_id, new_fiber->exec_time, new_fiber->start_time);

        hash_add_rcu(new_process->fibers, &(new_fiber->table_node), new_fiber->fiber_id);

        new_thread->runningFiber = new_fiber;
        hash_add_rcu(new_process->threads, &(new_thread->table_node), new_thread->pid);

    } else {
        spin_unlock_irqrestore(&(process_lock), process_flags);
        current_thread = get_thread_by_pid(current_process, current->pid);
        //if the current thread already issued a convert, error
        if(current_thread != NULL){
           printk(KERN_ALERT "Convert: Process present but thread already issued a convert! tgid: %d, pid: %d\n", current->tgid, current->pid);
           return 0; //error
        }
        //a new thread of a known process issued a convert
        
        //new thread
        new_thread = kzalloc(sizeof(thread), GFP_KERNEL);
        new_thread->pid = current->pid;
        atomic_inc(&(current_process->nThreads));

        //new fiber
        new_fiber = kzalloc(sizeof(fiber), GFP_KERNEL);
        new_fiber->context = kzalloc(sizeof(struct pt_regs), GFP_KERNEL);
        new_fiber->fpu_regs = kzalloc(sizeof(struct fpu), GFP_KERNEL);
        bitmap_zero(new_fiber->fls_bitmap, MAX_SIZE_FLS);
        memset(new_fiber->fls, 0, sizeof(long long)*MAX_SIZE_FLS);
        spin_lock_init(&(new_fiber->lock));
        memcpy(new_fiber->context, task_pt_regs(current), sizeof(struct pt_regs));
        copy_fxregs_to_kernel(new_fiber->fpu_regs);
        new_fiber->fiber_id = atomic_inc_return(&(current_process->total_fibers));
        snprintf(new_fiber->fiber_id_string, 256, "%d", new_fiber->fiber_id);
        new_fiber->parent_pid = current->pid;
        new_fiber->finalized_activations = 1;
        new_fiber->failed_activations = 0;
        new_fiber->running = 1;
        new_fiber->initial_entry_point = (void *) task_pt_regs(current)->ip;
        memset(&current_time, 0, sizeof(struct timespec));
        new_fiber->exec_time = 0;
        getnstimeofday(&current_time);
        new_fiber->start_time = current_time.tv_nsec + current_time.tv_sec*1000000000;
        //printk(KERN_INFO "New id %d, new_fiber->exec_time = %ld, new_fiber->start_time = %ld", new_fiber->fiber_id, new_fiber->exec_time, new_fiber->start_time);

        hash_add_rcu(current_process->fibers, &(new_fiber->table_node), new_fiber->fiber_id);
        
        new_thread->runningFiber = new_fiber;
        hash_add_rcu(current_process->threads, &(new_thread->table_node), new_thread->pid);
    }

    //printk(KERN_INFO "Convert: New fiber %d inserted\n", new_fiber->fiber_id);
    return new_fiber->fiber_id;
}


//CREATE
int createFiber(unsigned long sp, entry_point user_function, void *args){
    fiber *new_fiber;
    process *current_process;
    thread *current_thread;

    current_process = get_process_by_tgid(current->tgid);
    if(current_process == NULL){
        //printk(KERN_ALERT "Create: The process never issued a convert! tgid: %d\n", current->tgid);
        return 0;
    }
    
    current_thread = get_thread_by_pid(current_process, current->pid);
    if(current_thread == NULL){
        //printk(KERN_ALERT "Create: The thread never issued a convert! tgid: %d, pid: %d\n", current->tgid, current->pid);
        return 0;
    }
    
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
    snprintf(new_fiber->fiber_id_string, 256, "%d", new_fiber->fiber_id);
    new_fiber->parent_pid = current->pid;
    new_fiber->finalized_activations = 0;
    new_fiber->failed_activations = 0;
    new_fiber->running = 0;
    new_fiber->initial_entry_point = (void *) user_function;
    new_fiber->exec_time = 0;
    new_fiber->start_time = 0;
    //printk(KERN_INFO "New id %d, new_fiber->exec_time = %ld, new_fiber->start_time = %ld", new_fiber->fiber_id, new_fiber->exec_time, new_fiber->start_time);

    hash_add_rcu(current_process->fibers, &(new_fiber->table_node), new_fiber->fiber_id);
    //printk(KERN_INFO "[-] New fiber created [id %d, tgid %d, parent_pid %d]\n", new_fiber->fiber_id, current_process->tgid, new_fiber->parent_pid);

    //printk(KERN_INFO "Create: New fiber %d inserted\n", new_fiber->fiber_id);
    return new_fiber->fiber_id;
}

//SWITCH
int switchToFiber(int target_fiber_id){
    fiber *calling_fiber, *target_fiber;
    process *current_process;
    thread *current_thread;
    struct pt_regs *old_context;
    unsigned long flags;
    struct timespec current_time;

    current_process = get_process_by_tgid(current->tgid);
    if(current_process == NULL){
        //printk(KERN_ALERT "Switch: Process never issued a convert! tgid: %d\n", current->tgid);
        return 0;
    }
    current_thread = get_thread_by_pid(current_process, current->pid);
    if(current_thread == NULL){
        //printk(KERN_ALERT "Switch: Thread never issued a convert! tgid: %d, pid: %d\n",current->tgid,current->pid);
        return 0;
    }

    target_fiber = get_fiber_by_id(current_process, target_fiber_id);
    if(target_fiber == NULL){
        //printk(KERN_ALERT "Switch: Target fiber doesn't exists! id: %d\n",target_fiber_id);
        return 0;
    }    
    calling_fiber = current_thread->runningFiber;
    if(calling_fiber == target_fiber){
        //printk(KERN_ALERT "Switch: Calling fiber %d and target fiber %d are the same! Can't switch\n", calling_fiber->fiber_id, target_fiber->fiber_id);
        return 0;
    }
    
    //acquiring lock...
    spin_lock_irqsave(&(target_fiber->lock), flags);
    if(target_fiber->running == 0){
        target_fiber->running = 1;
        old_context = task_pt_regs(current);

        //copying registers from current to the calling fiber
        memcpy(calling_fiber->context, old_context, sizeof(struct pt_regs));
        copy_fxregs_to_kernel(calling_fiber->fpu_regs);

        /*For exec time*/
        memset(&current_time, 0, sizeof(struct timespec));
        getnstimeofday(&current_time);
        calling_fiber->exec_time += ((current_time.tv_nsec + current_time.tv_sec*1000000000) - calling_fiber->start_time)/1000000;

        //printk(KERN_INFO "Calling id = %d, calling_fiber->exec_time = %ld, calling_fiber->start_time = %ld", calling_fiber->fiber_id, calling_fiber->exec_time, calling_fiber->start_time);

        //copying registres from target fiber to current
        memcpy(old_context, target_fiber->context, sizeof(struct pt_regs));
        copy_kernel_to_fxregs(&(target_fiber->fpu_regs->state.fxsave));

        memset(&current_time, 0, sizeof(struct timespec));
        getnstimeofday(&current_time);
        target_fiber->start_time = (current_time.tv_nsec + current_time.tv_sec*1000000000);
        //printk(KERN_INFO "Target id = %d, target_fiber->exec_time = %ld, target_fiber->start_time = %ld", target_fiber->fiber_id, target_fiber->exec_time, target_fiber->start_time);

        //For statistics
        target_fiber->finalized_activations += 1;

        //realising calling fiber
        calling_fiber->running = 0;

        current_thread->runningFiber = target_fiber;
        spin_unlock_irqrestore(&(target_fiber->lock), flags);
        return 1;
    }
    //printk(KERN_INFO "[-] Target fiber is already running by %d\n", target_fiber->running_by);
    target_fiber->failed_activations += 1;
    spin_unlock_irqrestore(&(target_fiber->lock), flags);
    return 0;
}


//FLSALLOC
long flsAlloc(void){
    process *current_process;
    thread *current_thread;
    fiber *current_fiber;
    long pos; //bit

    current_process = get_process_by_tgid(current->tgid);
    if(current_process == NULL){
        //printk(KERN_ALERT "flsAlloc: process %d not found\n",current->tgid);
        return -1;
    }

    current_thread = get_thread_by_pid(current_process, current->pid);
    if(current_thread == NULL){
        //printk(KERN_ALERT "flsAlloc: thread %d not found, tgid: %d\n",current->pid, current->tgid);
        return -1;
    }
    
    current_fiber = current_thread->runningFiber;
    pos = find_first_zero_bit(current_fiber->fls_bitmap, MAX_SIZE_FLS);
    if (pos == MAX_SIZE_FLS){
        //printk(KERN_ALERT "[!] fls full for fiber %d\n", current_fiber->fiber_id);
        return -1;
    }
    change_bit(pos, current_fiber->fls_bitmap);
    current_fiber->fls[pos] = 0; // to clean
    //printk(KERN_INFO "flsAlloc: new pos %ld to be filled\n",pos);
    return pos;
}


int flsSet(long pos, long long value){
    process *current_process;
    thread *current_thread;
    fiber *current_fiber;

    current_process = get_process_by_tgid(current->tgid);
    if(current_process == NULL){
        //printk(KERN_ALERT "flsSet: process %d not found\n",current->tgid);
        return 0;
    }
    
    current_thread = get_thread_by_pid(current_process, current->pid);
    if(current_thread == NULL){
        //printk(KERN_ALERT "flsSet: thread %d not found, process %d\n",current->pid, current->tgid);
        return 0;
    }
    
    current_fiber = current_thread->runningFiber;
    if(test_bit(pos, current_fiber->fls_bitmap)){
        current_fiber->fls[pos] = value;
        //printk(KERN_INFO "flsSet: value %lld correctly inserted at position %ld\n",value, pos);
        //printk("Value %lld set at pos %ld of fiber %d\n", current_fiber->fls[pos], pos, current_fiber->fiber_id);
        return 1;
    }
    //printk(KERN_ALERT "flsSet: val %lld already present at position %ld\n",value,pos);
    return 0;
}



//FLSGET
long long flsGet(long pos){
    process *current_process;
    thread *current_thread;
    fiber *current_fiber;

    current_process = get_process_by_tgid(current->tgid);
    if(current_process == NULL){
        //printk(KERN_ALERT "flsGet: process %d not found\n",current->tgid);
        return 0;
    }
    
    current_thread = get_thread_by_pid(current_process, current->pid);
    if(current_thread == NULL){
        //printk(KERN_ALERT "flsGet: process %d not found, tgid: %d\n",current->pid ,current->tgid);
        return 0;
    }
    current_fiber = current_thread->runningFiber;
    if(test_bit(pos, current_fiber->fls_bitmap)){
        //printk(KERN_INFO "flsGet: taken value %lld from position %ld\n", current_fiber->fls[pos] ,pos);
        return current_fiber->fls[pos];
    }
    //printk(KERN_ALERT "flsGet: empty val at position %ld\n",pos);
    return 0;
}

//FLSFREE
int flsFree(long pos){
    process *current_process;
    fiber *current_fiber;
    thread *current_thread;

    current_process = get_process_by_tgid(current->tgid);
    if(current_process == NULL){
        //printk(KERN_ALERT "flsFree: process %d not found\n",current->tgid);
        return 0;
    }
    
    current_thread = get_thread_by_pid(current_process, current->pid);
    if(current_thread == NULL){
        //printk(KERN_ALERT "flsFree: process %d not found, tgid: %d\n",current->pid, current->tgid);
        return 0;
    }

    current_fiber = current_thread->runningFiber;
    if(test_bit(pos, current_fiber->fls_bitmap)){
        //printk(KERN_INFO "flsFree: correctly freed position %ld\n",pos);
        change_bit(pos, current_fiber->fls_bitmap);
        return 1;
    }
    //printk(KERN_ALERT "flsFree: nothing to free at pos %ld\n",pos);

    return 0;
}

//REMOVE PROCESS
void remove_process(process *current_process){
    thread *current_thread;
    fiber *current_fiber;
    int f_index, t_index;

    //printk(KERN_INFO "Remove: Removing process %d\n", current_process->tgid);
    //deleting all fibers of that tgid
    hash_for_each_rcu(current_process->fibers, f_index, current_fiber, table_node){
        kfree(current_fiber->context);
        kfree(current_fiber->fpu_regs);
        //printk(KERN_INFO "Remove: Fiber %d removed correctly\n", current_fiber->fiber_id);
        hash_del_rcu(&(current_fiber->table_node));
    }

    //deleting all threads of that tgid
    hash_for_each_rcu(current_process->threads, t_index, current_thread, table_node){
        //printk(KERN_INFO "Remove: Thread %d removed correctly\n", current_thread->pid);
        hash_del_rcu(&(current_thread->table_node));
    }

    //finally, remove process
    printk(KERN_INFO "Remove: all data structures of process %d removed!\n",current_process->tgid);
    hash_del_rcu(&(current_process->table_node));
}

//UPDATE TIMER
void update_timer(struct task_struct *prev, struct task_struct *next){
    process *next_process, *prev_process;
    fiber *next_fiber, *prev_fiber;
    thread *next_thread, *prev_thread;
    struct timespec current_time;

    //updating timer for prev
    prev_process = get_process_by_tgid(prev->tgid);
    if(prev_process != NULL){
        prev_thread = get_thread_by_pid(prev_process, prev->pid);
        if(prev_thread != NULL){
            prev_fiber =  prev_thread->runningFiber;
            //updating the execution timer of the descheduled process
            if(prev_fiber != NULL){
                memset(&current_time, 0, sizeof(struct timespec));
                getnstimeofday(&current_time);
                //printk(KERN_INFO "Update_timer (prev): Updating exec time %lu of fiber %d\n", prev_fiber->exec_time, prev_fiber->fiber_id);
                prev_fiber->exec_time += ((current_time.tv_nsec + current_time.tv_sec*1000000000) - prev_fiber->start_time)/1000000;
                //printk(KERN_INFO "Update_timer (prev): complete! new exec time %lu\n", prev_fiber->exec_time);
            }
        }
    }

    //updating timer for next (that is current process)
    next_process = get_process_by_tgid(next->tgid);
    if(next_process != NULL){
        next_thread = get_thread_by_pid(next_process, next->pid);
        if(next_thread != NULL){
            next_fiber = next_thread->runningFiber;
            //updating the start timer of the scheduled process
            if(next_fiber != NULL){
                memset(&current_time, 0, sizeof(struct timespec));
                getnstimeofday(&current_time);
                //printk(KERN_INFO "Update_timer (next): Updating start time %lu of fiber %d\n", next_fiber->start_time, next_fiber->fiber_id);
                next_fiber->start_time = current_time.tv_nsec + current_time.tv_sec*1000000000; 
                //printk(KERN_INFO "Update_timer (next): complete! new start time %lu\n", next_fiber->start_time);
            }
        }
    }
}




