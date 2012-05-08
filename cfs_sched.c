#include "thread_cfs.h"

#define random(x) (rand()%x)

//优先级到权重转换数组
static const int prio_to_weight[40] = { 
 /* -20 */     88761,     71755,     56483,     46273,     36291,
 /* -15 */     29154,     23254,     18705,     14949,     11916,
 /* -10 */      9548,      7620,      6100,      4904,      3906,
 /*  -5 */      3121,      2501,      1991,      1586,      1277,
 /*   0 */      1024,       820,       655,       526,       423,
 /*   5 */       335,       272,       215,       172,       137,
 /*  10 */       110,        87,        70,        56,        45,
 /*  15 */        36,        29,        23,        18,        15,
};

//方便运算 2^x/prio_to_weight
static const u32 prio_to_wmult[40] = {  
 /* -20 */     48388,     59856,     76040,     92818,    118348,
 /* -15 */    147320,    184698,    229616,    287308,    360437,
 /* -10 */    449829,    563644,    704093,    875809,   1099582,
 /*  -5 */   1376151,   1717300,   2157191,   2708050,   3363326,
 /*   0 */   4194304,   5237765,   6557202,   8165337,  10153587,
 /*   5 */  12820798,  15790321,  19976592,  24970740,  31350126,
 /*  10 */  39045157,  49367440,  61356676,  76695844,  95443717,
 /*  15 */ 119304647, 148102320, 186737708, 238609294, 286331153,
};



static pthread_mutex_t mtx = PTHREAD_MUTEX_INITIALIZER ;
pthread_mutex_t cfs_tree_mtx = PTHREAD_MUTEX_INITIALIZER ;
static pthread_cond_t cond = PTHREAD_COND_INITIALIZER;

unsigned int sysctl_sched_latency = 6000ULL; //单位微秒,最小调度周期6ms
unsigned int normalized_sysctl_sched_latency = 6000UL;
unsigned int sysctl_sched_min_granularity = 1000ULL;
unsigned int normalized_sysctl_sched_min_granularity = 1000ULL;
static unsigned int sched_nr_latency = 8;

pthread_t run_id=0;
pid_t now_id=0;
struct cfs_rq *cfs; //等待进程队列

void dequeue_entity( struct sched_entity *se);
void enqueue_entity(struct sched_entity *se,int flags) ;
int pthread_kill(pthread_t thread, int sig);
static void place_entity(struct sched_entity *se, int initial);

static void set_load_weight(struct thread_struct *p)
{
	int prio = p->se.static_prio - MAX_RT_PRIO;
	struct load_weight *load = &p->se.load;

	/*
	 * SCHED_IDLE tasks get minimal weight:
	 */
	// if (p->policy == SCHED_IDLE) {
	// 	load->weight = scale_load(WEIGHT_IDLEPRIO);
	// 	load->inv_weight = WMULT_IDLEPRIO;
	// 	return;
	// }

	load->weight = scale_load(prio_to_weight[prio]);
	load->inv_weight = prio_to_wmult[prio];
}

void *thread(int *nn){
		int a[3001]={};
		int b[3001]={};
		int c[3001]={};
		c[1]=2;
		b[1]=1;
		pthread_t self_id=pthread_self();       
		int nm = *nn;
	for(int i=1;i<=(nm-2);i++)
	{   
		pthread_mutex_lock(&mtx);
	
		while(run_id != self_id)
		{
			pthread_cond_wait(&cond, &mtx);
		}
		
		pthread_mutex_unlock(&mtx);
		
		for(int j=1;j<=3000;j++)
		{   
			a[j]=b[j];
			b[j]=c[j];
			c[j]=0;
		}
		for(int j=1;j<=3000;j++)
		{
			c[j]=c[j]+a[j]+b[j];
			if(c[j]>=10)
			{
				c[j]=c[j]-10;
				c[j+1]=1;
			}
		}   
		usleep(100+random(40));//实际usleep精度并不是1微妙，大概是1000微妙左右
		// printf("\nthread is running: id is %lu    n is:%d \n",pthread_self(),*n);
	}  
	pthread_cond_broadcast(&cond);
	int i=3000;
	while(c[i]==0) i--;
	for(i=i;i>=1;i--)
		printf("%d",c[i]);
	printf("\n");
	printf("\nthread is finish: id is %lu    n is:%d \n",pthread_self(),nm);
	// cfs->nr_running -=1;
	printf("the left thread is: %d \n",(int)cfs->nr_running);
	// pthread_detach(pthread_self());
	free(nn);


}

