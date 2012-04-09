#include "rbtree.h"
#include "rbtree_rc.h"

#include <pthread.h>
#include <stddef.h>
typedef long long u64;
#define run_body(id) \
  pthread_mutex_lock(&mtx);\
        while(x!=0)\
                pthread_cond_wait(&cond, &mtx);\
        pthread_cond_broadcast(&cond);\
        pthread_mutex_unlock(&mtx);\

struct load_weight {
  unsigned long weight, inv_weight;
};


struct cfs_rq  
{
	unsigned long all_weight;
	unsigned long nr_running, h_nr_running;

	u64 min_vruntime;

	struct rb_root tasks_timeline;
	struct rb_node *rb_leftmost;

	// struct list_head tasks;
	// struct list_head *balance_iterator;

	struct sched_entity *curr, *next, *last, *skip;

};


struct sched_entity { 

	//struct load_weight	load;		/* for load-balancing */
	struct rb_node		run_node;
	unsigned int		on_rq;  
  int key;
  pthread_t pid;
  int is_exit;
  int static_prior;
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

