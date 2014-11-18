/**
 * communication between threads by share memory.
 * author: europelee
 * date	 : 20141019
 */

#include "shm_comm.h"

/**
 * just support one conn at the same time now
 * it should be better implementing a communication-lib like tbus at a future
 */


/**
* shm_comm i/o msging learn from kfifo in linux kernel code
* but struct shm_comm_ctlinfo does not have a spinlock_t lock
* it means that shm_comm  could not support more than one reader or writer,
* shm_comm just support one reader and one writer.
* you also can get proof from linux kernel code comments like kfifo.c
* such below :

* __kfifo_put - puts some data into the FIFO, no locking version
* @fifo: the fifo to be used.
* @buffer: the data to be added.
* @len: the length of the data to be added.
*
* This function copies at most @len bytes from the @buffer into
* the FIFO depending on the free space, and returns the number of
* bytes copied.
*
* Note that with only one concurrent reader and one concurrent
* writer, you don't need extra locking to use these functions.
*/


#if BRANCH_NEVER_IN
static chn_comm_ctlinfo  g_chn_netlayer;  // alljoyn i/o
static chn_comm_ctlinfo  g_chn_chatlayer; // msgcenter chat i/o
static chn_comm_ctlinfo  g_chn_gamelayer; // msgcenter game i/o
static chn_comm_ctlinfo  g_chn_mmlayer; // msgcenter multimedia i/o
#endif

static int	 checkMQHead(shm_comm_ctlinfo * pShmCInfo) {
	
	return NULL == pShmCInfo->head_maped?SHM_OPT_FAIL:SHM_OPT_SUCC;
}

static void  setCCInfo(unsigned char *head_maped, chn_comm_ctlinfo * pCCInfo, int streamDirect, int mqSize) {

	if ( STREAM_IN_DIRECT == streamDirect) {
	pCCInfo->in_chn.head_maped = head_maped;
	pCCInfo->in_chn.qsize = mqSize;
	pCCInfo->in_chn.in_index = 0;
	pCCInfo->in_chn.out_index = 0;
	}

	if ( STREAM_OUT_DIRECT == streamDirect) {
	pCCInfo->out_chn.head_maped = head_maped;
	pCCInfo->out_chn.qsize = mqSize;
	pCCInfo->out_chn.in_index = 0;
	pCCInfo->out_chn.out_index = 0;
	}

}


static uint32_t buffer_write(shm_comm_ctlinfo * pShmCInfo, void *buffer, uint32_t size) {
	
    uint32_t len = 0;    
	size = min(size, pShmCInfo->qsize - pShmCInfo->in_index + pShmCInfo->out_index);
	/* first put the data starting from fifo->in to buffer end */    
	len  = min(size, pShmCInfo->qsize - (pShmCInfo->in_index & (pShmCInfo->qsize - 1)));    
	memcpy(pShmCInfo->head_maped + (pShmCInfo->in_index & (pShmCInfo->qsize - 1)), buffer, len);   
	/* then put the rest (if any) at the beginning of the buffer */    
	memcpy(pShmCInfo->head_maped, buffer + len, size - len);   
	pShmCInfo->in_index += size;    
	return size;


}


static uint32_t buffer_read(shm_comm_ctlinfo * pShmCInfo, void *buffer, uint32_t size) {
	
    uint32_t len = 0;    
	size  = min(size, pShmCInfo->in_index - pShmCInfo->out_index);            
	/* first get the data from fifo->out until the end of the buffer */    
	len = min(size, pShmCInfo->qsize - (pShmCInfo->out_index & (pShmCInfo->qsize - 1)));    
	memcpy(buffer, pShmCInfo->head_maped + (pShmCInfo->out_index & (pShmCInfo->qsize - 1)), len);   
	/* then get the rest (if any) from the beginning of the buffer */    
	memcpy(buffer + len, pShmCInfo->head_maped, size - len);    
	pShmCInfo->out_index += size;    
	return size;	
}

int	init_memqueue(int mqSize, chn_comm_ctlinfo * pCCInfo, int streamDirect) {

	int ret = SHM_OPT_SUCC;

	//below 4 if branches code same as init_shmfile, need elimating it
	if (MQ_MIN_SIZE > mqSize) {
		SHM_COMM_LOG(LOG_ERROR, "mqsize is too small, minsize:%d", MQ_MIN_SIZE);
		return SHM_OPT_FAIL;
	}

	if (0 == is_power_of_2(mqSize)) {
		SHM_COMM_LOG(LOG_ERROR, "mqSize must be power of 2");
		return SHM_OPT_FAIL;
	}

	if (NULL == pCCInfo) {
		SHM_COMM_LOG(LOG_ERROR, "pCCInfo is null");
		return SHM_OPT_FAIL;
	}

	if (streamDirect != STREAM_IN_DIRECT && streamDirect != STREAM_OUT_DIRECT) {
		SHM_COMM_LOG(LOG_ERROR, "streamDirect invalid");
		return SHM_OPT_FAIL;
	}

	unsigned char *head_maped =  (unsigned char *)malloc(mqSize);
	if (NULL == head_maped) {
		SHM_COMM_LOG(LOG_ERROR, "malloc %d bytes to head_maped fail!", mqSize);
		return SHM_OPT_FAIL;
	}
	
	setCCInfo(head_maped, pCCInfo, streamDirect, mqSize);

	return ret;
	
}

