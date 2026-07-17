#ifndef NO_OS
#include <stdio.h>
#include <stdlib.h>
#else
#include "common/gxmalloc.h"
#endif

#include "stack.h"

gx_stack_t *stack_new(gx_stack_t *stack)
{
	stack = (gx_stack_t *) GxCore_Malloc (sizeof (gx_stack_t));
	if (stack == NULL)
		return NULL;
	stack->stack_pointer = 0;

	return stack;
}

void stack_release(gx_stack_t *stack)
{
	GxCore_Free(stack);
}

void *stack_peek(gx_stack_t *stack)
{
	if (stack->stack_pointer != 0)
		return stack->stack[stack->stack_pointer - 1];
	else
		return NULL; /* empty */
}

void *stack_get(gx_stack_t *stack, int id)
{
	if (id >= 0 && id < stack->stack_pointer)
		return stack->stack[id];
	else
		return NULL;
}

void *stack_pop(gx_stack_t *stack)
{
	if (stack->stack_pointer != 0)
		return stack->stack[--stack->stack_pointer];
	else
		return NULL; /* empty */
}

int stack_push(gx_stack_t *stack, void *item)
{
	if (stack->stack_pointer < G_STACK_SIZE) {
		stack->stack[stack->stack_pointer++] = item;
		return G_STACK_OK;
	}
	else
		return G_STACK_FULL;
}

int stack_count (gx_stack_t *s)
{
	return s->stack_pointer;
}


void *stack_del(gx_stack_t *stack, int id)//zhaofz20090925
{
	int i = 0, j = 0;

	if(id < 0 || stack->stack_pointer <= 0 || id >= stack->stack_pointer)
	{
		return NULL;
	}

	for(i = id, j = i + 1; j < stack->stack_pointer; i++, j++)
	{
		stack->stack[i] = stack->stack[j];
	}

	stack->stack[--stack->stack_pointer] = NULL;

	//return
	if(0 == stack->stack_pointer)
	{
		return NULL;
	}

	if(0 == id || 1 ==stack->stack_pointer)
	{
		return stack->stack[0];
	}

	return stack->stack[id-1];

}

int stack_get_id(gx_stack_t *stack, void* value)//zhaofz20090925
{
	int i = 0;

	if(NULL == value)
	{
		return -1;
	}

	for(i = 0; i < stack->stack_pointer; i++)
	{
		if(stack->stack[i] == value)
		{
			return i;
		}
	}

	return -1;
}


