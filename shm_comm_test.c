
/**
  * for testing shm_comm	
*/

#include <pthread.h>
#include "shm_comm.h"
#include <signal.h>
#include <unistd.h>

#define false      0
#define true        1

typedef struct test_info
{   
	uint32_t a;    
	uint32_t b;    
	uint32_t c;
	uint32_t d;    
	uint32_t e;
	uint32_t f;    
	uint32_t g;
}test_info;


static chn_comm_ctlinfo  g_chn_test;
static volatile sig_atomic_t s_interrupt = false;

void get_test_info(test_info* pInfo, uint32_t var)
{
   	
	if (!pInfo)   
	{   
		fprintf(stderr, "pInfo is NULL.\n");    
		return ;    
	}   

	pInfo->a = var;
	pInfo->b = var+1;
	pInfo->c = var+2;
	pInfo->d = var+3;
	pInfo->e = var+4;
	pInfo->f = var+5;
	pInfo->g = var+6;
	
}

void print_test_info(test_info* pInfo)
{
	if (!pInfo)   
	{   
		fprintf(stderr, "pInfo is NULL.\n");    
		return ;    
	}  

	printf("%u %u %u %u %u %u %u\n", pInfo->a, pInfo->b, pInfo->c, pInfo->d, pInfo->e, pInfo->f,
			pInfo->g);
	
}

void * writer_proc(void *arg)
{    
	uint32_t var = 0;
	test_info test_info;
	while(!s_interrupt)    
	{   
   		
		var = var % 10;   
		printf("******************************************\n");    
		get_test_info(&test_info, var);    
		printf("put a t_info info to  buffer.\n");
		int ret = shm_write(&g_chn_test,(void *)&test_info, sizeof(test_info), STREAM_IN_DIRECT);
		if (SHM_OPT_FAIL == ret) 
		{
			printf("shm_write fail\n");
			return NULL;
		}
		int len = getBuffLen(&(g_chn_test.chn_list[STREAM_IN_DIRECT]));
		printf("buff length: %u\n",len);   
		printf("******************************************\n");   
		sleep(2);
		var++;
	}    
	return (void *)NULL;
}



int writer_thread(pthread_t *theadid, void *arg)
{    
	int ret = 0;    
	    
	ret = pthread_create(theadid, NULL, writer_proc, arg);    
	if ( 0 != ret)    
	{    
		fprintf(stderr, "Failed to create writer thread, errno:%u, reason:%s\n", errno, strerror(errno));    
		    
	}    
	return ret;
}

void * reader_proc(void *arg)
{
	
	test_info test_info; 
	while(!s_interrupt)
	{
		sleep(1);
		printf("------------------------------------------\n");
		printf("get t_info from buffer.\n");
		int ret = shm_read(&g_chn_test,(void *)&test_info, sizeof(test_info), STREAM_IN_DIRECT);
		if (SHM_OPT_FAIL == ret) 
		{
			printf("shm_read fail\n");
			return NULL;
		}
		int len = getBuffLen(&(g_chn_test.chn_list[STREAM_IN_DIRECT]));
		printf("buff length: %u\n",len);
		print_test_info(&test_info);
		printf("------------------------------------------\n");
	}
	return (void *)NULL;
}

int reader_thread(pthread_t * threadid, void *arg)
{
   int ret;
  
   ret = pthread_create(threadid, NULL, reader_proc, arg);
   if (0 != ret)
   {
	   fprintf(stderr, "Failed to create reader thread.errno:%u, reason:%s\n",
		   errno, strerror(errno));
	  
   }
   return ret;
}

static void SigIntHandler(int sig)
{
	s_interrupt = true;
}

int main(int argc, char ** argv)
{
    signal(SIGINT, SigIntHandler);
	
	int ret = 0;
	ret = init_memqueue(1024*1024, &g_chn_test, STREAM_IN_DIRECT);
	if (0 != ret)
	{
		return -1;
	}


	pthread_t wid;
	writer_thread(&wid, NULL);
	pthread_t rid;
	reader_thread(&rid, NULL);

	printf("ctrl+c now!\n");
	
	pthread_join(wid, NULL);
	pthread_join(rid, NULL);
	
	fini_memqueue(1024*1024, &g_chn_test, STREAM_IN_DIRECT);
	return 0;
}
