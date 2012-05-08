#include "rbtree.h"
#include "rbtree_rc.h"
#include <gtk/gtk.h>
#include <glade/glade.h>
#include <pthread.h> #include <stddef.h> #include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/time.h>
#include <time.h>
#include <errno.h>

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

#define MAX_USER_RT_PRIO	100  //最大用户实时优先级
#define MAX_RT_PRIO		MAX_USER_RT_PRIO
#define NICE_0_LOAD		1024
#define NICE_TO_PRIO(nice)	(MAX_RT_PRIO + (nice) + 20)
#define PRIO_TO_NICE(prio)	((prio) - MAX_RT_PRIO - 20)
#define TASK_NICE(p)		PRIO_TO_NICE((p)->static_prio)


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

	struct load_weight	load;		
	struct rb_node		run_node;
	unsigned int		on_rq;  
  int key;
  int cal_num;//计算步数
  pid_t p_pid;

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
 pid_t pid;
 struct sched_entity se; 
//struct sched_class *sched_class;

};
pthread_mutex_t cfs_tree_mtx ;
extern int if_output_tree;

extern int *tree_to_image(); //将二叉树输出为文件，独立线程不对调度产生影响
extern void *gtk (int argc, char *argv[]);
extern void put_exit_info(struct sched_entity *se);
extern void get_sched_info(struct sched_entity *prev,struct sched_entity *curr);

extern void put_init_info(struct sched_entity *se);
extern GtkTextBuffer *info_area;
extern GtkTextTag *tag;
extern GtkTextTag *blue_tag;
extern GtkScrolledWindow *swindow;

extern GtkTextBuffer *sched_info_area;
extern GtkScrolledWindow *sched_swindow;
extern GtkTextView *sched_textview;



extern FILE *sched_info_str_buffer; 
extern FILE *sched_output; 

extern GtkTextBuffer *init_info_area;
extern GtkTextView  *init_textview;

extern struct cfs_rq *cfs; //等待进程队列


