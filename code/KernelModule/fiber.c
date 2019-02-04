#include "fiber.h"
#include "macro.h"

DEFINE_HASHTABLE(Processes, 10);

spinlock_t process_lock = __SPIN_LOCK_UNLOCKED(process_lock);
unsigned long process_flags;


inline process_t *getProcessByTgid(pid_t tgid){
    process_t *current_process;
    hash_for_each_possible_rcu(Processes, current_process, table_node, tgid){
        if(current_process->tgid == tgid)
            return current_process;
    }
    return NULL;
}


inline thread_t *getThreadByPid(process_t* current_process, pid_t pid){
    thread_t *current_thread;
    hash_for_each_possible_rcu(current_process->Threads, current_thread, table_node, pid){
        if(current_thread->pid == pid)
            return current_thread;
    }
    return NULL;
}


inline fiber_t *getFiberById(process_t* current_process, int fiber_id){
    fiber_t *current_fiber;
    hash_for_each_possible_rcu(current_process->Fibers, current_fiber, table_node, fiber_id){
        if(current_fiber->fiber_id == fiber_id)
            return current_fiber;
    }
    return NULL;
}


//CONVERT
int convertThreadToFiber(void){
    struct timespec current_time;
    process_t *current_process, *new_process;
    thread_t *current_thread, *new_thread;
    fiber_t *new_fiber;

    //taking lock on process data structure in order to avoid simultaneous creation
    spin_lock_irqsave(&(process_lock), process_flags);
    current_process = getProcessByTgid(current->tgid);
    if(current_process == NULL){
        init_process(new_process, Processes);
        spin_unlock_irqrestore(&(process_lock), process_flags);

        init_thread(new_thread, new_process);
        init_fiber(new_fiber, new_process);
        new_fiber->activations = 1;
        new_fiber->running = 1;
        memset(&current_time, 0, sizeof(struct timespec));
        new_fiber->exec_time = 0;
        getnstimeofday(&current_time);
        new_fiber->start_time = current_time.tv_nsec + current_time.tv_sec*1000000000;
        new_thread->running_fiber = new_fiber;
    } else {
        spin_unlock_irqrestore(&(process_lock), process_flags);
        
        current_thread = getThreadByPid(current_process, current->pid);
        
        //if the current thread already issued a convert, error
        if(current_thread != NULL)
           return 0;

        //a new thread of a known process issued a convert
        init_thread(new_thread, current_process);
        init_fiber(new_fiber, current_process);
        new_fiber->activations = 1;
        new_fiber->running = 1;
        memset(&current_time, 0, sizeof(struct timespec));
        new_fiber->exec_time = 0;
        getnstimeofday(&current_time);
        new_fiber->start_time = current_time.tv_nsec + current_time.tv_sec*1000000000;
        new_thread->running_fiber = new_fiber;
    }
    return new_fiber->fiber_id;
}

//CREATE
int createFiber(unsigned long sp, entry_point_t user_function, void *args){
    process_t *current_process;
    thread_t *current_thread;
    fiber_t *new_fiber;

    current_process = getProcessByTgid(current->tgid);
    //no threads of current process ever issued a convert, process not found
    if(current_process == NULL)
        return 0;
    
    current_thread = getThreadByPid(current_process, current->pid);
    //current thread of current process never issued a convert, thead not found
    if(current_thread == NULL)
        return 0;
    
    init_fiber(new_fiber, current_process);
    new_fiber->context->sp = (unsigned long) (sp + STACK_SIZE - 8);
    new_fiber->context->bp = new_fiber->context->sp;
    new_fiber->context->ip = (unsigned long) user_function;
    new_fiber->context->di = (unsigned long) args;
    new_fiber->initial_entry_point = (void *) user_function;
    
    return new_fiber->fiber_id;
}

//SWITCH
int switchToFiber(int target_fiber_id){
    struct timespec current_time;
    unsigned long flags;
    process_t *current_process;
    thread_t *current_thread;
    fiber_t *calling_fiber, *target_fiber;
    struct pt_regs *old_context;

    current_process = getProcessByTgid(current->tgid);
    if(current_process == NULL)
        return 0;

    current_thread = getThreadByPid(current_process, current->pid);
    if(current_thread == NULL)
        return 0;

    target_fiber = getFiberById(current_process, target_fiber_id);
    if(target_fiber == NULL)
        return 0;

    calling_fiber = current_thread->running_fiber;
    if(calling_fiber == target_fiber)
        return 0;
    
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

        //copying registres from target fiber to current
        memcpy(old_context, target_fiber->context, sizeof(struct pt_regs));
        copy_kernel_to_fxregs(&(target_fiber->fpu_regs->state.fxsave));

        memset(&current_time, 0, sizeof(struct timespec));
        getnstimeofday(&current_time);
        target_fiber->start_time = (current_time.tv_nsec + current_time.tv_sec*1000000000);

        //For statistics
        target_fiber->activations += 1;

        //releasing calling fiber
        calling_fiber->running = 0;

        current_thread->running_fiber = target_fiber;
        spin_unlock_irqrestore(&(target_fiber->lock), flags);
        return 1;
    }
    target_fiber->failed_activations += 1;
    spin_unlock_irqrestore(&(target_fiber->lock), flags);

    return 0;
}

