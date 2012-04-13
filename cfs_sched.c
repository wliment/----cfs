#include "rbtree_rc.h" 
#include "thread_cfs.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/time.h>
#include <time.h>
#include <errno.h>
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



#define MAX_USER_RT_PRIO	100  //最大用户实时优先级
#define MAX_RT_PRIO		MAX_USER_RT_PRIO
#define NICE_0_LOAD		1024
#define NICE_TO_PRIO(nice)	(MAX_RT_PRIO + (nice) + 20)
#define PRIO_TO_NICE(prio)	((prio) - MAX_RT_PRIO - 20)
#define TASK_NICE(p)		PRIO_TO_NICE((p)->static_prio)

static pthread_mutex_t mtx = PTHREAD_MUTEX_INITIALIZER ;
static pthread_cond_t cond = PTHREAD_COND_INITIALIZER;

unsigned int sysctl_sched_latency = 6000ULL; //单位微秒,最小调度周期6ms
unsigned int normalized_sysctl_sched_latency = 6000UL;
unsigned int sysctl_sched_min_granularity = 750ULL;
unsigned int normalized_sysctl_sched_min_granularity = 750ULL;
static unsigned int sched_nr_latency = 8;

pthread_t run_id=0;
struct cfs_rq *cfs; //等待进程队列

void dequeue_entity( struct sched_entity *se);
void enqueue_entity(struct sched_entity *se) ;
int pthread_kill(pthread_t thread, int sig);
static void place_entity(struct sched_entity *se, int initial);

static void set_load_weight(struct thread_struct *p)
{
	int prio = p->se.static_prio - MAX_RT_PRIO;
	prio=0;
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
			//printf("thread %lu wait process \n",self_id);
			pthread_cond_wait(&cond, &mtx);
		}
		
		//pthread_cond_broadcast(&cond);
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
		usleep(100);//实际usleep精度并不是1微妙，大概是1000微妙左右
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
	// printf("zzzzzzzzzzzzzz: delta_exec  %lu * weight: %lu /lw->weight: %lu = %lu\n",delta_exec,weight,lw->weight,delta_exec * weight / lw->weight);
	return delta_exec * weight / lw->weight;
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
// printf("nnnnnnnnnnnnnnnnn   %lu\n",delta);
	if (unlikely(se->load.weight != NICE_0_LOAD))

		delta = calc_delta_mine(delta, NICE_0_LOAD, &se->load);
// printf("nnnnnnnnnnnnnnnnn   %lu\n",delta);
return delta;
}

static u64 __sched_period(unsigned long nr_running) //计算调度周期更具队列上的线程数
{
	u64 period = sysctl_sched_latency;
	unsigned long nr_latency = sched_nr_latency;

	if (unlikely(nr_running > nr_latency)) {
		period = sysctl_sched_min_granularity;
		period *= nr_running;
	}
	// printf("\n\nxxxxxxxxxxxxxxxxxxxx:%lu\n\n",period);
	return period;
}

