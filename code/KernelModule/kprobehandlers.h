#include <linux/kprobes.h> 
#include "fiber.h"

//int pre_do_exit(struct kprobe *, struct pt_regs *);

int pre_schedule(struct kretprobe_instance *, struct pt_regs *);

void postHandler(struct kprobe *, struct pt_regs *, unsigned long);

void post_do_exit(struct kprobe *, struct pt_regs *, unsigned long);

int post_schedule(struct kretprobe_instance *, struct pt_regs *);




int register_kp(struct kprobe *, char);

void unregister_kp(struct kprobe *);

int register_kretp(struct kretprobe *);

void unregister_kretp(struct kretprobe *);