#include "kprobehandlers.h"

int counterp = 0;
int counterpo = 0;

/*
int pre_do_exit(struct kprobe *p, struct pt_regs *regs){
    printk("PreHandler: counter = %d, pid %d\n",counter++, current->pid);
    return 0; 
}
*/


/*We should not delete anything, just setting to -1 the 'running_by' field of the fiber 
    which was run by the exited thread*/
void post_do_exit(struct kprobe *p, struct pt_regs *regs, unsigned long flags){
    fiber *current_fiber;

    current_fiber = get_running_fiber();
    if (current_fiber != NULL){
        printk(KERN_INFO "Thread %d with tgid %d exited, fiber %d runned by %d\n", current->pid, current->tgid, current_fiber->fiber_id, current_fiber->running_by);
        current_fiber->running_by = -1;
    }
    printk("PostHandler do_exit: eseguito\n");
}

struct my_data {
    pid_t p;
    unsigned int cpu;
};

int pre_schedule(struct kretprobe_instance *ri, struct pt_regs *regs){
    struct my_data *dat;
    //ri->data = dat;
    dat = (struct my_data *) ri->data;
    //ri->data[0] = (char *) dat;
    
    if (counterp==0){
        printk(KERN_INFO "PRE: %d\n", current->pid);
        counterp++;
        dat->p = current->pid;
        dat->cpu = current->cpu;
    } else{
        dat->p = 0;
        dat->cpu = 99;
    }
    //struct my_data *p;
    //ri->data = p;
    //p = (struct my_data *) ri->data;
    //p->p = current->pid;
    //ri->data[0] = (char *) p;

    //printk("PreHandler schedule(): pid %d\n", current->pid);
    return 0; 
}

/*We should not delete anything, just setting to -1 the 'running_by' field of the fiber 
    which was run by the exited thread*/
int post_schedule(struct kretprobe_instance *ri, struct pt_regs *regs){

    struct my_data *r;
    r = (struct my_data *) ri->data;
    
    if (counterpo==0  && (r->cpu == current->cpu)){
        printk(KERN_INFO "POST: %d, %d, %ud\n", current->pid, r->p, r->cpu);
        counterpo++;
    }
    //printk("PostHandler schedule(): pid %d, %d\n", r->p, current->pid);
    return 0;
}



int register_kp(struct kprobe *kp, char function){
    
    if (function == 'e'){
        kp->addr = (kprobe_opcode_t *) do_exit; 
        //kp->pre_handler = pre_do_exit; 
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
    //kretp->maxactive = 20;

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