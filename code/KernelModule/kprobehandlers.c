#include "kprobehandlers.h"

DEFINE_PER_CPU(struct task_struct *, prev) = NULL;
int nents = 0;
spinlock_t nents_lock = __SPIN_LOCK_UNLOCKED(nents_lock);

typedef int (*proc_pident_readdir_t)(struct file *file, struct dir_context *ctx, 
                const struct pid_entry *ents, unsigned int nents);

typedef struct dentry *(*proc_pident_lookup_t) (struct inode *dir, struct dentry *dentry,
					const struct pid_entry *ents, unsigned int nents);

typedef int (*pid_getattr_t) (const struct path *, struct kstat *, u32, unsigned int);
typedef int (*proc_setattr_t) (struct dentry *dentry, struct iattr *attr);

int fiber_readdir(struct file *, struct dir_context *);

struct dentry *fiber_lookup(struct inode *dir, struct dentry *, unsigned int);

ssize_t fiber_read(struct file *file, char __user *buffer, size_t size, loff_t *ppos);

struct file_operations file_ops = {
				.read  = generic_read_dir,
    			.iterate_shared = fiber_readdir,
				.llseek  = generic_file_llseek,
};

struct inode_operations inode_ops = {
				.lookup = fiber_lookup,
};

struct file_operations fiber_ops = {
        read: fiber_read,
};

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
        current_fiber->exec_time += ((current_time.tv_nsec + current_time.tv_sec*1000000000) - current_fiber->start_time)/1000000;
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

int pre_proc_lookup(struct kretprobe_instance *ri, struct pt_regs *regs){

    struct kretprobe_data *proc_data;
    proc_data = (struct kretprobe_data *) kmalloc(sizeof(struct kretprobe_data), GFP_KERNEL);

    struct inode *dir = (struct inode *) regs->di;
    struct dentry *dentry = (struct dentry *) regs->si;
    //const struct pid_entry *ents = (struct pid_entry *) regs->dx;
    unsigned int flags = (unsigned int) regs->dx;

    //proc_data = (struct kretprobe_data *) ri->data;

    proc_data->dir = dir;
    proc_data->dentry = dentry;
    proc_data->flags = flags;
    memcpy(ri->data, proc_data, sizeof(struct kretprobe_data));

    return 0;

}

int pre_proc_readdir(struct kretprobe_instance *ri, struct pt_regs *regs){

    struct kretprobe_data *proc_data;
    proc_data = (struct kretprobe_data *) kmalloc(sizeof(struct kretprobe_data), GFP_KERNEL);

    struct file *file = (struct file *) regs->di;
    struct dir_context *ctx = (struct dir_context *) regs->si;
    /*const struct pid_entry *ents = (struct pid_entry *) regs->dx;
    unsigned int nents = (unsigned int) regs->cx;*/

    //proc_data = (struct kretprobe_data *) ri->data;

    proc_data->file = file;
    proc_data->ctx = ctx;
    memcpy(ri->data, proc_data, sizeof(struct kretprobe_data));
    /*proc_data->ents = ents;
    proc_data->nents = nents;*/

    //struct file *f = (struct file *)regs->di;
    //printk("name file: %s, size ents: %ld, nents: %u", file->f_path.dentry->d_name.name, sizeof(ents), nents);
    return 0;
}

