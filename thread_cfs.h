#define run_body(id) \
  pthread_mutex_lock(&mtx);\
        while(x!=0)\
                pthread_cond_wait(&cond, &mtx);\
        pthread_cond_broadcast(&cond);\
        pthread_mutex_unlock(&mtx);\

struct sched_class { /* Defined in 2.6.23:/usr/include/linux/sched.h */
      struct sched_class *next;
      void (*enqueue_task) (struct rq *rq, struct task_struct *p, int wakeup);
      void (*dequeue_task) (struct rq *rq, struct task_struct *p, int sleep);
      void (*yield_task) (struct rq *rq, struct task_struct *p);

      void (*check_preempt_curr) (struct rq *rq, struct task_struct *p);

      struct task_struct * (*pick_next_task) (struct rq *rq);
      void (*put_prev_task) (struct rq *rq, struct task_struct *p);

/*       unsigned long (*load_balance) (struct rq *this_rq, int this_cpu,
                 struct rq *busiest,
                 unsigned long max_nr_move, unsigned long max_load_move,
                 struct sched_domain *sd, enum cpu_idle_type idle,
                 int *all_pinned, int *this_best_prio); */

      void (*set_curr_task) (struct rq *rq);
      void (*task_tick) (struct rq *rq, struct task_struct *p);
      void (*task_new) (struct rq *rq, struct task_struct *p);
};

struct cfs_rq  //cfs调度队列
{
	struct load_weight load;
	unsigned long nr_running, h_nr_running;

	u64 min_vruntime;

	struct rb_root tasks_timeline;
	struct rb_node *rb_leftmost;

	struct list_head tasks;
	struct list_head *balance_iterator;

	struct sched_entity *curr, *next, *last, *skip;
};


struct sched_entity { //调度实体

	struct load_weight	load;		/* for load-balancing */
	struct rb_node		run_node;
	unsigned int		on_rq;  //线程是否在运行队列上

	u64			exec_start;
	u64			sum_exec_runtime;
	u64			vruntime;
	u64			prev_sum_exec_runtime;
	u64			nr_migrations;
};

struct thread_struct {  //线程pcb

 struct pthread_t pid; //线程id
 struct sched_entity se; 
//struct sched_class *sched_class;

};