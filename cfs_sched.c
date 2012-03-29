#include "rbtree_rc.h" 
#include "thread_cfs.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#define random(x) (rand()%x)

static pthread_mutex_t mtx = PTHREAD_MUTEX_INITIALIZER ;
static pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
pthread_t run_id=0;

void *thread(int *n){
		int a[3001]={};
		int b[3001]={};
		int c[3001]={};
		c[1]=2;
        b[1]=1;
	for(int i=1;i<=*n-2;i++)
	{   
		pthread_mutex_lock(&mtx);
		while(run_id!=pthread_self())
		{
			//printf("thread %lu wait process \n",pthread_self());
			pthread_cond_wait(&cond, &mtx);
		}
		
		pthread_cond_broadcast(&cond);
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
		usleep(1);
	}  

	int i=3000;
	while(c[i]==0) i--;
	for(i=i;i>=1;i--)
		printf("%d",c[i]);
	printf("thrad is finish%lu\n",pthread_self());
	printf("\n");				


}

void *main_thread(void)//主调度线程
{




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

void ready_run_thread(struct thread_struct *ready_thread,int n) //准备运行线程
{
	pthread_t id;	
	while(ready_thread->pid != 1)
	{		
		int ret=pthread_create(&id,NULL,(void *) thread,&n);
		if(ret!=0){
			printf ("Create pthread error!\n");
			exit (1);
		}
		ready_thread->pid=id;
	//	printf("thread id is: %lu\n",ready_thread->pid);
		ready_thread->se.key = random(10000);
		ready_thread++;
	}
	
}

int insert_rb_tree(struct rb_root *tree_root,struct thread_struct *thread ) //插入单个节点到红黑树
{
		
	struct rb_node **new= &(tree_root->rb_node);
	struct rb_node *parent =0; 

	while(*new)
	{
		struct sched_entity *this = rb_entry(*new, struct sched_entity, run_node);

                int result =thread->se.key - this->key;

                parent = *new;

                if (result < 0)
                        new = &((*new)->rb_left);
                else if (result > 0)
                        new = &((*new)->rb_right);
                else
                        return -1; // the key is already exists

	}
	rb_link_node(&(thread->se.run_node), parent, new);
	rb_insert_color(&(thread->se.run_node),tree_root);

}


void insert_thread_group(struct rb_root *tree,struct thread_struct *thread_g)	
{

	while(thread_g->pid != 1)
	{

		insert_rb_tree(tree,thread_g);
		thread_g++;
	}
}

void wait_thread(struct thread_struct *thread_g)
{

	while(thread_g->pid !=1);
	{
		

		pthread_join(thread_g->pid,NULL);
		thread_g++;
		printf("dffdfd\n");

	}

}
int main(void)
{
	pthread_t id2,id3;
	int ret;
	int size = 100;
	int n =100;
	struct rb_root cfs_tree = RB_ROOT;
	struct thread_struct thread_group[21];
 
	init_thread(21,thread_group);

 	ready_run_thread(thread_group,n);

	insert_thread_group(&cfs_tree,thread_group);
	usleep(1);
	run_id =thread_group[19].pid;
	pthread_cond_broadcast(&cond);
	sleep(2);
	run_id =thread_group[18].pid;
	pthread_cond_broadcast(&cond);	

	sleep(2);
	run_id =thread_group[10].pid;
	pthread_cond_broadcast(&cond);	

    wait_thread(thread_group); //等待所有线程执行完成

	return 0;
}
