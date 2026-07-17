#include "memory_control.h"
#include "app_module.h"

// for memory control
void QueueMutexCreate(QueueWith_t *pQCtrlB)
{
	if((NULL != pQCtrlB)
		&& (pQCtrlB->m_nMutexHandle == 0))
	{
		GxCore_MutexCreate(&(pQCtrlB->m_nMutexHandle));
    }
}

void QueueMutexLock(QueueWith_t QCtrlB)
{
	if(QCtrlB.m_nMutexHandle > 0)
		GxCore_MutexLock(QCtrlB.m_nMutexHandle);
}

void QueueMutexUnlock(QueueWith_t QCtrlB)
{
	if(QCtrlB.m_nMutexHandle > 0)
		GxCore_MutexUnlock(QCtrlB.m_nMutexHandle);
}

void QueueMutexDelete(QueueWith_t QCtrlB)
{
	if(QCtrlB.m_nMutexHandle > 0)
	{
		GxCore_MutexCreate(&QCtrlB.m_nMutexHandle);
		QCtrlB.m_nMutexHandle = 0;
    }
}

void QueueSemCreate(QueueWith_t *pQCtrlB)
{
	if((NULL != pQCtrlB)
		&& (pQCtrlB->m_nSemHandle == 0))
	{
		GxCore_SemCreate(&(pQCtrlB->m_nSemHandle),0);
    }
}

void QueueSemPost(QueueWithHandle_t QHandle)
{
	volatile QueueWith_t *pQ = NULL;

	if(QHandle > 0)
	{
		pQ = (QueueWith_t*)QHandle;
		if(pQ->m_nSemHandle > 0)
			GxCore_SemPost(pQ->m_nSemHandle);
	}
}

void QueueSemWait(QueueWithHandle_t QHandle)
{
	volatile QueueWith_t *pQ = NULL;

	if(QHandle > 0)
	{
		pQ = (QueueWith_t*)QHandle;
		if(pQ->m_nSemHandle > 0)
			GxCore_SemWait(pQ->m_nSemHandle);
	}
}

void QueueSemTryWait(QueueWithHandle_t QHandle)
{
	volatile QueueWith_t *pQ = NULL;

	if(QHandle > 0)
	{
		pQ = (QueueWith_t*)QHandle;
		if(pQ->m_nSemHandle > 0)
			GxCore_SemTryWait(pQ->m_nSemHandle);
	}
}


void QueueSemDelete(QueueWith_t QCtrlB)
{
	if(QCtrlB.m_nSemHandle > 0)
	{
		GxCore_SemDelete(QCtrlB.m_nSemHandle);
		QCtrlB.m_nSemHandle = 0;
    }
}

int QueueSemCount(QueueWith_t QCtrlB)
{
	int Count = 0;
	if(QCtrlB.m_nSemHandle > 0)
	{
		GxCore_SemGetValue(QCtrlB.m_nSemHandle,&Count);
    }
	return Count;
}

static int __QueueWithGet(QueueWithHandle_t QHandle,
					   unsigned char* pCell)
{
		volatile QueueWith_t *pQ = NULL;
		volatile unsigned int nTemp1 = 0;
		unsigned int i = 0;

		if((0 == QHandle) || (NULL == pCell))
		{
			app_log_error("\nMemory Queue, invalid handle...\n");
			return QUEUE_ERROR;
		}

		pQ = (QueueWith_t*)QHandle;

		if(0 >= pQ->m_nUsedCtr)
		{
			pQ->m_nEmptyCtr++;
			//app_log_error("\nMemory Queue, queue is empty...\n");
			return QUEUE_ERROR;
		}

		(pQ->m_nUsedCtr)--;

		nTemp1 = (pQ->m_nFrontAddr) + 8;

		for(i = 0; i < pQ->m_nCellSize; i++)
		{
		   *(pCell + i) = *(volatile unsigned char*)(nTemp1+i);
		}

		pQ->m_nFrontAddr = *(volatile unsigned int*)pQ->m_nFrontAddr;

		pQ->m_nFreeCtr++;

		return QUEUE_SUCCESS;

}