#define TIME_TICK 1000  //模拟多少时间，触发一次时钟中断,单位为微秒us

int thread_is_alive(pthread_t pid) //判断线程是否死掉
{

	 int singal;
	 singal =  pthread_kill(pid,0); 
	 return singal == ESRCH ? 0:1;

}

static void update_cfsrq_clock()//更新队列信息
{
	struct timeval now; 

	gettimeofday(&now, NULL);
	cfs->clock=((u64)(now.tv_sec))*1000000+now.tv_usec;
	// printf("\nclock is:%llu\n\n\n",cfs->clock);

}

static inline int entity_before(struct sched_entity *a,
				struct sched_entity *b)
{
	return (s64)(a->vruntime - b->vruntime) < 0;
}

#define SRR(x, y) (((x) + (1UL << ((y) - 1))) >> (y))
static unsigned long
calc_delta_mine(unsigned long delta_exec, unsigned long weight, struct load_weight *lw)
{
	u64 tmp;
	// printf("zzzzzzzzzzzzzz: weight: %lu\n",weight);

			// printf("hahahahahahahahahahahaha %lu\n",lw->weight);
	// return delta_exec * weight / lw->weight;
	if (likely(weight > (1UL << SCHED_LOAD_RESOLUTION)))
		tmp = (u64)delta_exec * scale_load_down(weight);
	else
		tmp = (u64)delta_exec;
		// printf("ppppppppppppppppp  %d %llu\n",cfs->nr_running,tmp);	
	if (unlikely(tmp > WMULT_CONST))
		tmp = SRR(SRR(tmp, WMULT_SHIFT/2) * lw->inv_weight, WMULT_SHIFT/2);
	else
		tmp = SRR(tmp * lw->inv_weight, WMULT_SHIFT);
		// printf("wwwwwwwwwwwwwwwww   %llu\n",tmp);	
	return (unsigned long)min(tmp, (u64)(unsigned long)LONG_MAX);
}


static inline unsigned long
calc_delta_fair(unsigned long delta, struct sched_entity *se) //计算虚拟时钟增加的量
{
	if (unlikely(se->load.weight != NICE_0_LOAD))
		delta = calc_delta_mine(delta, NICE_0_LOAD, &se->load);
return delta;
}

static u64 __sched_period(unsigned long nr_running) //计算调度周期根据队列上的线程数
{
	u64 period = sysctl_sched_latency;
	unsigned long nr_latency = sched_nr_latency;

	if (unlikely(nr_running > nr_latency)) {
		period = sysctl_sched_min_granularity;
		period *= nr_running;
	}
		// printf("priod is %llu \n",period);
	return period;
}

static inline void update_load_add(struct load_weight *lw, unsigned long inc) //更新整个队列的权重
{
	lw->weight += inc;
	lw->inv_weight = 0;
}
static void account_entity_enqueue( struct sched_entity *se)
{

	update_load_add(&cfs->load, se->load.weight);
	inc_nr_running();
}
static inline void update_load_sub(struct load_weight *lw, unsigned long dec)
{
	lw->weight -= dec;
	lw->inv_weight = 0;
}

static void account_entity_dequeue( struct sched_entity *se)
{
	update_load_sub(&cfs->load, se->load.weight);
	
	// dec_nr_running();
}


