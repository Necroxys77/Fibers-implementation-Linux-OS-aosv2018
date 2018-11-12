#include <linux/kprobes.h> 
#include "fiber.h"
#include <linux/proc_fs.h>

#define STATS_SIZE 1024

union proc_op {
	int (*proc_get_link)(struct dentry *, struct path *);
	int (*proc_show)(struct seq_file *m,
		struct pid_namespace *ns, struct pid *pid,
		struct task_struct *task);
};

struct pid_entry {
	const char *name;
	unsigned int len;
	umode_t mode;
	const struct inode_operations *iop;
	const struct file_operations *fop;
	union proc_op op;
};

struct kretprobe_data {
	struct file *file;
    struct dir_context *ctx;
    unsigned int flags;
	struct inode *dir;
	struct dentry* dentry;
};

struct proc_inode {
	struct pid *pid;
	unsigned int fd;
	union proc_op op;
	struct proc_dir_entry *pde;
	struct ctl_table_header *sysctl;
	struct ctl_table *sysctl_entry;
	struct hlist_node sysctl_inodes;
	const struct proc_ns_operations *ns_ops;
	struct inode vfs_inode;
};//__randomize_layout

static inline struct proc_inode *PROC_I(const struct inode *inode) {
	return container_of(inode, struct proc_inode, vfs_inode);
}

static inline struct pid *proc_pid(struct inode *inode) {
	return PROC_I(inode)->pid;
}

static inline struct task_struct *get_proc_task(struct inode *inode) {
	return get_pid_task(proc_pid(inode), PIDTYPE_PID);
}


//int pre_do_exit(struct kprobe *, struct pt_regs *);

int pre_schedule(struct kretprobe_instance *, struct pt_regs *);

void postHandler(struct kprobe *, struct pt_regs *, unsigned long);

void post_do_exit(struct kprobe *, struct pt_regs *, unsigned long);

int post_schedule(struct kretprobe_instance *, struct pt_regs *);

int register_kp(struct kprobe *);

void unregister_kp(struct kprobe *);

int register_kretp(struct kretprobe *);

int register_kretp_proc_readdir(struct kretprobe *);

int register_kretp_proc_lookup(struct kretprobe *);

void unregister_kretp(struct kretprobe *);

int register_jp(struct jprobe *);

void unregister_jp(struct jprobe *);

