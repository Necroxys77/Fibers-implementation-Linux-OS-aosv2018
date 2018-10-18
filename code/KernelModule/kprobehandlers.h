#include <linux/kprobes.h> 
#include "fiber.h"
//#include <linux/proc_fs.h>
//#include <linux/proc_ns.h>

union proc_op {
	int (*proc_get_link)(struct dentry *, struct path *);
	int (*proc_show)(struct seq_file *m,
		struct pid_namespace *ns, struct pid *pid,
		struct task_struct *task);
};

struct proc_inode {
	struct pid *pid;
	unsigned int fd;
	union proc_op op;
	struct proc_dir_entry *pde;
	struct ctl_table_header *sysctl;
	struct ctl_table *sysctl_entry;
	const struct proc_ns_operations *ns_ops;
	struct inode vfs_inode;
};



//int pre_do_exit(struct kprobe *, struct pt_regs *);

int pre_schedule(struct kretprobe_instance *, struct pt_regs *);

void postHandler(struct kprobe *, struct pt_regs *, unsigned long);

void post_do_exit(struct kprobe *, struct pt_regs *, unsigned long);

int post_schedule(struct kretprobe_instance *, struct pt_regs *);

int register_kp(struct kprobe *);

void unregister_kp(struct kprobe *);

int register_kretp(struct kretprobe *);

void unregister_kretp(struct kretprobe *);

int register_jp(struct jprobe *);

void unregister_jp(struct jprobe *);