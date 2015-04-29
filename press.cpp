#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/tcp.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include "config.h"
#include <sys/time.h>

#include <iostream>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <arpa/inet.h>

#include <errno.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/wait.h>

#include <unistd.h>
#include <fcntl.h>
#include <string>
#include <iostream>

#define BUFFER_MAX_SIZE 10000
#define CONF_FILE "./conf/press.conf"
#define PRESS_MON_INTERVAL 5

pthread_mutex_t g_cond_mutex;
pthread_mutex_t g_counter_mutex;
pthread_cond_t g_data_cond;

/**以下变量临时记录从配置文件读的参数值**/
char *ip; //被压测的服务器ip
char *port_s; //被压测的服务器端口
char *timeout_s; //超时时间
char *press_high_s; //压测三角波的最高频率
char *press_low_s; //压测三角波的最低频率
char *interval_s; //压测三角波的周期，以秒为单位
char *thread_num_s; //并发压测线程数
struct conf_info clist[] = {
	{"ip", &ip},
	{"port", &port_s},
	{"timeout", &timeout_s},
	{"press_high", &press_high_s},
	{"press_low", &press_low_s},
	{"interval", &interval_s},
	{"thread_num", &thread_num_s},
};
/***************************************/

/**以下参数是在压测逻辑中使用的， **/
/**他们的值由 clist[]结构体得到， **/
/**但是ip直接使用前面的char *ip。 **/
int port = 8080; //被压测的服务器端口
int press_high = 10; //压测三角波的最高频率
int press_low = 10; //压测三角波的最低频率
int interval = 20; //压测三角波的周期，以秒为单位
int thread_num = 2; //并发压测线程数
int timeout = 1000; //超时时间
/***********************************/

int g_needsend;

//负责压测的线程函数
void * press_thread(void * param)
{
	int pid = *(int*)param;
	signal(SIGPIPE, SIG_IGN);
	int needsend = 0;
	
	/************************************************/
	/**********压测用的变量声明在此处！**************/
	/************************************************/
	char sendBuf[BUFFER_MAX_SIZE]; //存储发送socket的内容
	char recvBuf[BUFFER_MAX_SIZE]; //存储接收socket的内容
	
	srand((unsigned)time(NULL));
	
	pthread_mutex_lock(&g_cond_mutex);
	pthread_cond_wait(&g_data_cond, &g_cond_mutex);
	pthread_mutex_unlock(&g_cond_mutex);
	
	while (1)
	{
		
		/************************************************/
		/*************压测逻辑写在此处！*****************/
		/************************************************/
		//常见的压测过程如下：
		//struct sockaddr_in serv_addr;
		//Step 1： connfd=socket(AF_INET, SOCK_STREAM, 0) 创建socket
		//Step 2： bzero(&serv_addr, sizeof(serv_addr);
		//         serv_addr.sin_family  = AF_INET;
		//         serv_addr.sin_port    = htons(port);
		//         serv_addr.sin_addr.s_addr=inet_addr(ip);
		//Step 3:  connect(connfd, (struct sockaddr *)&serv_addr, sizeof(struct sockaddr))建立连接
		//Step 4:  memset(sendBuf, 0, sizeof(sendBuf);
		//         snprintf(sendBuf, "Send Message");
		//         send(connfd, sendBuf, length, 0); 发送数据
		//Step 5:  recv(connfd, recvBuf, BUFFER_MAX_SIZE, 0); //接收数据
		//Step 6:  校验数据
		/************************************************/
		pthread_mutex_lock(&g_counter_mutex);
		needsend = --g_needsend;
		pthread_mutex_unlock(&g_counter_mutex);
		if (needsend < thread_num) //注意这里压测计数，一秒之内够了就停
		{
			pthread_mutex_lock(&g_cond_mutex);
			pthread_cond_wait(&g_data_cond, &g_cond_mutex);
			pthread_mutex_unlock(&g_cond_mutex);
		}
	}
	return NULL;
}

//负责数据统计与压测频率控制的函数
void * control_thread(void * param)
{
	time_t t = 0;
	u_int sec = 0;
	u_int f = press_low;
	u_int counter = 0;
	while(1)
	{
		pthread_mutex_lock(&g_counter_mutex);
		g_needsend = f;
		pthread_mutex_unlock(&g_counter_mutex);
		
		pthread_cond_broadcast(&g_data_cond);
		sleep(1);
		sec ++;
		counter += (f - g_needsend);
		if (sec >= PRESS_MON_INTERVAL)
		{
			//每间隔一段时间，输出压测结果
			printf("[press_control] velocity expected last %d sec: %d/s\n", PRESS_MON_INTERVAL, f);
			printf("[press_control] press times last %d sec: %d (%d/s)\n", PRESS_MON_INTERVAL, counter, counter/PRESS_MON_INTERVAL);
			counter = 0;
			sec = 0;
			
			//调节压测频率，以方波或近似三角波进行压测
			t = (t + PRESS_MON_INTERVAL) % interval;
			if (t*2 < (int)interval)
			{
				f = ((press_high - press_low) * t * 2)/interval + press_low;
			} else {
				f = ((press_high - press_low) * (interval - t) * 2)/interval + press_low;
			}
		}
	}
	
	return NULL;
}

//参数初始化函数
int init()
{
	pthread_cond_init(&g_data_cond, NULL);
	pthread_mutex_init(&g_cond_mutex, NULL);
	pthread_mutex_init(&g_counter_mutex, NULL);
	
	//parser config file
	FILE *fp = NULL;
	if ( ( fp = fopen(CONF_FILE, "r") ) == NULL )
	{
		fprintf(stderr, "Can't open conf file %s .\n", CONF_FILE);
		exit(1);
	}
	parse(fp); //解析配置文件
	port = atoi(port_s);
	timeout = atoi(timeout_s);
	press_high = atoi(press_high_s);
	press_low = atoi(press_low_s);
	interval = atoi(interval_s);
	thread_num = atoi(thread_num_s);
	
	return 0;
}

int main(int argc, char *argv[])
{
	if (init() != 0)
	{
		printf("init failed!\n");
		return -1;
	}
	signal(SIGPIPE, SIG_IGN);
	pthread_t pid_press[1000];
	pthread_t pid_control;
	
	if(thread_num <= 0 || thread_num > 1000)
	{
		printf("thread number cannot be larger than 1000\n");
		return -1;
	}
	
	//创建压测线程
	for (int i = 0; i < thread_num; i++)
	{
		pthread_create(&pid_press[i], NULL, press_thread, &i);
	}
	
	//创建控制线程
	pthread_attr_t control_attr;
	pthread_attr_init(&control_attr);
	pthread_attr_setdetachstate(&control_attr, PTHREAD_CREATE_DETACHED);
	pthread_create(&pid_control, &control_attr, control_thread, NULL);
	
	for (int i = 0; i < thread_num; i++)
	{
		pthread_join(pid_press[i], NULL);
	}
	
	return 0;
}