//FLSALLOC
long flsAlloc(void){
    long pos;
    process_t *current_process;
    thread_t *current_thread;
    fiber_t *current_fiber;

    current_process = getProcessByTgid(current->tgid);
    if(current_process == NULL)
        return -1;

    current_thread = getThreadByPid(current_process, current->pid);
    if(current_thread == NULL)
        return -1;
    
    current_fiber = current_thread->running_fiber;
    pos = find_first_zero_bit(current_fiber->fls_bitmap, MAX_FLS);
    if (pos == MAX_FLS)
        return -1;

    change_bit(pos, current_fiber->fls_bitmap);
    current_fiber->fls[pos] = 0;

    return pos;
}

//FLSSET
int flsSet(long pos, long long value){
    process_t *current_process;
    thread_t *current_thread;
    fiber_t *current_fiber;

    current_process = getProcessByTgid(current->tgid);
    if(current_process == NULL)
        return 0;
    
    current_thread = getThreadByPid(current_process, current->pid);
    if(current_thread == NULL)
        return 0;
    
    current_fiber = current_thread->running_fiber;
    if(test_bit(pos, current_fiber->fls_bitmap)){
        current_fiber->fls[pos] = value;
        return 1;
    }
    return 0;
}

//FLSGET
long long flsGet(long pos){
    process_t *current_process;
    thread_t *current_thread;
    fiber_t *current_fiber;

    current_process = getProcessByTgid(current->tgid);
    if(current_process == NULL)
        return 0;
    
    current_thread = getThreadByPid(current_process, current->pid);
    if(current_thread == NULL)
        return 0;

    current_fiber = current_thread->running_fiber;
    if(test_bit(pos, current_fiber->fls_bitmap))
        return current_fiber->fls[pos];

    return 0;
}

//FLSFREE
int flsFree(long pos){
    process_t *current_process;
    thread_t *current_thread;
    fiber_t *current_fiber;

    current_process = getProcessByTgid(current->tgid);
    if(current_process == NULL)
        return 0;
    
    current_thread = getThreadByPid(current_process, current->pid);
    if(current_thread == NULL)
        return 0;

    current_fiber = current_thread->running_fiber;
    if(test_bit(pos, current_fiber->fls_bitmap)){
        change_bit(pos, current_fiber->fls_bitmap);
        return 1;
    }
    return 0;
}

//REMOVE PROCESS
void removeProcess(process_t *current_process){
    thread_t *current_thread;
    fiber_t *current_fiber;
    int f_index, t_index;

    //deleting all fibers of that tgid
    hash_for_each_rcu(current_process->Fibers, f_index, current_fiber, table_node){
        kfree(current_fiber->context);
        kfree(current_fiber->fpu_regs);
        hash_del_rcu(&(current_fiber->table_node));
    }

    //deleting all threads of that tgid
    hash_for_each_rcu(current_process->Threads, t_index, current_thread, table_node){
        hash_del_rcu(&(current_thread->table_node));
    }

    //finally, remove process
    printk(KERN_INFO "[+] Remove: all data structures of process %d removed!\n",current_process->tgid);
    hash_del_rcu(&(current_process->table_node));
}

//UPDATE TIMER
void updateTimer(struct task_struct *prev, struct task_struct *next){
    struct timespec current_time;
    process_t *next_process, *prev_process;
    thread_t *next_thread, *prev_thread;
    fiber_t *next_fiber, *prev_fiber;

    //updating timer for prev
    prev_process = getProcessByTgid(prev->tgid);
    if(prev_process != NULL){
        prev_thread = getThreadByPid(prev_process, prev->pid);
        if(prev_thread != NULL){
            prev_fiber =  prev_thread->running_fiber;
            //updating the execution timer of the descheduled process
            if(prev_fiber != NULL){
                memset(&current_time, 0, sizeof(struct timespec));
                getnstimeofday(&current_time);
                prev_fiber->exec_time += ((current_time.tv_nsec + current_time.tv_sec*1000000000) - prev_fiber->start_time)/1000000;
            }
        }
    }
    //updating timer for next (that is current process)
    next_process = getProcessByTgid(next->tgid);
    if(next_process != NULL){
        next_thread = getThreadByPid(next_process, next->pid);
        if(next_thread != NULL){
            next_fiber = next_thread->running_fiber;
            //updating the start timer of the scheduled process
            if(next_fiber != NULL){
                memset(&current_time, 0, sizeof(struct timespec));
                getnstimeofday(&current_time);
                next_fiber->start_time = current_time.tv_nsec + current_time.tv_sec*1000000000; 
            }
        }
    }
}