int QueueWithCreate(QueueWith_t *pQCtrlB,
                   QueueWithHandle_t *pQHandle,
                   unsigned int nCellAmount,
                   unsigned int nCellSize
                  )
{
    volatile unsigned int nTemp1 = 0;
    unsigned int i = 0;

	QueueMutexCreate(pQCtrlB);
	QueueSemCreate(pQCtrlB);
    pQCtrlB->m_pFreeList = (unsigned char*)malloc((nCellSize+8)*nCellAmount);
    if(NULL == pQCtrlB->m_pFreeList)
    {
    	app_log_error("\nMemory Queue, malloc failed...\n");
		QueueMutexDelete(*pQCtrlB);
		QueueSemDelete(*pQCtrlB);
        return QUEUE_ERROR;
    }
    for(i = 0; i < (nCellSize+8)*nCellAmount; i++)
    {
        *(pQCtrlB->m_pFreeList + i) = 0xab;
    }


    *pQHandle = (unsigned int)pQCtrlB;

    pQCtrlB->m_nTotalNum = nCellAmount;
    pQCtrlB->m_nFreeCtr  = nCellAmount;
    pQCtrlB->m_nUsedCtr  = 0;
    pQCtrlB->m_nFullCtr  = 0;
    pQCtrlB->m_nEmptyCtr = 0;

    pQCtrlB->m_nCellSize = nCellSize;

    pQCtrlB->m_nFrontAddr = (unsigned int)pQCtrlB->m_pFreeList;
    pQCtrlB->m_nRearAddr  = (unsigned int)pQCtrlB->m_pFreeList;

    nTemp1 = (unsigned int)pQCtrlB->m_pFreeList;
    *(volatile unsigned int*)nTemp1 = (unsigned int)pQCtrlB->m_pFreeList + nCellSize + 8;
    *(volatile unsigned int*)(nTemp1+4) = (unsigned int)pQCtrlB->m_pFreeList + (nCellSize + 8)*(nCellAmount-1);
    for(i = 1; i < nCellAmount - 1; i++)
    {
        nTemp1 += nCellSize + 8;
        *(volatile unsigned int*)nTemp1 = nTemp1 + nCellSize + 8;
        *(volatile unsigned int*)(nTemp1+4) = nTemp1 - (nCellSize + 8);
    }

    nTemp1 += nCellSize + 8;
    *(volatile unsigned int*)nTemp1 = (unsigned int)pQCtrlB->m_pFreeList;
    *(volatile unsigned int*)(nTemp1+4) = nTemp1 - (nCellSize + 8);

    return QUEUE_SUCCESS;
}

int QueueWithDestroy(QueueWithHandle_t QHandle)
{
	volatile QueueWith_t *pQ = NULL;
	QwithCell_t Cell = {0};
	int ret = 0;

	if(QHandle <= 0 )
	{
		app_log_error("\nMemory Queue, invalid handle...\n");
		return QUEUE_ERROR;
	}

	pQ = (QueueWith_t*)QHandle;

	QueueMutexLock(*pQ);
	// TODO:20130715 for queue memory
	while((pQ->m_nUsedCtr) > 0)
	{
		ret = __QueueWithGet(QHandle, (unsigned char*)&Cell);
		if(ret == 0)
		{
			if(0 != Cell.m_nCell)
				free((void*)(Cell.m_nCell));
		}
	}
	// for sem
	while(1)
	{
		if(0 == QueueSemCount(*pQ))
			break;
		QueueSemTryWait(QHandle);
	}
	if(pQ->m_pFreeList)
	{
		free(pQ->m_pFreeList);
		pQ->m_pFreeList = NULL;
	}
	// TODO:
	QueueMutexUnlock(*pQ);
	QueueMutexDelete(*pQ);
	QueueSemDelete(*pQ);
	return QUEUE_SUCCESS;

}

int QueueWithClear(QueueWithHandle_t QHandle)
{
	volatile QueueWith_t *pQ = NULL;
	QwithCell_t Cell = {0};
	int ret = 0;

	if(QHandle <= 0 )
	{
		app_log_error("\nMemory Queue, invalid handle...\n");
		return QUEUE_ERROR;
	}

	pQ = (QueueWith_t*)QHandle;

	QueueMutexLock(*pQ);
	while((pQ->m_nUsedCtr) > 0)
	{
		ret = __QueueWithGet(QHandle, (unsigned char*)&Cell);
		if(ret == 0)
		{
			if(0 != Cell.m_nCell)
				free((void*)(Cell.m_nCell));
		}
	}
	// for sem
	while(1)
	{
		if(0 == QueueSemCount(*pQ))
			break;
		QueueSemTryWait(QHandle);
	}
	QueueMutexUnlock(*pQ);

	return QUEUE_SUCCESS;
}

