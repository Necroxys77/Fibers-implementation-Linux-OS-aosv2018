#define init_process(np, ps) do {   \
    np = kzalloc(sizeof(process_t), GFP_KERNEL);  \
    np->tgid = current->tgid;   \
    atomic_set(&(np->total_fibers), 0); \
    atomic_set(&(np->total_threads), 0); \
    hash_init(np->Fibers);  \
    hash_init(np->Threads); \
    hash_add_rcu(ps, &(np->table_node), np->tgid);  \
    atomic_inc(&(np->total_threads));   \
} while (0)

#define init_thread(nt, p) do {    \
    nt = kzalloc(sizeof(thread_t), GFP_KERNEL);   \
    nt->pid = current->pid; \
    nt->running_fiber = NULL;    \
    hash_add_rcu(p->Threads, &(nt->table_node), nt->pid); \
} while (0)

#define init_fiber(nf, p) do {  \
    nf = kzalloc(sizeof(fiber_t), GFP_KERNEL);    \
    nf->context = kzalloc(sizeof(struct pt_regs), GFP_KERNEL);  \
    nf->fpu_regs = kzalloc(sizeof(struct fpu), GFP_KERNEL); \
    bitmap_zero(nf->fls_bitmap, MAX_FLS);  \
    memset(nf->fls, 0, sizeof(long long)*MAX_FLS); \
    spin_lock_init(&(nf->lock));    \
    memcpy(nf->context, task_pt_regs(current), sizeof(struct pt_regs)); \
    copy_fxregs_to_kernel(nf->fpu_regs);    \
    nf->fiber_id = atomic_inc_return(&(p->total_fibers));   \
    snprintf(nf->name, 256, "%d", nf->fiber_id); \
    nf->parent_pid = current->pid;  \
    nf->failed_activations = 0; \
    nf->activations = 0;  \
    nf->running = 0;    \
    nf->initial_entry_point = (void *) task_pt_regs(current)->ip;   \
    nf->exec_time = 0;  \
    nf->start_time = 0; \
    hash_add_rcu(p->Fibers, &(nf->table_node), nf->fiber_id);   \
} while(0)