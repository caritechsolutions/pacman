#ifndef NO_OS
#include <stdio.h>
#include <stdlib.h>
#include <gxcore.h>
#include "gui_core.h"
#else
#include "common/gxmalloc.h"
#include "mini_gui_private.h"
#endif

#include <string.h>
#include "linklist.h"

quefr_t *linklist_new(quefr_t *qe)
{
	quenode_t *h = NULL;

	if(NULL == qe)
	{
		qe = (quefr_t *)GUI_MALLOCZ(sizeof(quefr_t));
		if(NULL == qe)
		{
			return (NULL);
		}

		h = (quenode_t *)GUI_MALLOCZ(sizeof(quenode_t));
		if(NULL == h)
		{
			GUI_FREE(qe);
			return (NULL);
		}

		qe->front = qe->rear = h;
		qe->count = 0;
	}

	return (qe);
}

void linklist_insert(quefr_t *qe, ElemType x)
{
	quenode_t *s = NULL;

	s = (quenode_t *)GUI_MALLOCZ(sizeof(quenode_t));
	if(NULL == s)
	{
		return;
	}

	s->data = x;
	s->next = NULL;
	if(NULL == Q.front->next)
	{
		Q.front->next = s;
	}
	Q.rear->next = s;
	Q.rear = s;
	Q.count++;
}

ElemType linklist_delete(quefr_t *qe)
{
	ElemType x;
	quenode_t *p = NULL;

	if(Q.front == Q.rear)
	{
		x = NULL;
	}
	else
	{
		p = Q.front->next;
		if(NULL == p)
		{
			return (NULL);
		}
		Q.front->next = p->next;

		if(p->next==NULL)
		{
			Q.rear=Q.front;
		}
		x = p->data;
		GUI_FREE(p);
		Q.count--;
	}

	return(x);
}

quenode_t *linklist_get_front(quefr_t *qe)
{
	if(NULL == qe)
	{
		return (NULL);
	}

	if(Q.front == Q.rear)
	{
		return (NULL);
	}

	return (qe->front->next);
}

quenode_t *linklist_get_rear(quefr_t *qe)
{
	if(NULL == qe)
	{
		return (NULL);
	}

	if(Q.front == Q.rear)
	{
		return (NULL);
	}

	return (qe->rear);
}

int linklist_get_count(quefr_t *qe)
{
	if(NULL == qe)
	{
		return (0);
	}

	return (qe->count);
}

void linklist_set_front(quefr_t *qe, quenode_t *front)
{
	if(NULL == qe)
	{
		return;
	}

	Q.front->next = front;
}

void linklist_set_rear(quefr_t *qe, quenode_t *rear)
{
	if(NULL == qe)
	{
		return;
	}

	if(rear)
	{
	    Q.rear = rear;
	}
	else
	{
	    Q.rear = Q.front;
	}
}

void linklist_set_count(quefr_t *qe, int count)
{
	if(NULL == qe)
	{
		return;
	}

	Q.count = count;
}

void linklist_clear(quefr_t *qe)
{
	if(NULL == qe)
	{
		return;
	}

	Q.rear = Q.front;
	Q.count = 0;
}

void linklist_delete_all(quefr_t *qe, linklist_free_func fun)
{
	int i = 0, count = 0;
	void *data = NULL;

	if(NULL == qe)
	{
		return;
	}

	if(NULL == qe)
	{
		return;
	}

	count = linklist_get_count(qe);
	i = 0;
	while(i < count)
	{
		data = linklist_delete(qe);
		if(fun)
		{
			fun(data);
		}
		i++;
	}

	Q.rear = Q.front;
	Q.count = 0;
}

void linklist_release(quefr_t **qe, linklist_free_func fun)
{
	quefr_t *queue = NULL;

	if(NULL == qe)
	{
		return;
	}

	queue = *qe;

	if(NULL == queue)
	{
		return;
	}

	linklist_delete_all(queue, fun);
	GUI_FREE(queue->front);
	queue->front = queue->rear;
	GUI_FREE(queue);
	*qe = NULL;
}