ssize_t fiber_read(struct file *file, char __user *buffer, size_t size, loff_t *ppos){

    struct task_struct *task_pid;
    char fiber_stats[STATS_SIZE];
    unsigned long fiber_id;
    fiber *fib;
    size_t written_bytes, offset;

    if ((task_pid = get_proc_task(file->f_inode)) == NULL){
        //printk("fiber_lookup no\n");
        return 0;//real_lookup(dir, dentry, NULL, 0);
    }

    kstrtoul(file->f_path.dentry->d_name.name, 10, &fiber_id);
    if ((fib = get_fiber_by_id(task_pid->tgid, fiber_id)) == NULL)
        return 0;

    snprintf(fiber_stats, STATS_SIZE, "Running: %s\n"
                                    "Initial entry point: 0x%016lx\n"
                                    "Parent thread id: %d\n"
                                    "Number of activations: %d\n"
                                    "Number of failed activations: %d\n"
                                    "Total execution time: %lu ms\n", 
                                    ((fib->running_by == -1) ? "no" : "yes"), (unsigned long) fib->initial_entry_point, fib->parent_pid, fib->finalized_activations, fib->failed_activations, fib->exec_time);

    written_bytes = strnlen(fiber_stats, STATS_SIZE);
    if (*ppos >= written_bytes)
        return 0;

    offset = (size < written_bytes) ? size : written_bytes;
    if (copy_to_user(buffer, fiber_stats, offset))
        return -EFAULT;
    *ppos += offset;
    return offset;
}

struct dentry *fiber_lookup(struct inode *dir, struct dentry *dentry, unsigned int flags){
    
    struct task_struct *task_pid;
    struct pid_entry *ents;
    unsigned int nents_fiber_readdir;
    process *current_process;
    fiber *current_fiber;
    int f_index;
    struct dentry *ret;
    proc_pident_lookup_t real_lookup = (proc_pident_lookup_t) kallsyms_lookup_name("proc_pident_lookup");
    //printk("fiber_lookup\n");
    
    if ((task_pid = get_proc_task(dir)) == NULL || dir == NULL || dentry == NULL)
        return ERR_PTR(-ENOENT);

    if ((current_process = get_process_by_tgid(task_pid->tgid)) == NULL){
        //printk("fiber_lookup no\n");
        return 0;//real_lookup(dir, dentry, NULL, 0);
    }
    
    nents_fiber_readdir = (unsigned int) atomic_read(&(current_process->total_fibers));
    ents = (struct pid_entry *) kmalloc(nents_fiber_readdir * sizeof(struct pid_entry), GFP_KERNEL);
    memset(ents, 0, nents_fiber_readdir * sizeof(struct pid_entry));
    //printk("fiber_lookup\n");
    hash_for_each_rcu(current_process->fibers, f_index, current_fiber, table_node){
        if (current_fiber == NULL)
            break;
        //memset(name, 0, 8);
        //printk("cycle_lookup\n");
        //ents[current_fiber->fiber_id].len = sprintf(name, "%d", current_fiber->fiber_id);
        ents[current_fiber->fiber_id - 1].name = current_fiber->fiber_id_string;
        ents[current_fiber->fiber_id - 1].len = strlen(current_fiber->fiber_id_string);
        //memcpy((void *) ents[current_fiber->fiber_id].name, name, ents[current_fiber->fiber_id].len);
        ents[current_fiber->fiber_id - 1].mode = (S_IFREG | (S_IRUGO)); // XRUGO pls
        ents[current_fiber->fiber_id - 1].iop = NULL;
        ents[current_fiber->fiber_id - 1].fop = &fiber_ops;
    }
    ret = real_lookup(dir, dentry, ents, nents_fiber_readdir);
    kfree(ents);
    return ret;
}

