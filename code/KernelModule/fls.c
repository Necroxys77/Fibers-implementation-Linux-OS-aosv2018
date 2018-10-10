/*
#include "fiber.h"

extern processes;

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
                        printk(KERN_ALERT "[!] FLS full for fiber %d\n", current_fiber->fiber_id);
                        return -1;
                    }
                    change_bit(pos, current_fiber->fls_bitmap);
                    return pos;
                }
            }
        }
    }
    return -1; //process/fiber not found
}

*/