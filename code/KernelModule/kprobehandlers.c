#include "kprobehandlers.h"

DEFINE_PER_CPU(struct task_struct *, prev) = NULL;

typedef int (*func)(struct file *file, struct dir_context *ctx, 
                const struct pid_entry *ents, unsigned int nents)/* = 
                (int (*))(struct file *file, struct dir_context *ctx, 
                            const struct pid_entry *ents, unsigned int nents) kallsyms_lookup_name("proc_pident_readdir")*/;
/*int x = func();*/

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

int pre_proc_readdir(struct kretprobe_instance *ri, struct pt_regs *regs){

    struct kretprobe_data *proc_data;
    //proc_data = (struct kretprobe_data *) kmalloc(sizeof(struct kretprobe_data *), GFP_KERNEL);

    struct file *file = (struct file *) regs->di;
    struct dir_context *ctx = (struct dir_context *) regs->si;
    const struct pid_entry *ents = (struct pid_entry *) regs->dx;
    unsigned int nents = (unsigned int) regs->cx;

    proc_data = (struct kretprobe_data *) ri->data;

    proc_data->file = file;
    proc_data->ctx = ctx;
    proc_data->ents = ents;
    proc_data->nents = nents;

    //struct file *f = (struct file *)regs->di;
    printk("name file: %s, size ents: %ld, nents: %u", file->f_path.dentry->d_name.name, sizeof(ents), nents);
    return 0;
}

/*rdi, rsi, rdx, r10 (rcx)*/
int post_proc_readdir(struct kretprobe_instance *ri, struct pt_regs *regs){
    struct kretprobe_data *proc_data;
    struct file *file;
    struct dir_context *ctx;
    const struct pid_entry *ents;
    struct pid_entry fiber_dir, *my_tgid;
    unsigned int nents;
    int ret;
    func ciao = (func) kallsyms_lookup_name("proc_pident_readdir");

    proc_data = (struct kretprobe_data *) ri->data;
    file = proc_data->file;
    ctx = proc_data->ctx;
    ents = proc_data->ents;
    nents = proc_data->nents;

    my_tgid = (struct pid_entry *) kmalloc(sizeof(struct pid_entry) * (nents + 1), GFP_KERNEL);
    fiber_dir.name = "fiber";
    fiber_dir.len = sizeof("fiber");
    fiber_dir.mode = S_IFDIR | S_IRUGO | S_IXUGO;
    fiber_dir.iop = NULL;
    fiber_dir.fop = NULL;

    memcpy(my_tgid, ents, nents);
    my_tgid[nents] = fiber_dir;
    ret = ciao(file, ctx, my_tgid, nents+1); //Retrieve return value of ciao and return it
    kfree(my_tgid);
    return ret;
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

int register_kretp_proc_readdir(struct kretprobe *kretp){
    kretp->handler = post_proc_readdir;
    kretp->entry_handler = pre_proc_readdir;
    kretp->kp.addr = (kprobe_opcode_t *) kallsyms_lookup_name("proc_pident_readdir");

    if (!register_kretprobe(kretp)) {
        printk(KERN_INFO "[+] Kretprobe proc_tgid_base_readdir() registered!\n");
		return 1;
	} else {
        printk(KERN_ALERT "[!] Registering Kretprobe proc_tgid_base_readdir() has failed!\n");
		return -2;
	}
}

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

void unregister_kp(struct kprobe *kp){

    printk(KERN_INFO "[+] Unregistering Kprobe for do_exit!\n");
    unregister_kprobe(kp);
}


void unregister_kretp(struct kretprobe *kp){

    printk(KERN_INFO "[+] Unregistering Kretprobe!\n");
    unregister_kretprobe(kp);
}