#include "kprobe.h"

DEFINE_PER_CPU(struct task_struct *, prev) = NULL;

int nents = 0;
spinlock_t nents_lock = __SPIN_LOCK_UNLOCKED(nents_lock);

struct file_operations file_ops = {
				.read  = generic_read_dir,
    			.iterate_shared = fiberReaddir,
				.llseek  = generic_file_llseek,
};

struct inode_operations inode_ops = {
				.lookup = fiberLookup,
};

struct file_operations fiber_ops = {
        read: fiberRead,
};


ssize_t fiberRead(struct file *file, char __user *buffer, size_t size, loff_t *ppos){
    char fiber_stats[STATS_SIZE];
    unsigned long fiber_id;
    size_t written_bytes, offset;
    struct task_struct *task_pid;
    process_t *my_process;
    fiber_t *my_fiber;

    task_pid = get_proc_task(file->f_inode);
    if (task_pid == NULL)
        return 0;

    if (kstrtoul(file->f_path.dentry->d_name.name, 10, &fiber_id))
        return 0;
    
    my_process = getProcessByTgid(task_pid->tgid);
    if(my_process == NULL)
        return 0;
    
    my_fiber = getFiberById(my_process, fiber_id);
    if (my_fiber == NULL)
        return 0;

    snprintf(fiber_stats, STATS_SIZE, "Running: %s\n"
                                    "Initial entry point: 0x%016lx\n"
                                    "Parent thread id: %d\n"
                                    "Number of activations: %d\n"
                                    "Number of failed activations: %d\n"
                                    "Total execution time: %lu ms\n", 
                                    ((my_fiber->running == 0) ? "no" : "yes"),
                                    (unsigned long) my_fiber->initial_entry_point,
                                    my_fiber->parent_pid, my_fiber->activations,
                                    my_fiber->failed_activations,
                                    my_fiber->exec_time);

    written_bytes = strnlen(fiber_stats, STATS_SIZE);
    if (*ppos >= written_bytes)
        return 0;

    offset = (size < written_bytes) ? size : written_bytes;
    if (copy_to_user(buffer, fiber_stats, offset))
        return -EFAULT;
    *ppos += offset;
    return offset;
}


struct dentry *fiberLookup(struct inode *dir, struct dentry *dentry, unsigned int flags){
    unsigned int nents_fiber_readdir;
    int f_index;
    struct task_struct *task_pid;
    struct pid_entry *ents;
    struct dentry *ret;
    process_t *current_process;
    fiber_t *current_fiber;
    
    proc_pident_lookup_t real_lookup = (proc_pident_lookup_t) kallsyms_lookup_name("proc_pident_lookup");
    
    task_pid = get_proc_task(dir);
    if (task_pid == NULL || dir == NULL || dentry == NULL)
        return ERR_PTR(-ENOENT);

    current_process = getProcessByTgid(task_pid->tgid);
    if (current_process == NULL)
        return 0;
    
    nents_fiber_readdir = (unsigned int) atomic_read(&(current_process->total_fibers));
    ents = (struct pid_entry *) kmalloc(nents_fiber_readdir * sizeof(struct pid_entry), GFP_KERNEL);
    memset(ents, 0, nents_fiber_readdir * sizeof(struct pid_entry));
    hash_for_each_rcu(current_process->Fibers, f_index, current_fiber, table_node){
        if (current_fiber == NULL)
            break;
        ents[current_fiber->fiber_id - 1].name = current_fiber->name;
        ents[current_fiber->fiber_id - 1].len = strlen(current_fiber->name);
        ents[current_fiber->fiber_id - 1].mode = (S_IFREG | (S_IRUGO));
        ents[current_fiber->fiber_id - 1].iop = NULL;
        ents[current_fiber->fiber_id - 1].fop = &fiber_ops;
    }
    ret = real_lookup(dir, dentry, ents, nents_fiber_readdir);
    kfree(ents);
    return ret;
}


