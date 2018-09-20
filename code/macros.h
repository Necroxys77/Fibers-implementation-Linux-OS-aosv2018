#include "fibermodule.h"

#define hashtable_thread_add(hashtable, pid, fib) do {   \
    struct table_element_thread *new_element;   \
    new_element = kmalloc(sizeof(struct table_element_thread), GFP_KERNEL); \
    new_element->parent = pid;  \
    atomic_set(&(new_element->total_fibers), 0);    \
    new_element->fiber = fib;   \
    hash_add_rcu(hashtable, &(new_element->table_node), new_element->parent);   \
} while(0)

#define hashtable_fiber_add(hashtable, fib) do {   \
    struct table_element_fiber *new_element;   \
    new_element = kmalloc(sizeof(struct table_element_fiber), GFP_KERNEL); \
    new_element->fiber = fib;   \
    hash_add_rcu(hashtable, &(new_element->table_node), new_element->fiber->fiber_id);   \
} while(0)