#include "kprobehandlers.h"

DEFINE_PER_CPU(struct task_struct *, prev) = NULL;

/*We should not delete anything, just setting to -1 the 'running_by' field of the fiber 
    which was run by the exited thread*/
void post_do_exit(struct kprobe *p, struct pt_regs *regs, unsigned long flags){
    fiber *current_fiber;
    struct timespec current_time;
    int need_clean;

    current_fiber = get_running_fiber(&need_clean);
    if(need_clean == 1)
        remove_process(current->tgid);
    else if ((current_fiber != NULL)) {
        //printk(KERN_INFO "Thread %d with tgid %d exited, fiber %d runned by %d\n", current->pid, current->tgid, current_fiber->fiber_id, current_fiber->running_by);
        current_fiber->running_by = -1;
        memset(&current_time, 0, sizeof(struct timespec));
        getnstimeofday(&current_time);
        current_fiber->exec_time += (current_time.tv_nsec - current_fiber->start_time);
    }

    //printk("PostHandler do_exit: eseguito %d\n", current->pid);
}

/*We should not delete anything, just setting to -1 the 'running_by' field of the fiber 
    which was run by the exited thread*/
int post_schedule(struct kretprobe_instance *ri, struct pt_regs *regs){

    struct task_struct *prev_task = get_cpu_var(prev);

    if (prev_task == NULL){
        this_cpu_write(prev, current);
        put_cpu_var(prev);
        prev_task = get_cpu_var(prev);
        //printk(KERN_INFO "tgid current %d cpu %d, tgid prev_task %d cpu %d", current->pid, current->cpu, prev_task->pid, prev_task->cpu);
        put_cpu_var(prev);
        return 0;
    }

    //printk(KERN_INFO "tgid current %d cpu %d, tgid prev_task %d cpu %d", current->pid, current->cpu, prev_task->pid, prev_task->cpu);
    update_timer(prev_task, current);
    this_cpu_write(prev, current);
    put_cpu_var(prev);

    return 0;
}

int register_kp(struct kprobe *kp){
    
    kp->addr = (kprobe_opcode_t *) do_exit; 
    //kp->pre_handler = pre_do_exit; 
    kp->post_handler = post_do_exit;

    if (!register_kprobe(kp)) {
        printk(KERN_INFO "[+] Kprobe registered for do_exit!\n");
		return 1;
	} else {
        printk(KERN_ALERT "[!] Registering Kprobe for do_exit has failed!\n");
		return -2;
	}
}

/* kallsyms_lookup_name("proc_pident_lookup") */

int register_kretp(struct kretprobe *kretp){

    kretp->handler = post_schedule;
    kretp->kp.symbol_name = "finish_task_switch";

    if (!register_kretprobe(kretp)) {
        printk(KERN_INFO "[+] Kretprobe registered!\n");
		return 1;
	} else {
        printk(KERN_ALERT "[!] Registering Kretprobe has failed!\n");
		return -2;
	}
}

static int my_func(struct file *file, struct dir_context *ctx, const struct pid_entry *ents, unsigned int nents){

    //struct pid_entry *x;
    //x = (struct pid_entry *) kmalloc(sizeof(struct pide_entry *), GFP_KERNEL);
    //memcpy(x, ents, sizeof(ents));
    struct proc_inode *x = container_of(file->f_inode, struct proc_inode, vfs_inode);
    struct task_struct *task = get_pid_task(x->pid,  PIDTYPE_PID);
    //struct task_struct *task = get_proc_task(file_inode(file));
    //struct inode *i = file->f_inode;
    printk("Caiooo %d\n", task->pid);
    //printk("%s\n", x->name);
    return 1;
}

void unregister_kp(struct kprobe *kp){

    printk(KERN_INFO "[+] Unregistering Kprobe for do_exit!\n");
    unregister_kprobe(kp);
}


void unregister_kretp(struct kretprobe *kp){

    printk(KERN_INFO "[+] Unregistering Kretprobe!\n");
    unregister_kretprobe(kp);
}