int fiber_readdir(struct file *file, struct dir_context *ctx){

    struct task_struct *task_pid;
    struct pid_entry *ents;
    unsigned int nents_fiber_readdir;
    process *current_process;
    fiber *current_fiber;
    int f_index, ret;


    proc_pident_readdir_t real_readdir = (proc_pident_readdir_t) kallsyms_lookup_name("proc_pident_readdir");

    //printk("fiber_readdir sopra\n");

    if ((task_pid = get_proc_task(file_inode(file))) == NULL || file == NULL || ctx == NULL)
        return -ENOENT;

    if ((current_process = get_process_by_tgid(task_pid->tgid)) == NULL){
        //printk("fiber_readdir no\n");
        return 0;//real_readdir(file, ctx, NULL, 0);
    }


    nents_fiber_readdir = (unsigned int) atomic_read(&(current_process->total_fibers));
    ents = (struct pid_entry *) kmalloc(nents_fiber_readdir * sizeof(struct pid_entry), GFP_KERNEL);
    memset(ents, 0, nents_fiber_readdir * sizeof(struct pid_entry));
    hash_for_each_rcu(current_process->fibers, f_index, current_fiber, table_node){
        if (current_fiber == NULL)
            break;
        //memset(name, 0, 8);
        //printk("cycle_lookup\n");
        //ents[current_fiber->fiber_id].len = sprintf(name, "%d", current_fiber->fiber_id);
        ents[current_fiber->fiber_id - 1].name = current_fiber->fiber_id_string;
        ents[current_fiber->fiber_id - 1].len = strlen(current_fiber->fiber_id_string);
        //memcpy((void *) ents[current_fiber->fiber_id].name, name, ents[current_fiber->fiber_id].len);
        ents[current_fiber->fiber_id - 1].mode = (S_IFREG | (S_IRUGO)); // XRUGO pls
        ents[current_fiber->fiber_id - 1].iop = NULL;
        ents[current_fiber->fiber_id - 1].fop = &fiber_ops;
    }
    ret = real_readdir(file, ctx, ents, nents_fiber_readdir);
    kfree(ents);
    return ret;
}

/*rdi, rsi, rdx, r10 (rcx)*/
int post_proc_readdir(struct kretprobe_instance *ri, struct pt_regs *regs){
    
    struct kretprobe_data *proc_data;
    struct file *file;
    struct dir_context *ctx;
    struct pid_entry *fiber_dir;
    unsigned int nents_readdir;
    struct task_struct *task_pid;
   // struct inode_operations inode_ops;
    //struct file_operations file_ops;
    unsigned long flags;
/*
    inode_ops.lookup = fiber_lookup;
    inode_ops.getattr = (pid_getattr_t) kallsyms_lookup_name("pid_getattr");
	inode_ops.setattr = (proc_setattr_t) kallsyms_lookup_name("proc_setattr");

    file_ops.iterate_shared = fiber_readdir;
    file_ops.read = generic_read_dir;
    file_ops.llseek = generic_file_llseek;
*/
    proc_data = (struct kretprobe_data *) ri->data;
    file = proc_data->file;
    /*ents = proc_data->ents;
    nents = proc_data->nents;*/

    if (!nents) {
        spin_lock_irqsave(&nents_lock, flags);
        if (!nents)
            nents = proc_data->ctx->pos;//51
        spin_unlock_irqrestore(&nents_lock, flags);
    }

    if ((task_pid = get_proc_task(file_inode(file))) == NULL){
        //printk("NOOOOO readdir\n");
        return 0;//-ENOENT;
    }
    proc_pident_readdir_t real_readdir = (proc_pident_readdir_t) kallsyms_lookup_name("proc_pident_readdir");

    if (get_process_by_tgid(task_pid->tgid) == NULL){
        //printk("NOOOOO readdir 2\n");
        return 0;//real_readdir(file, ctx, ents, nents); //Retrieve return value of ciao and return it
    }

    fiber_dir = (struct pid_entry *) kmalloc(sizeof(struct pid_entry), GFP_KERNEL);
    fiber_dir->name = "fiber";
    fiber_dir->len = strlen("fiber");
    fiber_dir->mode = S_IFDIR | S_IRUGO | S_IXUGO;
    fiber_dir->iop = &inode_ops;
    fiber_dir->fop = &file_ops;
    nents_readdir = nents;
    //memcpy(my_tgid, ents, nents);
    //my_tgid[nents] = fiber_dir;
    real_readdir(file, proc_data->ctx, fiber_dir - (nents_readdir - 2), nents_readdir - 1); //Retrieve return value of ciao and return it
    //kfree(my_tgid);
    return 0;
}

