#include "kprobehandlers.h"

int counter = 0;

int pre_do_exit(struct kprobe *p, struct pt_regs *regs){
    printk("PreHandler: counter = %d, pid %d\n",counter++, current->pid);
    return 0; 
}

/*We should not delete anything, just setting to -1 the 'running_by' field of the fiber 
    which was run by the exited thread*/
void post_do_exit(struct kprobe *p, struct pt_regs *regs, unsigned long flags){

    /*process *current_process;
    fiber *current_fiber;
    int f_index;

    hash_for_each_possible_rcu(processes, current_process, table_node, 10){
        if(current_process->tgid == current->tgid){
            hash_for_each_rcu(current_process->fibers, f_index, current_fiber, table_node){
                if(current_fiber->running_by == current->pid){
                    printk(KERN_INFO "Thread %d with tgid %d exited, fiber %d runned by %d\n", current->pid, current->tgid, current_fiber->fiber_id, current_fiber->running_by);
                    current_fiber->running_by = -1;
                }
            }
        }
    }

    printk("PostHandler: eseguito\n");*/
}

struct my_data {
    pid_t p;
};

int pre_schedule(struct kretprobe_instance *ri, struct pt_regs *regs){

    struct my_data *p;
    //ri->data = p;
    p = (struct my_data *) ri->data;
    p->p = current->pid;
    //ri->data[0] = (char *) p;

    //printk("PreHandler schedule(): pid %d\n", current->pid);
    return 0; 
}

/*We should not delete anything, just setting to -1 the 'running_by' field of the fiber 
    which was run by the exited thread*/
int post_schedule(struct kretprobe_instance *ri, struct pt_regs *regs){

    struct my_data *r;
    r = (struct my_data *) ri->data;
    
    if(current->pid != r->p){
        printk("SI\n");
    }

    //printk("PostHandler schedule(): pid %d, %d\n", r->p, current->pid);
    return 0;
}

int register_kp(struct kprobe *kp, char function){
    
    if (function == 'e'){
        kp->addr = (kprobe_opcode_t *) do_exit; 
        kp->pre_handler = pre_do_exit; 
        kp->post_handler = post_do_exit;
    }
    /*if (function == 's'){
        kp->addr = (kprobe_opcode_t *) schedule; 
        kp->pre_handler = pre_schedule; 
        kp->post_handler = post_schedule;
    }*/

    if (!register_kprobe(kp)) {
        printk(KERN_INFO "[+] Kprobe registered for %c!\n", function);
		return 1;
	} else {
        printk(KERN_ALERT "[!] Registering Kprobe for %c has failed!\n", function);
		return -2;
	}
}

int register_kretp(struct kretprobe *kretp){

    kretp->handler = post_schedule;
    kretp->entry_handler = pre_schedule;
    kretp->kp.symbol_name = "__schedule";
    kretp->data_size = sizeof(struct my_data *);
    kretp->maxactive = 20;

    if (!register_kretprobe(kretp)) {
        printk(KERN_INFO "[+] Kretprobe registered!\n");
		return 1;
	} else {
        printk(KERN_ALERT "[!] Registering Kretprobe has failed!\n");
		return -2;
	}
}

void unregister_kp(struct kprobe *kp){

    printk(KERN_INFO "[+] Unregistering Kprobe for do_exit!\n");
    unregister_kprobe(kp);
}

void unregister_kretp(struct kretprobe *kp){

    printk(KERN_INFO "[+] Unregistering Kretprobe!\n");
    unregister_kretprobe(kp);
}