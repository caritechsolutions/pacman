#ifndef __HDA_BLIT_H__
	#define __HDA_BLIT_H__

#ifndef MAX_BLIT_COUNT
	#define MAX_BLIT_COUNT			(300)
#endif

typedef struct _HD_DrawNode
{
	void *surface;
	struct _HD_DrawNode *next;
}HD_DrawNode;

typedef struct _HD_COMMIT_JOB
{
	int total_num;
	HD_DrawNode *list_head;
	HD_DrawNode *list_tail;
}HD_COMMIT_JOB;

#endif

