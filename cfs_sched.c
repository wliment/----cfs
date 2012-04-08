#include "rbtree_rc.h" 
#include "thread_cfs.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
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

#define MAX_USER_RT_PRIO	100  //最大用户实时优先级
#define MAX_RT_PRIO		MAX_USER_RT_PRIO
#define NICE_0_LOAD		1024
#define NICE_TO_PRIO(nice)	(MAX_RT_PRIO + (nice) + 20)
#define PRIO_TO_NICE(prio)	((prio) - MAX_RT_PRIO - 20)
#define TASK_NICE(p)		PRIO_TO_NICE((p)->static_prio)

static pthread_mutex_t mtx = PTHREAD_MUTEX_INITIALIZER ;
static pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
pthread_t run_id=0;
struct cfs_rq *cfs; //等待进程队列

void dequeue_entity( struct sched_entity *se);
void enqueue_entity(struct sched_entity *se) ;

void *thread(int *n){
		int a[3001]={};
		int b[3001]={};
		int c[3001]={};
		c[1]=2;
        b[1]=1;
		pthread_t self_id=pthread_self();       
	for(int i=1;i<=(*n-2);i++)
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
	printf("\nthread is finish: id is %lu    n is:%d \n",pthread_self(),*n);
	// cfs->nr_running -=1;
	printf("the left thread is: %d \n",(int)cfs->nr_running);


}

#define TIME_TICK 1000  //模拟多少时间，触发一次时钟中断,单位为微秒us
int pthread_kill(pthread_t thread, int sig);
int thread_is_alive(pthread_t pid) //判断线程是否死掉
{

	 int singal;
	 singal =  pthread_kill(pid,0); 
	 return singal == ESRCH ? 0:1;

}



void *main_thread()//主调度线程
{

	struct sched_entity *curr  = rb_entry(cfs->rb_leftmost, struct sched_entity, run_node);
	struct sched_entity *left_node = curr;
	dequeue_entity(curr); //出队运行
	while(1)
	{

		left_node  = rb_entry(cfs->rb_leftmost, struct sched_entity, run_node);
		if(left_node == 0)
		{
				left_node  = rb_entry(cfs->tasks_timeline.rb_node, struct sched_entity, run_node);
		}
		if(cfs->tasks_timeline.rb_node == 0)
			break;

		curr->key +=10;
		// printf("mmmmmmmmmmmmmm:%d  %lu %d\n",cfs->nr_running,curr->pid,left_node);
	
		if(left_node->key < curr->key)
		{
			if(thread_is_alive(curr->pid) )
			{
				enqueue_entity(curr); //如果进程退出，不用进队
				// printf("ppppppppppppppppppppppp\n");
				//清空curr指向的内存空间
			}	
			curr = left_node;
			// printf("vvvvvvvvvvvvvvvvvvvv: cfs->nr_running: %d %lu \n",cfs->nr_running,curr->pid);
			dequeue_entity(curr);

		}

				
		
		run_id =curr->pid;
		pthread_cond_broadcast(&cond);
		// pthread_cond_signal(&cond);

		usleep(TIME_TICK*2);//模拟每隔1ms触发一次时钟中断
	}
	// curr = rb_entry(&cfs->tasks_timeline.rb_node, struct sched_entity, run_node);

	pthread_join(curr->pid,NULL);
printf("ddddddddddddddddddddd  %lu\n",run_id);
	// run_id = curr->pid;
	//  pthread_cond_broadcast(&cond);
	
	
}



 void inc_nr_running()
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




//初始化线程调度实体信息
void init_thread_info(struct thread_struct *ready_thread,pthread_t id)
{
	ready_thread->pid = id;
	ready_thread->se.is_exit = 0;
	ready_thread->se.pid = id;
	ready_thread->se.key=10000+random(3000);
}


void ready_run_thread(struct thread_struct *ready_thread,int *n) //准备运行线程
{
	pthread_t id;	
	while(ready_thread->pid != 1)
	{	

		int ret=pthread_create(&id,NULL,(void *)thread,n);
		if(ret!=0){
			printf ("Create pthread error!\n");
			exit (1);
		}
		init_thread_info(ready_thread,id);	
		ready_thread++;

	}
	
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




//将调度实体插入红黑树，在插入过程中缓存最左边的节点
void enqueue_entity(struct sched_entity *se ) //插入单个节点到红黑树
{
	struct rb_node **new= &(cfs->tasks_timeline.rb_node);
	struct rb_node *parent =0;
	struct sched_entity *this;
	int leftmost = 1;

	while(*new)
	{
		parent = *new;
		this = rb_entry(*new, struct sched_entity, run_node);

		int result =se->key - this->key;
		

		if (result < 0){
			new = &parent->rb_left;
		}
		else 
		{
			new = &parent->rb_right;
			leftmost = 0; 
		}

// printf("hhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhh\n");
	}

	if (leftmost)
		cfs->rb_leftmost = &this->run_node;

	rb_link_node(&(se->run_node), parent, new);
	rb_insert_color(&(se->run_node),&cfs->tasks_timeline);
	inc_nr_running();
}


void insert_thread_group(struct thread_struct *thread_g)	
{

	while(thread_g->pid != 1)
	{

		enqueue_entity(&thread_g->se);
		thread_g++;
		//printf('dsfdsfsdfdsfdsfdssssslsm  \n');
	}
}

void wait_thread(struct thread_struct *thread_g)
{
	while(thread_g->pid !=1);
	{

		pthread_join(thread_g->pid,NULL);
		thread_g++;
		printf("sdfsfsdfsfdsfsdfsfds\n");

	}


}

void init_cfs_rq()  
{


}


int main(void)
{
	pthread_t sched_id;
	int ret;
	int size = 100;
	int n =1000;
	struct cfs_rq cfs_run;
	cfs=&cfs_run;
	cfs->tasks_timeline = RB_ROOT;
	struct rb_root *cfs_tree= &cfs->tasks_timeline;
	struct thread_struct thread_group[25];

	cfs->nr_running=0; //队列初始化数量

	init_thread(25,thread_group);


	ready_run_thread(thread_group,&n);

	insert_thread_group(thread_group);

// sleep(1);

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
// sleep(10);
}