int QueueWithPut(QueueWithHandle_t QHandle,
                   unsigned char* pCell,
                   unsigned int nLength
                   )
{

    volatile QueueWith_t *pQ = NULL;
    volatile unsigned int nTemp1 = 0;
    unsigned int i = 0;


    if((0 == QHandle) || (NULL == pCell))
    {
    	app_log_error("\nMemory Queue, invalid handle...\n");
        return QUEUE_ERROR;
    }

	pQ = (QueueWith_t*)QHandle;
	QueueMutexLock(*pQ);
    if(0 >= pQ->m_nFreeCtr)
    {
        pQ->m_nFullCtr++;
		//app_log_debug("\nMemory Queue, queue is full...\n");
		QueueMutexUnlock(*pQ);
        return QUEUE_FULL;
    }

    (pQ->m_nFreeCtr)--;

    nTemp1 = (pQ->m_nRearAddr) + 8;

    for(i = 0; i < nLength; i++)
    {
        *(volatile unsigned char*)(nTemp1+i) = *(pCell + i);
    }

    pQ->m_nRearAddr = *(volatile unsigned int*)(pQ->m_nRearAddr);

    pQ->m_nUsedCtr++;

	QueueMutexUnlock(*pQ);
    return QUEUE_SUCCESS;
}


int QueueWithGet(QueueWithHandle_t QHandle,
                   unsigned char* pCell
                   )
{
    volatile QueueWith_t *pQ = NULL;
	int ret = QUEUE_SUCCESS;

    if((0 == QHandle) || (NULL == pCell))
    {
    	app_log_error("\nMemory Queue, invalid handle...\n");
        return QUEUE_ERROR;
    }

	pQ = (QueueWith_t*)QHandle;

	QueueMutexLock(*pQ);

   	ret = __QueueWithGet(QHandle,pCell);

	QueueMutexUnlock(*pQ);
    return ret;

}

// sample
static QueueWith_t         g_QWithCAT			= {0};
static QueueWithHandle_t   g_QWithHandlCAT 	    = 0;
static handle_t 		   g_ThreadCons         = -1;
static handle_t 		   g_ThreadCntrol       = 1;

// step one: initialize the control block module
extern void cat_console(void* arg);

int cat_console_init(void)
{
	int ret = 0;
	ret = QueueWithCreate(&g_QWithCAT, &g_QWithHandlCAT, 2, sizeof(QwithCell_t));
	if(ret < 0)
	{
		app_log_error("\nQueue, CAT alloc failed...\n");
		return -1;
	}
	g_ThreadCntrol = 1;
	GxCore_ThreadCreate("CAT", &g_ThreadCons, cat_console, NULL,8*1024,GXOS_DEFAULT_PRIORITY);
	if(g_ThreadCons <= 0)
	{
		g_ThreadCntrol = 0;
		QueueWithDestroy(g_QWithHandlCAT);
		memset(&g_QWithCAT,0,sizeof(QueueWith_t));
		return -1;
	}
	app_log_min("\nQueue, init successfully...\n");
	return 0;
}

// step two: use the memory control module put data
void cat_data_put(char* pData)
{
	char *p = NULL;
	int len = 0;
	int ret = 0;
	QwithCell_t Cell = {0};

	len = (((pData[1]&0x0f)<<8)|((pData[2]))) + 3;
	p = malloc(len);
	if(p != NULL)
	{
		memcpy(p,pData,len);

		Cell.len = len;
		Cell.m_nCell = (unsigned int)(p);
loop:	ret = QueueWithPut(g_QWithHandlCAT,(unsigned char*)&Cell,sizeof(QwithCell_t));// here only malloc , free need adter get
		if(ret == QUEUE_ERROR)
		{
			free(p);
		}
		else if (ret == QUEUE_FULL)
		{// buffer is full, then clear the old data,
			QueueWithClear(g_QWithHandlCAT);
			goto loop;
		}
		else
		{
			QueueSemPost(g_QWithHandlCAT);
		}
	}
}

// step three: use the memory control module get data
void cat_console(void* arg)
{
	QwithCell_t Cell = {0};
	int ret = 0;
	int len = 0;
	unsigned char *p = NULL;
	while(g_ThreadCntrol)
	{
		QueueSemWait(g_QWithHandlCAT);
		ret = QueueWithGet(g_QWithHandlCAT,(unsigned char*)&Cell);
		if(ret == QUEUE_SUCCESS)
		{
			len = Cell.len;
			p = (unsigned char*)Cell.m_nCell;
			{
					int i = 0;
					if(len > 256)
					{
						app_log_error("\n------CAT DATE ERROR\n");
						len = 255;
					}
					app_log_min("\n***************************************************************\n");
			    	for(i = 0; i < len; i++)
			    	{
						app_log_min("%02x ",p[i]);
						if(((i + 1)%16) == 0)
						app_log_min("\n");
					}
					app_log_min("\n***************************************************************\n");
			}

			// TODO: 20130715
			free(p);//
		}
	}
}

