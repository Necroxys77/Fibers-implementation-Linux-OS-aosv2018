#include "kprobehandlers.h"

int counter = 0;

int preHandler(struct kprobe *p, struct pt_regs *regs){
    printk("PreHandler: counter = %d, pid %d\n",counter++, current->pid);
    return 0; 
}

/*We should not delete anything, just setting to -1 the 'running_by' field of the fiber 
    which was run by the exited thread*/
void postHandler(struct kprobe *p, struct pt_regs *regs, unsigned long flags){

    process *current_process;
    fiber *current_fiber;
    int f_index;

    hash_for_each_possible_rcu(processes, current_process, table_node, current->tgid){
        if(current_process->tgid == current->tgid){
            hash_for_each_rcu(current_process->fibers, f_index, current_fiber, table_node){
                if(current_fiber->running_by == current->pid){
                    printk(KERN_INFO "Thread %d with tgid %d exited, fiber %d runned by %d\n", current->pid, current->tgid, current_fiber->fiber_id, current_fiber->running_by);
                    current_fiber->running_by = -1;
                }
            }
        }
    }

    printk("PostHandler: eseguito\n");
}

int register_doexit_probe(struct kprobe *kp){

    kp->pre_handler = preHandler; 
    kp->post_handler = postHandler; 
    kp->addr = (kprobe_opcode_t *) do_exit; 

    if (!register_kprobe(kp)) {
        printk(KERN_INFO "[+] Kprobe registered for do_exit!\n");
		return 1;
	} else {
        printk(KERN_ALERT "[!] Registering Kprobe for do_exit has failed\n");
		return -2;
	}
}

void unregister_doexit_probe(struct kprobe *kp){

    printk(KERN_INFO "[+] Unregistering Kprobe for do_exit!\n");
    unregister_kprobe(kp);
}