static u64 sched_slice( struct sched_entity *se)
{
	u64 slice = __sched_period(cfs->nr_running + !se->on_rq);


		struct load_weight *load;
		struct load_weight lw;

		load = &cfs->load;

		if (unlikely(!se->on_rq)) {
			lw = cfs->load;
			update_load_add(&lw, se->load.weight);
			load = &lw;
		}

		slice = calc_delta_mine(slice, se->load.weight, load);

		// printf("slice is %llu \n",slice);
	return slice;
}

static u64 sched_vslice( struct sched_entity *se)
{
	return calc_delta_fair(sched_slice( se), se);
}



static inline u64 min_vruntime(u64 min_vruntime, u64 vruntime)
{
	s64 delta = (s64)(vruntime - min_vruntime);
	if (delta < 0)
		min_vruntime = vruntime;

	return min_vruntime;
}

static inline u64 max_vruntime(u64 min_vruntime, u64 vruntime)
{
	s64 delta = (s64)(vruntime - min_vruntime);
	// printf(">>>>>>>>>>>>>>> %llu",min_vruntime);
	if (delta > 0)
	{
		min_vruntime = vruntime;
		
}
	return min_vruntime;
}

static void update_min_vruntime()
{
	u64 vruntime = cfs->min_vruntime;

	if (cfs->curr)
		vruntime = cfs->curr->vruntime;

	if (cfs->rb_leftmost) {
		struct sched_entity *se = rb_entry(cfs->rb_leftmost, struct sched_entity, run_node);

		if (!cfs->curr)
			vruntime = se->vruntime;
		else
			vruntime = min_vruntime(vruntime, se->vruntime);
	}

	cfs->min_vruntime = max_vruntime(cfs->min_vruntime, vruntime);

}

static inline void update_stats_curr_start(struct sched_entity *se)
{
	
	se->exec_start = cfs->clock;
}

static void update_curr(struct sched_entity *se) //更新当前线程信息
{
	u64 now = cfs->clock;
	struct sched_entity *curr = cfs->curr;
	unsigned long delta_exec;
	if (unlikely(!curr))
		return;
	delta_exec = (unsigned long)(now - se->exec_start);
	// printf("zzzzzzzzzzzzzzzzzzzzzzzz %lu  %llu %llu\n", delta_exec,now,se->exec_start);
	if (!delta_exec)
		return;

	unsigned long delta_exec_weighted;
	se->sum_exec_runtime += delta_exec; //实际运行时间
	delta_exec_weighted = calc_delta_fair(delta_exec, se);
	se->vruntime += delta_exec_weighted;
	/*printf("xxxxxxxxxxxxxxxxxxxxxxx delta:%lu se->vruntime:%llu prio:%d  run_id:%lu\n",delta_exec_weighted,se->vruntime,se->static_prio,run_id);*/
	// if(se->vruntime > 100000000)
	// 	exit(1);
	se->exec_start = now;
	update_min_vruntime();	

}


static void entity_tick() //时钟中断调用
{
 update_curr(cfs->curr);

}

static struct sched_entity *__pick_first_entity(struct cfs_rq *cfs_rq)
{
	struct rb_node *left = cfs_rq->rb_leftmost;

	if (!left)
		return NULL;

	return rb_entry(left, struct sched_entity, run_node);
}

static int check_preempt_tick(struct sched_entity *curr)
{
	unsigned long ideal_runtime, delta_exec;
	struct sched_entity *se;
	s64 delta;

	ideal_runtime = sched_slice(curr);
	delta_exec = curr->sum_exec_runtime - curr->prev_sum_exec_runtime;
	if (delta_exec > ideal_runtime) {
		// resched_task(cfs->curr);
		/*
		 * The current task ran long enough, ensure it doesn't get
		 * re-elected due to buddy favours.
		 */
		// clear_buddies(cfs_rq, curr);
		return 1;
	}

	
	if (delta_exec < sysctl_sched_min_granularity)
		return 0;
	return 0;

	se = __pick_first_entity(cfs);
	delta = curr->vruntime - se->vruntime;

	if (delta < 0)
		return 0;

	if (delta > ideal_runtime)
		// resched_task(cfs->curr);
		return 1;
}