static inline void update_load_add(struct load_weight *lw, unsigned long inc)
{
	lw->weight += inc;
	lw->inv_weight = 0;
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
	if (delta > 0)
		min_vruntime = vruntime;

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


static void update_curr(struct sched_entity *se) //更新当前线程信息
{
	u64 now = cfs->clock;
	unsigned long delta_exec;

	delta_exec = (unsigned long)(now - se->exec_start);
	// printf("llllllllllllllllll:%lu\n\n",delta_exec);
	unsigned long delta_exec_weighted;

	se->sum_exec_runtime += delta_exec;
	delta_exec_weighted = calc_delta_fair(delta_exec, se);

	se->vruntime += delta_exec_weighted;
	se->exec_start = now;

	update_min_vruntime();	

}


static void entity_tick() //时钟中断调用
{
 update_curr(cfs->curr);

}
static void *main_thread()//主调度线程
{
	struct timeval t_start,t_end;
	struct sched_entity *curr  = rb_entry(cfs->rb_leftmost, struct sched_entity, run_node);
	cfs->curr=curr;
	struct sched_entity *left_node = curr;
		
	dequeue_entity(curr); //出队运行

	while(1)
	{

	
		left_node  = rb_entry(cfs->rb_leftmost, struct sched_entity, run_node);
		if(cfs->nr_running == 0)
			break;

		update_cfsrq_clock();
printf("mmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmm %llu %llu id: %lu nr:%d \n",cfs->curr->vruntime,left_node->vruntime,run_id,cfs->nr_running);
		entity_tick();
// printf("bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb\n");
		
		// printf("curr->vruntime: %llu pid: %lu\nleft->vruntime: %llu left_node->pid:%lu\n\n ",cfs->curr->vruntime,curr->pid,left_node->vruntime,left_node->pid);	
		
	
		if(left_node->vruntime + 150 <curr->vruntime)
		{

			// printf("vvvvvvvvvvvvvvvvvvvv: cfs->nr_running: %d %lu \n",cfs->nr_running,curr->pid);
			if(thread_is_alive(curr->pid) )
			{
				enqueue_entity(curr); //如果进程退出，不用进队
				 // printf("ppppppppppppppppppppppp\n");
			}	
			curr = left_node;
			cfs->curr=curr;
			dequeue_entity(curr);
			// printf("hhhhhhhhhhhhhhhh\n");

		}

				
	
		run_id =curr->pid;
		pthread_cond_broadcast(&cond);

		usleep(1);//模拟每隔1000us左右触发一次时钟中断

	}

printf("\n\n\nffffffffffffffffffffff %lu\n",run_id);
	// curr = rb_entry(&cfs->tasks_timeline.rb_node, struct sched_entity, run_node);

	pthread_join(curr->pid,NULL);
	// run_id = curr->pid;
	//  pthread_cond_broadcast(&cond);
	
	
}



 void inc_nr_running()//增加队列等待线程的数量
{
	cfs->nr_running++;
}

int init_thread(int n,struct thread_struct *thread_group) //初始化所有线程,n代表线程数目
{

	struct thread_struct * pc;
	for(int i=0;i<n-1;i++)
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


//初始化线程调度实体信息
void init_thread_info(struct thread_struct *ready_thread,pthread_t id)
{

	ready_thread->pid = id;
	ready_thread->se.is_exit = 0;
	ready_thread->se.pid = id;
	ready_thread->se.static_prio=random(40)+100;
	set_load_weight(ready_thread);
	ready_thread->se.weight = prio_to_weight[ready_thread->se.static_prio];
	cfs->all_weight += ready_thread->se.weight;
	// ready_thread->se.setup = cal_setup(ready_thread->se.weight);
	// ready_thread->se.key=10000+random(311);
	ready_thread->se.on_rq=1;
	ready_thread->se.sum_exec_runtime =0;
	// printf("bbbbbbbbbbbbbbbbbbbbbbbbbbb %2d %lu %lu %d %d\n",ready_thread->se.static_prio,id,ready_thread->se.weight,ready_thread->se.setup,ready_thread->se.key );
	printf("bbbbbbbbbbbbbbbbbbbbbbbbbbb  %d %lu %lu\n",ready_thread->se.static_prio,ready_thread->se.load.weight,ready_thread->se.load.inv_weight);
}


void ready_run_thread(struct thread_struct *ready_thread,int *n) //准备运行线程
{
	pthread_t id;	
	srand((int)time(0));
	while(ready_thread->pid != 1)
	{	
		int *m=(int *)malloc(sizeof(int));
		*m = random(1000)+9000 ;
		int ret=pthread_create(&id,NULL,(void *)thread,m);
		if(ret!=0){
			printf ("创建线程失败!\n");
			exit (1);
		}
		init_thread_info(ready_thread,id);	
		place_entity(&ready_thread->se, 1);  
	printf("summmmmmmmm is id:%lu %llu\n",ready_thread->pid,ready_thread->se.vruntime);
		enqueue_entity(&ready_thread->se);
		ready_thread++;

	}
	// sleep(100);
		printf("fffffffffffffffffffffffffffff\n");
}

	void dequeue_entity(struct sched_entity *se)
	{
		if (cfs->rb_leftmost == &se->run_node) {
			struct rb_node *next_node;

			next_node = rb_next(&se->run_node);
			cfs->rb_leftmost = next_node;
		}

		rb_erase(&se->run_node, &cfs->tasks_timeline);
		cfs->nr_running -=1;
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
	
	vruntime = max_vruntime(se->vruntime, vruntime);

	se->vruntime = vruntime;
}


//将调度实体插入红黑树，在插入过程中缓存最左边的节点
void enqueue_entity(struct sched_entity *se ) //插入单个节点到红黑树
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

	if (leftmost)
		cfs->rb_leftmost = &se->run_node;

	rb_link_node(&se->run_node, parent, new);
	rb_insert_color(&se->run_node,&cfs->tasks_timeline);
	inc_nr_running();
}


void insert_thread_group(struct thread_struct *thread_g)	
{

	while(thread_g->pid != 1)
	{

		enqueue_entity(&thread_g->se);
		thread_g++;
	}
}

// void wait_thread(struct thread_struct *thread_g)
// {
// 	while(thread_g->pid !=1);
// 	{

// 		pthread_join(thread_g->pid,NULL);
// 		thread_g++;
// 		printf("sdfsfsdfsfdsfsdfsfds\n");

// 	}


// }

void init_cfs_rq(struct cfs_rq *cfs_run)  //初始化cfs运行队列
{
	cfs=cfs_run;
	cfs->tasks_timeline = RB_ROOT;
	cfs->all_weight =0;
	cfs->clock = 0;
	cfs->min_vruntime = (u64)(-(1LL << 20));
	cfs->nr_running=0; //队列初始化数量
}


int main(void)
{
	pthread_t sched_id;
	int ret;
	int size = 100;
	int n =1000;
	struct cfs_rq cfs_run;
	init_cfs_rq(&cfs_run);
	struct rb_root *cfs_tree= &cfs->tasks_timeline;
	struct thread_struct thread_group[25];


	init_thread(25,thread_group);


	ready_run_thread(thread_group,&n);

	// insert_thread_group(thread_group);
	sleep(1);


	 ret=pthread_create(&sched_id,NULL,(void *)main_thread,NULL); //初始化调度线程
		if(ret!=0){
			printf ("Create pthread error!\n");
			exit (1);
		}

	// printf("all runing thread number is :%d\n",cfs->nr_running);
	// run_id =thread_group[19].pid;
	// printf("id is %lu\n",thread_group[19].pid);
	// pthread_cond_broadcast(&cond);
	// sleep(5);
	// struct sched_entity *that  = rb_entry(cfs->rb_leftmost, struct sched_entity, run_node);
	// printf("bbbbbbbbbbbbbbbbb: %d\n",that->key);
	// wait_thread(thread_group); //等待所有线程执行完成
   pthread_join(sched_id,NULL);

printf("zzzzzzzzzzzzzzzzzzzzz  %lu \n",run_id);
}