int fiberReaddir(struct file *file, struct dir_context *ctx){
    unsigned int nents_fiber_readdir;
    int f_index, ret;
    struct task_struct *task_pid;
    struct pid_entry *ents;
    process_t *current_process;
    fiber_t *current_fiber;

    proc_pident_readdir_t real_readdir = (proc_pident_readdir_t) kallsyms_lookup_name("proc_pident_readdir");

    task_pid = get_proc_task(file_inode(file));
    if (task_pid == NULL || file == NULL || ctx == NULL)
        return -ENOENT;
    current_process = getProcessByTgid(task_pid->tgid);
    if (current_process == NULL)
        return 0;

    nents_fiber_readdir = (unsigned int) atomic_read(&(current_process->total_fibers));
    ents = (struct pid_entry *) kmalloc(nents_fiber_readdir * sizeof(struct pid_entry), GFP_KERNEL);
    memset(ents, 0, nents_fiber_readdir * sizeof(struct pid_entry));
    hash_for_each_rcu(current_process->Fibers, f_index, current_fiber, table_node){
        if (current_fiber == NULL)
            break;
        ents[current_fiber->fiber_id - 1].name = current_fiber->name;
        ents[current_fiber->fiber_id - 1].len = strlen(current_fiber->name);
        ents[current_fiber->fiber_id - 1].mode = (S_IFREG | (S_IRUGO));
        ents[current_fiber->fiber_id - 1].iop = NULL;
        ents[current_fiber->fiber_id - 1].fop = &fiber_ops;
    }
    ret = real_readdir(file, ctx, ents, nents_fiber_readdir);
    kfree(ents);

    return ret;
}


int register_kp_doExit(struct kprobe *kp){
    kp->addr = (kprobe_opcode_t *) do_exit; 
    kp->post_handler = postH_doExit;

    if (!register_kprobe(kp)) {
        printk(KERN_INFO "[+] Kprobe registered for do_exit!\n");
		return 1;
	} else {
        printk(KERN_ALERT "[!] Registering Kprobe for do_exit has failed!\n");
		return -2;
	}
}


void postH_doExit(struct kprobe *p, struct pt_regs *regs, unsigned long flags){
    struct timespec current_time;
    process_t *current_process;
    thread_t *current_thread;
    fiber_t *current_fiber;
    
    current_process = getProcessByTgid(current->tgid);
    if(current_process != NULL) {
        //if this is the last thread of that process to exit, then clean all data structures
        if(!atomic_dec_return(&(current_process->total_threads))){
            removeProcess(current_process);
            return;
        }
        current_thread = getThreadByPid(current_process, current->pid);
        if(current_thread != NULL){
            current_fiber = current_thread->running_fiber;
            if(current_fiber != NULL){//context?
                current_fiber->running = 0;
                memset(&current_time, 0, sizeof(struct timespec));
                getnstimeofday(&current_time);
                current_fiber->exec_time += ((current_time.tv_nsec + current_time.tv_sec*1000000000) - current_fiber->start_time)/1000000;
            }
        }
    }
}


void unregister_kp_doExit(struct kprobe *kp){
    printk(KERN_INFO "[+] Unregistering Kprobe for do_exit!\n");
    unregister_kprobe(kp);
}


int register_kretp_finishTaskSwitch(struct kretprobe *kretp){
    kretp->handler = postH_finishTaskSwitch;
    kretp->kp.symbol_name = "finish_task_switch";

    if (!register_kretprobe(kretp)) {
        printk(KERN_INFO "[+] Kretprobe registered for finish_task_switch!\n");
		return 1;
	} else {
        printk(KERN_ALERT "[!] Registering Kretprobe for finish_task_switch has failed!\n");
		return -2;
	}
}


int register_kretp_procReaddir(struct kretprobe *kretp){
    kretp->handler = postH_procReaddir;
    kretp->entry_handler = preH_procReaddir;
    kretp->data_size = sizeof(struct kretprobe_data);
    kretp->kp.symbol_name = "proc_tgid_base_readdir";

    if (!register_kretprobe(kretp)) {
        printk(KERN_INFO "[+] Kretprobe registered for proc_tgid_base_readdir!\n");
        inode_ops.getattr = (pid_getattr_t) kallsyms_lookup_name("pid_getattr");
        inode_ops.setattr = (proc_setattr_t) kallsyms_lookup_name("proc_setattr");
		return 1;
	} else {
        printk(KERN_ALERT "[!] Registering Kretprobe for proc_tgid_base_readdir has failed!\n");
		return -2;
	}
    
}

int register_kretp_procLookup(struct kretprobe *kretp){
    kretp->handler = postH_procLookup;
    kretp->entry_handler = preH_procLookup;
    kretp->data_size = sizeof(struct kretprobe_data);
    kretp->kp.symbol_name = "proc_tgid_base_lookup";

    if (!register_kretprobe(kretp)) {
        printk(KERN_INFO "[+] Kretprobe registered for proc_tgid_base_lookup!\n");          
        inode_ops.getattr = (pid_getattr_t) kallsyms_lookup_name("pid_getattr");
        inode_ops.setattr = (proc_setattr_t) kallsyms_lookup_name("proc_setattr");
        return 1;
	} else {
        printk(KERN_ALERT "[!] Registering Kretprobe for proc_tgid_base_lookup has failed!\n");
		return -2;
	}
    
}


