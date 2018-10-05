#include "fibermodule.h"

#define hash_add_process(hashtable) do{ \
    Process *process_to_add;    \
    process_to_add = kmalloc(sizeof(Process), GFP_KERNEL);  \
    hash_init(process_to_add->threads); \
    hash_init(process_to_add->fibers); \
    process_to_add->tgid = current->tgid;   \
    atomic_set(&(process_to_add->total_fibers));    \
    hash_add_rcu(hashtable, &(process_to_add->table_node), process_to_add->tgid);   \
} while(0)

#define hash_add_thread(hashtable, fiber_id) do{  \
    Thread *thread_to_add;  \
    thread_to_add = kmalloc(sizeof(Thread), GFP_KERNEL);    \
    thread_to_add->pid = current->pid;  \
    thread_to_add->active_fiber = fiber_id; \
    hash_add_rcu(hashtable, &(thread_to_add->table_node), thread_to_add->pid);  \
} while(0)

#define hash_add_fiber(hashtable) do{   \
    Fiber *fiber_to_add;    \
    fiber_to_add = kmalloc(sizeof(Fiber), GFP_KERNEL);  \
} while(0)