static void *main_thread()//主调度线程
{
	struct timeval t_start,t_end;
	struct sched_entity *curr  = rb_entry(cfs->rb_leftmost, struct sched_entity, run_node);
	struct sched_entity *prev =NULL;
	cfs->curr=curr;
	struct sched_entity *left_node = curr;
	update_cfsrq_clock();
	update_stats_curr_start(cfs->curr);	
	dequeue_entity(curr); //出队运行
				// printf("iiiiiiiiiiiiiiiiiiiiiii %p %p\n",curr,cfs->curr);

	while(1)
	{

	
		left_node  = rb_entry(cfs->rb_leftmost, struct sched_entity, run_node);
		// printf("nnnnnnnnnnnnn %p nr %d\n",left_node,cfs->nr_running );
		if(cfs->nr_running == 0)
		{
		// printf("nnnnnnnnnnnnn %p\n",left_node );
			break;
		}
		update_cfsrq_clock();
		// printf("bbbbbbbbbbbbbbbbb\n");
// printf("tttttttttttttttttttttttttttt %p %p id: %lu nr:%d \n",curr,left_node,run_id,cfs->nr_running);
// printf("aaaaaaaaaaaaaaaaaaaaaaaaaaaa %-22llu %-22llu id: %lu nr:%d \n",cfs->curr->vruntime,left_node->vruntime,run_id,cfs->nr_running);
// printf("mmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmm: %llu %llu nr_running: %d\n", cfs->min_vruntime,cfs->clock,cfs->nr_running);
		entity_tick();
		
		// printf("curr->vruntime: %llu pid: %lu\nleft->vruntime: %llu left_node->pid:%lu\n\n ",cfs->curr->vruntime,curr->pid,left_node->vruntime,left_node->pid);	
		
			/*printf("min_vvvvvvvvvvvvvvv: %llu %lu\n",cfs->min_vruntime,run_id);*/
	
		if(check_preempt_tick(cfs->curr))
		{

			// printf("vvvvvvvvvvvvvvvvvvvv: \n");
			prev = cfs->curr;
			cfs->curr->prev_sum_exec_runtime = cfs->curr->sum_exec_runtime; //暂时代码
			cfs->curr=__pick_first_entity(cfs);
			update_stats_curr_start(cfs->curr);
				// printf("jjjjjjjjjjjjjjjjjjjjjjj curr->vruntime %llu  curr_id: %lu\n",cfs->curr->vruntime,cfs->curr->pid);
      get_sched_info(cfs->curr,prev) ;
      /*printf("currid:%lu prev_id%lu\n",cfs->curr->p_pid,prev->p_pid);*/
			if(thread_is_alive(prev->pid) )
			{
				// printf("mmmmmmmmmmmmmmm curr->vruntime:%llu curr->pid:%lu %d\n ",curr->vruntime,curr->pid,cfs->nr_running); 
				__enqueue_entity(prev); //如果进程退出，不用进队//se != cfs=curr
				update_load_add(&cfs->load,prev->load.weight);
				cfs->nr_running++;
			}	
      else{
            put_exit_info(prev);
       /*put_sched_info(prev,cfs->curr) ;*/
      }
			dequeue_entity(cfs->curr);
			curr = cfs->curr;
		}

	
		run_id =cfs->curr->pid;
				   // printf("ooooooooooooooooooooooo %lu\n",run_id);
		pthread_cond_broadcast(&cond);

		usleep(1);//模拟每隔1000us左右触发一次时钟中断

	}

// printf("\n\n\nffffffffffffffffffffff %lu\n",run_id);
	// curr = rb_entry(&cfs->tasks_timeline.rb_node, struct sched_entity, run_node);

	pthread_join(curr->pid,NULL);
	// run_id = curr->pid;
	//  pthread_cond_broadcast(&cond);
	
	
}

 void inc_nr_running()//增加队列等待线程的数量
{
	cfs->nr_running++;
}
 void dec_nr_running()//减少队列等待线程的数量
{
	cfs->nr_running--;
}

