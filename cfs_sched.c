//#include "thread_cfs.h"
#include "rbtree_rc.h" //红黑树实现
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>


static pthread_mutex_t mtx = PTHREAD_MUTEX_INITIALIZER ;
static pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
pthread_t run_id=0;//可运行线程的id;

//运行实体,用线程模拟进程,计算菲波那契数列
void thread(void){
		int a[3001]={};
		int b[3001]={};
		int c[3001]={};
		int n=10000;
		c[1]=2;
        b[1]=1;

	for(int i=1;i<=n-2;i++)
	{   
		pthread_mutex_lock(&mtx);
		while(run_id!=pthread_self())
		{
			printf("thread 1 wait process \n");
			pthread_cond_wait(&cond, &mtx);
		}

		pthread_cond_broadcast(&cond);
		pthread_mutex_unlock(&mtx);
		for(int j=1;j<=3000;j++)
		{   
			a[j]=b[j];
			b[j]=c[j];
			c[j]=0;}
			for(int j=1;j<=3000;j++)
			{c[j]=c[j]+a[j]+b[j];
				if(c[j]>=10)
				{c[j]=c[j]-10;
					c[j+1]=1;}
			}   
			usleep(1);
	}  

	int i=3000;
	while(c[i]==0) i--;
	for(i=i;i>=1;i--)
		printf("%d",c[i]);
	printf("\n");				


}

pthread_t thread1_id;
void thread_test(void)
{
	for(int i=0;i<10;i++)
	{

		pthread_mutex_lock(&mtx);
		while(run_id!=pthread_self())
		{
			printf("thread test  wait process \n");
			pthread_cond_wait(&cond, &mtx);
		}

		pthread_cond_broadcast(&cond);
		pthread_mutex_unlock(&mtx);
		printf("thread_test is working\n");
		sleep(1);

	}
	run_id = thread1_id;
	pthread_cond_broadcast(&cond);
}
int main(void)
{
	pthread_t id2,id3;
	int ret;
	int size = 100;
	struct rb_root cfs_tree = RB_ROOT;
	struct container * pc;
	pc = calloc(sizeof(struct container) + size, 1);
	strcat((char*)&pc->rb_data.data, "cfs better than o(1)");

	ret=pthread_create(&id2,NULL,(void *) thread,NULL);
	thread1_id=id2;
	pc->rb_data.key=id2;

	ret=container_insert(&cfs_tree,pc);
	if(ret!=0){
		printf ("Create pthread error!\n");
		exit (1);
	}

	pc = calloc(sizeof(struct container) + size, 1);
	strcat((char*)&pc->rb_data.data, "cfs better than o(2)");

	ret=pthread_create(&id3,NULL,(void *) thread_test,NULL);
	pc->rb_data.key=id3;
	ret=container_insert(&cfs_tree,pc);
	if(ret!=0){
		printf ("Create pthread error!\n");
		exit (1);
	}
	run_id=id3;
	pthread_join(id2,NULL);
	pthread_join(id3,NULL);

	return 0;
}
