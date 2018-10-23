#include <linux/kprobes.h> 
#include "fiber.h"
#include <linux/proc_fs.h>

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
    const struct pid_entry *ents;
    unsigned int nents;
};

//int pre_do_exit(struct kprobe *, struct pt_regs *);

int pre_schedule(struct kretprobe_instance *, struct pt_regs *);

void postHandler(struct kprobe *, struct pt_regs *, unsigned long);

void post_do_exit(struct kprobe *, struct pt_regs *, unsigned long);

int post_schedule(struct kretprobe_instance *, struct pt_regs *);

int register_kp(struct kprobe *);

void unregister_kp(struct kprobe *);

int register_kretp(struct kretprobe *);

int register_kretp_proc_readdir(struct kretprobe *);

void unregister_kretp(struct kretprobe *);

int register_jp(struct jprobe *);

void unregister_jp(struct jprobe *);