int init_thread(int n,struct thread_struct *thread_group) //初始化所有线程,n代表线程数目
{

	struct thread_struct * pc;
	for(int i=0;i<n;i++)
	{

		pc = calloc(sizeof(struct thread_struct) , 1);
		*(thread_group)=*pc;
		thread_group->pid=1000;
		thread_group++;
	}
	thread_group->pid=1;//确保最后一个元素为1

	return 0;
}

int cal_setup(unsigned int  weight) //优先级越高的走的越慢
{

  return   1000*1024/weight;

}


 //获得线程对应的进程id ，用于绘图时的显示,靠主进程的进程id猜测
int get_thread_pid(pthread_t t_pid)
{
	return now_id++;

}


//初始化线程调度实体信息
void init_thread_info(struct thread_struct *ready_thread,pthread_t id,int num)
{
	struct sched_entity *curr = &ready_thread->se;
	ready_thread->pid = id;
  curr->cal_num = num;
	curr->pid = id;
	curr->p_pid = ++now_id;
	curr->static_prio=random(40)+100;
	set_load_weight(ready_thread);
	curr->weight = prio_to_weight[curr->static_prio];
	cfs->all_weight += curr->weight;
	curr->on_rq=1;
	curr->sum_exec_runtime = 0;
	curr->prev_sum_exec_runtime =0;
	curr->exec_start=0;
	// printf("bbbbbbbbbbbbbbbbbbbbbbbbbbb %2d %lu %lu %d %d\n",curr->static_prio,id,curr->weight,curr->setup,curr->key );
	printf("bbbbbbbbbbbbbbbbbbbbbbbbbbb  %d %lu %lu\n",curr->static_prio,curr->load.weight,curr->load.inv_weight);
  put_init_info(curr);
}


void ready_run_thread(struct thread_struct *ready_thread,int *n) //准备运行线程
{
	pthread_t id;	
	srand((int)time(0));
	while(ready_thread->pid != 1)
	{	
		int *m=(int *)malloc(sizeof(int));
		*m = random(9000)+1000;
		int ret=pthread_create(&id,NULL,(void *)thread,m);
		if(ret!=0){
			printf ("创建线程失败: %s\n",strerror(ret));
			exit (1);
		}
		init_thread_info(ready_thread,id,*m);	
		enqueue_entity(&ready_thread->se,1);
	 printf("summmmmmmmm is id:%lu %llu %llu\n",ready_thread->pid,ready_thread->se.vruntime,ready_thread->se.sum_exec_runtime);
		ready_thread++;

	}
}

void __dequeue_entity(struct sched_entity *se)
{
	if (cfs->rb_leftmost == &se->run_node) {
		struct rb_node *next_node;

		next_node = rb_next(&se->run_node);
		cfs->rb_leftmost = next_node;
	}
pthread_mutex_lock(&cfs_tree_mtx);
rb_erase(&se->run_node, &cfs->tasks_timeline);
pthread_mutex_unlock(&cfs_tree_mtx);
	cfs->nr_running -=1;
}

void dequeue_entity(struct sched_entity *se)
{

	// if (se != cfs->curr)
	
		__dequeue_entity( se);
	
	se->on_rq = 0;
	account_entity_dequeue(se);

	update_min_vruntime();

}

static void place_entity( struct sched_entity *se, int initial)
{
	u64 vruntime = cfs->min_vruntime;

	if (initial )//&& sched_feat(START_DEBIT)) //待修改
		vruntime += sched_vslice( se);


	// if (!initial) {
	// 	unsigned long thresh = sysctl_sched_latency;

	// 	if (sched_feat(GENTLE_FAIR_SLEEPERS))
	// 		thresh >>= 1;

	// 	vruntime -= thresh;
	// }
	
	printf("kkkkkkkkkkkkkkkkkkkkkkkkk %llu %llu\n",se->vruntime,vruntime);
	// vruntime = max_vruntime(se->vruntime, vruntime);

	se->vruntime = vruntime;
}