int init_shmfile(const char * pShmName, int mqSize, chn_comm_ctlinfo * pCCInfo, int streamDirect) {
	
	if (NULL == pShmName) {
		SHM_COMM_LOG(LOG_ERROR, "shmfile name is null");
		return SHM_OPT_FAIL;
	}

	if (MQ_MIN_SIZE > mqSize) {
		SHM_COMM_LOG(LOG_ERROR, "mqsize is too small, minsize:%d", MQ_MIN_SIZE);
		return SHM_OPT_FAIL;
	}

	if (0 == is_power_of_2(mqSize)) {
		SHM_COMM_LOG(LOG_ERROR, "mqSize must be power of 2");
		return SHM_OPT_FAIL;
	}

	if (NULL == pCCInfo) {
		SHM_COMM_LOG(LOG_ERROR, "pCCInfo is null");
		return SHM_OPT_FAIL;
	}

	if (streamDirect != STREAM_IN_DIRECT && streamDirect != STREAM_OUT_DIRECT) {
		SHM_COMM_LOG(LOG_ERROR, "streamDirect invalid");
		return SHM_OPT_FAIL;
	}

	int fd = -1;
	//O_EXCL for exclude case: another process/thread still open and do some job
	if ((fd = open(pShmName, O_RDWR | O_CREAT | O_EXCL)) == -1) {
		if (errno != EEXIST) {
			SHM_COMM_LOG(LOG_ERROR, "open fail: %d", errno);
			return SHM_OPT_FAIL;
		} else {
			SHM_COMM_LOG(LOG_INFO, "file %s exist!", pShmName);
			int ret = remove(pShmName);
			if (0 != ret) {
				SHM_COMM_LOG(LOG_ERROR, "remove %s fail, errno:%d",
						pShmName, errno);
				return SHM_OPT_FAIL;
			} else {
				fd = open(pShmName, O_RDWR | O_CREAT | O_EXCL);
				if (-1 == fd) {
					SHM_COMM_LOG(LOG_ERROR, "open fail again!: %d ", errno);
					return SHM_OPT_FAIL;
				}
			}
		}
	}

	if (ftruncate(fd, mqSize) == -1) {
		SHM_COMM_LOG(LOG_ERROR, "error truncate the file: %d", errno);
		close(fd);
		return SHM_OPT_FAIL;
	}

	unsigned char *head_maped = mmap(0, mqSize, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);

	if (head_maped == MAP_FAILED || head_maped == NULL) {
		SHM_COMM_LOG(LOG_ERROR, "error mmap SHM_FILE: %d", errno);
		perror("mmap");
		close(fd);
		return SHM_OPT_FAIL;
	} else {
		SHM_COMM_LOG(LOG_INFO, "mmap succ!");
	}

	setCCInfo(head_maped, pCCInfo, streamDirect, mqSize);

	return SHM_OPT_SUCC;
}

int  shm_write(chn_comm_ctlinfo * pCCInfo, void * pChr, int binSize, int streamDirect) {

	int i4Ret = SHM_OPT_SUCC;
	if (NULL == pCCInfo || NULL == pChr) {
		SHM_COMM_LOG(LOG_ERROR, "NULL == pCCInfo || NULL == pChr");
		return SHM_OPT_FAIL;
	}

	switch(streamDirect) {
	case STREAM_IN_DIRECT:
		if (SHM_OPT_FAIL == checkMQHead(&(pCCInfo->in_chn))) {
			i4Ret = SHM_OPT_FAIL;
		}
		else {
			int nLoop = MAX_NUM_TRY_RW;
			uint32_t wLen = buffer_write(&(pCCInfo->in_chn), pChr, binSize);
			while (wLen != binSize)
			{
				if (nLoop < 0)
				{
					SHM_COMM_LOG(LOG_ERROR, "try out, wLen != binSize");
					i4Ret = SHM_OPT_FAIL;					
					break;
				}
				--nLoop;
				SHM_COMM_LOG(LOG_INFO, "try write again!");
				usleep(TIME_SLEEP_RW);
				uint32_t cLen = buffer_write(&(pCCInfo->in_chn), pChr+wLen, binSize-wLen);
				wLen += cLen;
			}
			
		}
		break;

	case STREAM_OUT_DIRECT:

		break;

	default:
		i4Ret = SHM_OPT_FAIL;
		break;
	}

	
		return i4Ret;
	
}