int preH_procLookup(struct kretprobe_instance *ri, struct pt_regs *regs){
    unsigned int flags;
    struct kretprobe_data *proc_data;
    struct inode *dir = (struct inode *) regs->di;
    struct dentry *dentry = (struct dentry *) regs->si;
    
    flags = (unsigned int) regs->dx;
    proc_data = (struct kretprobe_data *) kmalloc(sizeof(struct kretprobe_data), GFP_KERNEL);
    proc_data->dir = dir;
    proc_data->dentry = dentry;
    proc_data->flags = flags;
    memcpy(ri->data, proc_data, sizeof(struct kretprobe_data));

    return 0;
}

int preH_procReaddir(struct kretprobe_instance *ri, struct pt_regs *regs){
    struct kretprobe_data *proc_data;
    struct file *file;
    struct dir_context *ctx;

    file = (struct file *) regs->di;
    ctx = (struct dir_context *) regs->si;
    proc_data = (struct kretprobe_data *) kmalloc(sizeof(struct kretprobe_data), GFP_KERNEL);
    proc_data->file = file;
    proc_data->ctx = ctx;
    memcpy(ri->data, proc_data, sizeof(struct kretprobe_data));
    return 0;
}


int postH_procReaddir(struct kretprobe_instance *ri, struct pt_regs *regs){
    proc_pident_readdir_t real_readdir;
    unsigned int nents_readdir;
    unsigned long flags;
    struct kretprobe_data *proc_data;
    struct file *file;
    //struct dir_context *ctx;
    struct pid_entry *fiber_dir;
    struct task_struct *task_pid;
    process_t *current_process;

    proc_data = (struct kretprobe_data *) ri->data;
    file = proc_data->file;
    
    if (!nents) {
        spin_lock_irqsave(&nents_lock, flags);
        if (!nents)
            nents = proc_data->ctx->pos;//51
        spin_unlock_irqrestore(&nents_lock, flags);
    }

    if ((task_pid = get_proc_task(file_inode(file))) == NULL)
        return 0;
    
    real_readdir = (proc_pident_readdir_t) kallsyms_lookup_name("proc_pident_readdir");

    current_process = getProcessByTgid(task_pid->tgid);
    if (current_process == NULL)
        return 0;

    fiber_dir = (struct pid_entry *) kmalloc(sizeof(struct pid_entry), GFP_KERNEL);
    fiber_dir->name = "fiber";
    fiber_dir->len = strlen("fiber");
    fiber_dir->mode = S_IFDIR | S_IRUGO | S_IXUGO;
    fiber_dir->iop = &inode_ops;
    fiber_dir->fop = &file_ops;
    nents_readdir = nents;
    real_readdir(file, proc_data->ctx, fiber_dir - (nents_readdir - 2), nents_readdir - 1);

    return 0;
}


int postH_procLookup(struct kretprobe_instance *ri, struct pt_regs *regs){
    proc_pident_lookup_t real_lookup;
    struct kretprobe_data *proc_data;
    struct inode *dir;
    struct dentry *dentry;
    struct pid_entry *fiber_dir;
    unsigned int nents_lookup;
    struct task_struct *task_pid;

    proc_data = (struct kretprobe_data *) ri->data;
    dir = proc_data->dir;
    dentry = proc_data->dentry;

    if(!nents)
        return 0;

    if ((task_pid = get_proc_task(dir)) == NULL)
        return 0;

    real_lookup = (proc_pident_lookup_t) kallsyms_lookup_name("proc_pident_lookup");

    if (getProcessByTgid(task_pid->tgid) == NULL)
        return 0;

    fiber_dir = (struct pid_entry *) kmalloc(sizeof(struct pid_entry), GFP_KERNEL);
    fiber_dir->name = "fiber";
    fiber_dir->len = strlen("fiber");
    fiber_dir->mode = S_IFDIR | S_IRUGO | S_IXUGO;
    fiber_dir->iop = &inode_ops;
    fiber_dir->fop = &file_ops;
    nents_lookup = nents;
    real_lookup(dir, dentry, fiber_dir - (nents_lookup - 2), nents_lookup-1);
    
    return 0;
}


int postH_finishTaskSwitch(struct kretprobe_instance *ri, struct pt_regs *regs){
    struct task_struct *prev_task = get_cpu_var(prev);

    if (prev_task == NULL){
        this_cpu_write(prev, current);
        put_cpu_var(prev);
        prev_task = get_cpu_var(prev);
        put_cpu_var(prev);
        return 0;
    }
    updateTimer(prev_task, current);
    this_cpu_write(prev, current);
    put_cpu_var(prev);

    return 0;
}


void unregister_kretp(struct kretprobe *kp){
    printk(KERN_INFO "[+] Unregistering Kretprobe!\n");
    unregister_kretprobe(kp);
}