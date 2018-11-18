#include <linux/hashtable.h>
#include <asm/fpu/internal.h>
#include <asm/atomic.h>
#include <linux/spinlock.h>
#include <linux/bitmap.h>
#include <linux/types.h>
#include <linux/timekeeping32.h>

#define MAX_FLS 4096
#define STACK_SIZE (4096*2)

typedef void (*entry_point_t)(void *param);

typedef struct {
    pid_t tgid;
    atomic_t total_fibers;
    atomic_t total_threads;
    DECLARE_HASHTABLE(Fibers, 10);
    DECLARE_HASHTABLE(Threads, 10);
    struct hlist_node table_node;
} process_t;

typedef struct {
    spinlock_t lock;
    char running; 
    int fiber_id;
    int activations;
    int failed_activations;
    pid_t parent_pid;
    unsigned long exec_time;
    unsigned long start_time;
    long long fls[MAX_FLS];
    struct pt_regs *context;
    struct fpu *fpu_regs;
    void *initial_entry_point;
    char name[512];
    DECLARE_BITMAP(fls_bitmap, MAX_FLS);
    struct hlist_node table_node;
} fiber_t;


typedef struct {
    pid_t pid;
    fiber_t* running_fiber;
    struct hlist_node table_node;
} thread_t;


int convertThreadToFiber(void);

int createFiber(unsigned long, entry_point_t, void *);

int switchToFiber(int);

long long flsGet(long);

int flsFree(long);

long flsAlloc(void);

int flsSet(long, long long);

void removeProcess(process_t *);

void updateTimer(struct task_struct *, struct task_struct *);

inline process_t *getProcessByTgid(pid_t);

inline thread_t *getThreadByPid(process_t *, pid_t);

inline fiber_t *getFiberById(process_t *, int);