int post_proc_lookup(struct kretprobe_instance *ri, struct pt_regs *regs){
    
    struct kretprobe_data *proc_data;
    struct inode *dir;
    struct dentry *dentry;
    struct pid_entry *fiber_dir;
    unsigned int nents_lookup;
    unsigned long flags;
    struct task_struct *task_pid;
    //struct inode_operations inode_ops;
    //struct file_operations file_ops;
/*
    inode_ops.lookup = fiber_lookup;
    inode_ops.getattr = (pid_getattr_t) kallsyms_lookup_name("pid_getattr");//kallsyms_lookup_name("pid_getattr");
	inode_ops.setattr = (proc_setattr_t) kallsyms_lookup_name("proc_setattr");

    file_ops.iterate_shared = fiber_readdir;
    file_ops.read = generic_read_dir;
    file_ops.llseek = generic_file_llseek;
*/
    proc_data = (struct kretprobe_data *) ri->data;
    dir = proc_data->dir;
    dentry = proc_data->dentry;
    //flags = proc_data->flags;

    if(!nents)
        return 0;

    if ((task_pid = get_proc_task(dir)) == NULL){
        //printk("NOOOOO\n");
        return 0;//-ENOENT;
    } 
    proc_pident_lookup_t real_lookup = (proc_pident_lookup_t) kallsyms_lookup_name("proc_pident_lookup");

    if (get_process_by_tgid(task_pid->tgid) == NULL){
        //printk("NOOOOO2\n");
        //real_lookup(dir, dentry, ents, nents); //Retrieve return value of ciao and return it
        return 0;
    }

    //my_tgid = (struct pid_entry *) kmalloc(sizeof(struct pid_entry) * (nents + 1), GFP_KERNEL);
    fiber_dir = (struct pid_entry *) kmalloc(sizeof(struct pid_entry), GFP_KERNEL);
    fiber_dir->name = "fiber";
    fiber_dir->len = strlen("fiber");
    fiber_dir->mode = S_IFDIR | S_IRUGO | S_IXUGO;
    fiber_dir->iop = &inode_ops;
    fiber_dir->fop = &file_ops;
    nents_lookup = nents;
    //memcpy(my_tgid, ents, nents);
    //my_tgid[nents] = fiber_dir;
    real_lookup(dir, dentry, fiber_dir - (nents_lookup - 2), nents_lookup-1); //Retrieve return value of ciao and return it
    //kfree(my_tgid);
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

int register_kretp_proc_lookup(struct kretprobe *kretp){
    kretp->handler = post_proc_lookup;
    kretp->entry_handler = pre_proc_lookup;
    kretp->data_size = sizeof(struct kretprobe_data);
    //kretp->kp.addr = (kprobe_opcode_t *) kallsyms_lookup_name("proc_tgid_base_lookup");
    kretp->kp.symbol_name = "proc_tgid_base_lookup";

    if (!register_kretprobe(kretp)) {
        printk(KERN_INFO "[+] Kretprobe proc_tgid_base_lookup() registered!\n");
		return 1;
	} else {
        printk(KERN_ALERT "[!] Registering Kretprobe proc_tgid_base_lookup() has failed!\n");
		return -2;
	}
    inode_ops.getattr = (pid_getattr_t) kallsyms_lookup_name("pid_getattr");
	inode_ops.setattr = (proc_setattr_t) kallsyms_lookup_name("proc_setattr");

}

int register_kretp_proc_readdir(struct kretprobe *kretp){
    kretp->handler = post_proc_readdir;
    kretp->entry_handler = pre_proc_readdir;
    kretp->data_size = sizeof(struct kretprobe_data);
    //kretp->kp.addr = (kprobe_opcode_t *) kallsyms_lookup_name("proc_tgid_base_readdir");
    kretp->kp.symbol_name = "proc_tgid_base_readdir";

    if (!register_kretprobe(kretp)) {
        printk(KERN_INFO "[+] Kretprobe proc_tgid_base_readdir() registered!\n");
		return 1;
	} else {
        printk(KERN_ALERT "[!] Registering Kretprobe proc_tgid_base_readdir() has failed!\n");
		return -2;
	}
    inode_ops.getattr = (pid_getattr_t) kallsyms_lookup_name("pid_getattr");
	inode_ops.setattr = (proc_setattr_t) kallsyms_lookup_name("proc_setattr");

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