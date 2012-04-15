#include "rbtree.h"
#include "rbtree_rc.h"

#include <pthread.h>
#include <stddef.h>

typedef unsigned char   u8;
typedef unsigned short  u16;
typedef unsigned int    u32;
typedef unsigned long long  u64;
typedef signed char   s8;
typedef short     s16;
typedef int     s32;
typedef long long   s64;

#define run_body(id) \
  pthread_mutex_lock(&mtx);\
        while(x!=0)\
                pthread_cond_wait(&cond, &mtx);\
        pthread_cond_broadcast(&cond);\
        pthread_mutex_unlock(&mtx);\

#define min(x, y) ({        \
  typeof(x) _min1 = (x);      \
  typeof(y) _min2 = (y);      \
  (void) (&_min1 == &_min2);    \
  _min1 < _min2 ? _min1 : _min2; })

#define max(x, y) ({        \
  typeof(x) _max1 = (x);      \
  typeof(y) _max2 = (y);      \
  (void) (&_max1 == &_max2);    \
  _max1 > _max2 ? _max1 : _max2; })

#define SCHED_LOAD_RESOLUTION  0
#define scale_load(w)    (w)
#define scale_load_down(w) (w)
#define   likely(x)               __builtin_expect(!!(x),   1) 
#define   unlikely(x)           __builtin_expect(!!(x),   0) 
#define WMULT_CONST (~0UL) 
#define WMULT_SHIFT  32 
#define LONG_MAX  ((long)(~0UL>>1))
#define ENQUEUE_WAKEUP    1
#define ENQUEUE_HEAD    2
#define ENQUEUE_WAKING    4
struct load_weight {
  unsigned long weight, inv_weight;
};


struct cfs_rq  
{
	unsigned long all_weight;
	unsigned long nr_running, h_nr_running;
  struct load_weight  load;
	u64 min_vruntime;
  u64 clock;

	struct rb_root tasks_timeline;
	struct rb_node *rb_leftmost;

	// struct list_head tasks;
	// struct list_head *balance_iterator;

	struct sched_entity *curr, *next, *last, *skip;

};


struct sched_entity { 

	struct load_weight	load;		/* for load-balancing */
	struct rb_node		run_node;
	unsigned int		on_rq;  
  int key;
  pthread_t pid;
  int is_exit;
  int static_prio;
  int setup;
  unsigned long weight;
	u64			exec_start;
	u64			sum_exec_runtime;
	u64			vruntime;
	u64			prev_sum_exec_runtime;
	u64			nr_migrations;
};

struct thread_struct { 

 pthread_t pid;
 struct sched_entity se; 
//struct sched_class *sched_class;

};

