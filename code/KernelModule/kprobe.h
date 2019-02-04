#include <linux/kprobes.h> 
#include <linux/proc_fs.h>
#include "fiber.h"

#define STATS_SIZE 512

union proc_op {
	int (*proc_get_link)(struct dentry *, struct path *);
	int (*proc_show)(struct seq_file *m, struct pid_namespace *ns, struct pid *pid, struct task_struct *task);
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
};

static inline struct proc_inode *PROC_I(const struct inode *inode) {
	return container_of(inode, struct proc_inode, vfs_inode);
}

static inline struct pid *proc_pid(struct inode *inode) {
	return PROC_I(inode)->pid;
}

static inline struct task_struct *get_proc_task(struct inode *inode) {
	return get_pid_task(proc_pid(inode), PIDTYPE_PID);
}


typedef int (*proc_pident_readdir_t)(struct file *file, struct dir_context *ctx, const struct pid_entry *ents, unsigned int nents);

typedef struct dentry *(*proc_pident_lookup_t) (struct inode *dir, struct dentry *dentry, const struct pid_entry *ents, unsigned int nents);

typedef int (*pid_getattr_t) (const struct path *, struct kstat *, u32, unsigned int);

typedef int (*proc_setattr_t) (struct dentry *dentry, struct iattr *attr);



ssize_t fiberRead(struct file *, char __user *, size_t, loff_t *);

struct dentry *fiberLookup(struct inode *, struct dentry *, unsigned int);

int fiberReaddir(struct file *, struct dir_context *);



int register_kp_doExit(struct kprobe *);

void postH_doExit(struct kprobe *, struct pt_regs *, unsigned long);

void unregister_kp_doExit(struct kprobe *);

int register_kretp_finishTaskSwitch(struct kretprobe *);

int register_kretp_procReaddir(struct kretprobe *);

int register_kretp_procLookup(struct kretprobe *);

int preH_procLookup(struct kretprobe_instance *, struct pt_regs *);

int preH_procReaddir(struct kretprobe_instance *, struct pt_regs *);

int postH_procLookup(struct kretprobe_instance *, struct pt_regs *);

int postH_procReaddir(struct kretprobe_instance *, struct pt_regs *);

int postH_finishTaskSwitch(struct kretprobe_instance *, struct pt_regs *);

void unregister_kretp(struct kretprobe *);



