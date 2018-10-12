#include <linux/kprobes.h> 
#include "fiber.h"

int preHandler(struct kprobe *, struct pt_regs *);

void postHandler(struct kprobe *, struct pt_regs *, unsigned long);

int register_doexit_probe(struct kprobe *);

void unregister_doexit_probe(struct kprobe *);