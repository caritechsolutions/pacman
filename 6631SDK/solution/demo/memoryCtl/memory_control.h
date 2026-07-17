#ifndef __GX_MEMORY_CONTROL_H__
#define __GX_MEMORY_CONTROL_H__

// for memory control
#define QUEUE_ERROR			-1
#define QUEUE_FULL			-2
#define QUEUE_SUCCESS		0
typedef struct QueueWith_s
{
    int m_nTotalNum;
    int m_nFreeCtr;
    int m_nUsedCtr;
    int m_nFullCtr;
    int m_nEmptyCtr;

    unsigned int m_nCellSize;
    unsigned char *m_pFreeList;
    unsigned int m_nFrontAddr;
    unsigned int m_nRearAddr;
	int m_nMutexHandle;
	int m_nSemHandle;
}QueueWith_t;

typedef struct Qwithcell_s
{
	unsigned int len;
	unsigned int m_nCell;
}QwithCell_t;


typedef unsigned int QueueWithHandle_t;


int QueueWithGet(QueueWithHandle_t QHandle, unsigned char* pCell);
int QueueWithPut(QueueWithHandle_t QHandle, unsigned char* pCell, unsigned int nLength);
int QueueWithCreate(QueueWith_t *pQCtrlB, QueueWithHandle_t *pQHandle, unsigned int nCellAmount,unsigned int nCellSize);
int QueueWithDestroy(QueueWithHandle_t QHandle);
int QueueWithClear(QueueWithHandle_t QHandle);
void QueueSemPost(QueueWithHandle_t QHandle);
void QueueSemWait(QueueWithHandle_t QHandle);
#endif

