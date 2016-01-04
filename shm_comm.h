/**
 * communication between threads by share memory.
 * author: europelee
 * date	 : 20141019
 */

#ifndef SHM_COMM_H
#define SHM_COMM_H

#ifdef _ANDROID
#include <android/log.h>
#endif

#include<stdio.h>
#include<stdint.h>
#include<stdlib.h>
#include<string.h>
#include <errno.h>
#include <pthread.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include<sys/types.h>
#include <sys/mman.h>


#define LOG_TAG   "SHM_COMM"
#define LOG_DEBUG "DEBUG"
#define LOG_TRACE "TRACE"
#define LOG_ERROR "ERROR"
#define LOG_INFO  "INFOR"
#define LOG_CRIT  "CRTCL"

#ifdef __linux__

#define SHM_COMM_LOG(level,format,...) \
    do { \
        fprintf(stdout, "[%s|%s@%s,%d] " format "\n", \
                level, __func__, __FILE__, __LINE__, ##__VA_ARGS__ ); \
    } while (0)

#elif _ANDROID

#define SHM_COMM_LOG(level,...) \
    do { \
        if (0 == strcmp(level, LOG_ERROR)) \
        { \
            __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__); \
        } \
        else \
        { \
            __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__); \
        } \
    } while (0)

#endif

#define  SHM_OPT_SUCC 0
#define  SHM_OPT_FAIL -1

#define  MQ_MIN_SIZE  1024

#define  SHMCC_MAX_CHANNEL_SIZE 10
#define  BRANCH_NEVER_IN  0

#define  STREAM_IN_DIRECT 0
#define  STREAM_OUT_DIRECT 1

#define  MAX_NUM_TRY_RW 200
#define  TIME_SLEEP_RW  50*1000

#define  MAX_CHN_NUM 2

#define is_power_of_2(x) ((x) != 0 && (((x) & ((x) - 1)) == 0))
#define min(a, b) (((a) < (b)) ? (a) : (b))

typedef struct	shm_comm_ctlinfo {
    unsigned char *head_maped;
    unsigned int in_index; //next input index
    unsigned int out_index;
    unsigned int qsize;
}shm_comm_ctlinfo;

typedef struct  chn_comm_ctlinfo {
    shm_comm_ctlinfo  chn_list[MAX_CHN_NUM];
}chn_comm_ctlinfo;


    int init_memqueue(int mqSize, chn_comm_ctlinfo * pCCInfo, int streamDirect);
    int init_shmfile(const char * pShmName, int mqSize, chn_comm_ctlinfo * pCCInfo, int streamDirect);
    int shm_write(chn_comm_ctlinfo * pCCInfo, void * pChr, uint32_t binSize, int streamDirect);
    int shm_read(chn_comm_ctlinfo * pCCInfo, void * pChr, uint32_t binSize, int streamDirect);
    int fini_memqueue(int mqSize, chn_comm_ctlinfo * pCCInfo, int streamDirect);
    int fini_shmfile(int mqSize, chn_comm_ctlinfo * pCCInfo, int streamDirect); 
    int remove_shmfile(const char * pShmName);
    int getBuffLen(shm_comm_ctlinfo * pShmCInfo);

#endif
