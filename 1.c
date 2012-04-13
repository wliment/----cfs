#include <stdio.h>
#include <stdlib.h>
#include <string.h>
// int main()
// {

// 	// long long *b=(long long *)malloc(sizeof(long long )*9999*9999*999);
// 	// sleep(30);
// 	// exit(0);
// 	int a=0,n=96,m=0;
// 	while(1)
// 	{
// 		printf("please input you number:");
// 		scanf("%d",&a);
// 		m = n+a;	
// 		printf("the char is : %c\n",m);


// 	}


// }
void GetMemory(char **p,int num)
{
*p=(char*)malloc(sizeof(char)*num);       //p是形参指向的地址
}
void main()
{
char *str=NULL;
GetMemory(&str,100);                            //str是实参指向的地址，不能通过调用函数来申请内存
strcpy(str,"hello");
printf("sssssssssss %s %d \n",str,1 << 10);
}