//将调度实体插入红黑树，在插入过程中缓存最左边的节点
void __enqueue_entity(struct sched_entity *se) //插入单个节点到红黑树
{
	struct rb_node **new= &cfs->tasks_timeline.rb_node;
	struct rb_node *parent =NULL;
	struct sched_entity *this;
	int leftmost = 1;

	while(*new)
	{
		parent = *new;
		this = rb_entry(parent, struct sched_entity, run_node);


		if ( entity_before(se,this) )
		{
			new = &parent->rb_left;
		}
		else 
		{
			new = &parent->rb_right;
			leftmost = 0; 
		}

	}
	pthread_mutex_lock(&cfs_tree_mtx);
	if (leftmost)
		cfs->rb_leftmost = &se->run_node;

	rb_link_node(&se->run_node, parent, new);
	rb_insert_color(&se->run_node,&cfs->tasks_timeline);
	pthread_mutex_unlock(&cfs_tree_mtx);
	
}

void enqueue_entity(struct sched_entity *se, int flags )
{

	if (!(flags & ENQUEUE_WAKEUP) )//|| (flags & ENQUEUE_WAKING))
	{
		se->vruntime += cfs->min_vruntime;
	}
	update_curr(cfs->curr);
	account_entity_enqueue(se);

	if (flags )//& ENQUEUE_WAKEUP)
{
		place_entity( se, 1);
	}	

	
	if (se != cfs->curr) 
	{
		__enqueue_entity(se); //插入红黑树
	}
	se->on_rq = 1;	

}


void init_cfs_rq()  //初始化cfs运行队列
{
	cfs =(struct cfs_rq *)malloc(sizeof(struct cfs_rq));
	cfs->tasks_timeline = RB_ROOT;
	cfs->all_weight = 0;
	cfs->clock = 0;
	cfs->min_vruntime =0; 
	// cfs->min_vruntime = (u64)(-(1LL << 20));
	cfs->nr_running = 0; //队列初始化数量
	cfs->curr = 0;
	cfs->load.weight = 0;
	cfs->load.inv_weight = 0;
}

void usage()
{
	printf("  程序参数错误！\n");
	printf("  用法:\n\t./cfs n   n=你要运行的线程数\n\n");
}

void on_button3_clicked()
{
  struct thread_struct *pc = calloc(sizeof(struct thread_struct) , 1);
   pthread_t id; 
	srand((int)time(0));
		int *m=(int *)malloc(sizeof(int));
		*m = random(1000)+9000;
		int ret=pthread_create(&id,NULL,(void *)thread,m);
		if(ret!=0){
			printf ("创建线程失败: %s\n",strerror(ret));
			exit (1);
		}
		init_thread_info(pc,id,*m);	
    pc->se.run_node.new=1;

		enqueue_entity(&pc->se,1);

}


int main(int argc,char *argv[])
{
	pthread_t sched_id,dot_id;
	int ret;
	int size = 100;
	int n =1000;
	int t_nu=0;
  int main_id=getpid(); 
  now_id=main_id;
	 if( argc ==1 || argc> 2 )
	{
		usage();
    	exit(0);
	}
	else
		t_nu=atoi(argv[1]);
  sched_info_str_buffer = fopen("sched_file.txt","w+");
  sched_output = fopen("sched_file.txt","r");
	init_cfs_rq();
	struct rb_root *cfs_tree= &cfs->tasks_timeline;
	struct thread_struct thread_group[t_nu];

 	ret=pthread_create(&dot_id,NULL,(void *)gtk,NULL); 	
 	if(ret!=0){
			printf ("Create pthread error!\n");
			exit (1);
		}
  sleep(1);
	init_thread(t_nu,thread_group);
	ready_run_thread(thread_group,&n);
  sleep(1);

   ret=pthread_create(&sched_id,NULL,(void *)main_thread,NULL); //初始化调度线程
    if(ret!=0){
      printf ("Create pthread error!\n");
      exit (1);
    }

   pthread_join(dot_id,NULL);

}