int  shm_read(chn_comm_ctlinfo * pCCInfo, void * pChr, int binSize, int streamDirect) {

	int i4Ret = SHM_OPT_SUCC;
	if (NULL == pCCInfo || NULL == pChr) {
		SHM_COMM_LOG(LOG_ERROR, "NULL == pCCInfo || NULL == pChr");
		return SHM_OPT_FAIL;
	}

	switch(streamDirect) {
		
	case STREAM_IN_DIRECT:
		if (SHM_OPT_FAIL == checkMQHead(&(pCCInfo->in_chn))) {
			i4Ret = SHM_OPT_FAIL;
		}
		else {
			int nLoop = MAX_NUM_TRY_RW;
			uint32_t rLen = buffer_read(&(pCCInfo->in_chn), pChr, binSize);
			while (rLen != binSize)
			{
				if (nLoop < 0)
				{
					SHM_COMM_LOG(LOG_ERROR, "try out, rLen != binSize");
					i4Ret = SHM_OPT_FAIL;					
					break;
				}
				--nLoop;
				SHM_COMM_LOG(LOG_INFO, "try read again!");
				usleep(TIME_SLEEP_RW);
				uint32_t cLen = buffer_read(&(pCCInfo->in_chn), pChr+rLen, binSize-rLen);
				rLen += cLen;
			}
			
		}

		break;

	case STREAM_OUT_DIRECT:

		break;

	default:

		break;
	}

	return i4Ret;
}

int  fini_memqueue(int mqSize, chn_comm_ctlinfo * pCCInfo, int streamDirect) {

	int i4Ret = SHM_OPT_SUCC;
	
	if (NULL == pCCInfo) {
		SHM_COMM_LOG(LOG_ERROR, "NULL == pCCInfo");
		return SHM_OPT_FAIL;
	}

	if ( STREAM_IN_DIRECT == streamDirect) {

		if (NULL != pCCInfo->in_chn.head_maped) {
			free(pCCInfo->in_chn.head_maped);		
			pCCInfo->in_chn.head_maped = NULL;
		}

	}	


	if ( STREAM_OUT_DIRECT == streamDirect) {

		if (NULL != pCCInfo->out_chn.head_maped) {
			free(pCCInfo->out_chn.head_maped);		
			pCCInfo->out_chn.head_maped = NULL;
		}

	}


	return i4Ret;

}

int  fini_shmfile(int mqSize, chn_comm_ctlinfo * pCCInfo, int streamDirect) {

	int i4Ret = SHM_OPT_SUCC;
	
	if (NULL == pCCInfo) {
		SHM_COMM_LOG(LOG_ERROR, "NULL == pCCInfo");
		return SHM_OPT_FAIL;
	}
	
	if ( STREAM_IN_DIRECT == streamDirect) {

		if ((NULL != pCCInfo->in_chn.head_maped)
				&& ((munmap((void *) pCCInfo->in_chn.head_maped, mqSize)) == -1)) {
			SHM_COMM_LOG(LOG_ERROR,"error munmap SHM_FILE: %d", errno);
			perror("munmap");
			return SHM_OPT_FAIL;
		}

		pCCInfo->in_chn.head_maped = NULL;

	}

	if ( STREAM_OUT_DIRECT == streamDirect) {
		if ((NULL != pCCInfo->out_chn.head_maped)
				&& ((munmap((void *) pCCInfo->out_chn.head_maped, mqSize)) == -1)) {
			SHM_COMM_LOG(LOG_ERROR,"error munmap SHM_FILE: %d", errno);
			perror("munmap");
			return SHM_OPT_FAIL;
		}

		pCCInfo->out_chn.head_maped = NULL;

	}	

	return i4Ret;
}


int remove_shmfile(const char * pShmName) {

	if (NULL == pShmName) {
		SHM_COMM_LOG(LOG_ERROR, "NULL == pShmName");
		return SHM_OPT_FAIL;
	}
	
	int i4Ret = SHM_OPT_SUCC;
	
	if (0 == access(pShmName, F_OK)) {
		int ret = remove(pShmName);
		if (0 != ret) {
			SHM_COMM_LOG(LOG_ERROR,	"remove %s fail, errno:%d", pShmName, errno);
			i4Ret = SHM_OPT_FAIL;
		}
	} else {
		SHM_COMM_LOG(LOG_ERROR, "%s not exist!", pShmName);
		i4Ret = SHM_OPT_SUCC;
	}

	return i4Ret;

}


 int   getBuffLen(shm_comm_ctlinfo * pShmCInfo) {

	return pShmCInfo->in_index - pShmCInfo->out_index;
}

