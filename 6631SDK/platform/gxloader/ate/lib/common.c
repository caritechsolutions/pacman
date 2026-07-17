#include <stdio.h>
#include <string.h>

typedef struct _Node{
	char *word;
	int cnt;
}Node;

static int cmp(void* a,void* b){//（int*）属强制转换，再*间接取址
	return *(int *)a - *(int *)b;
}

static int cmp2(void* a,void* b){
	Node pa = *(Node *)a;
	Node pb = *(Node *)b;
	return strcmp(pa.word,pb.word);
}

static void swap(char *a,char* b,int width){
	char temp;
	while(width --){
		temp = *a,*a++ = *b,*b++ = temp;
	}
}

void qsort(void* base,int nleft,int nright,int width,int (*fcmp)(void*,void*)){
	int i = 0;

	if(nleft >= nright) return;
	char *lo = (char*)base + width*nleft;
	char *hi = (char*)base + width*((nleft + nright)/2);
	swap(lo,hi,width);
	int x = nleft;
	lo = (char*)base + width * nleft;//standard
	for(i = nleft + 1 ; i <= nright ; i ++){
		hi = (char*)base + width * i;
		if(fcmp(lo,hi) < 0){
			x ++;
			char *ll = (char*)base + width * x;
			swap(ll,hi,width);
		}
	}
	hi = (char*)base + width * x;
	swap(lo,hi,width);
	qsort(base,nleft,x - 1,width,fcmp);
	qsort(base,x + 1,nright,width,fcmp);
}

long atol(const char *nptr)
{
	int c;              /* current char */
	long total;         /* current total */
	int sign;           /* if '-', then negative, otherwise positive */

	/* skip whitespace */
	while ( isspace((int)(unsigned char)*nptr) )
		++nptr;

	c = (int)(unsigned char)*nptr++;
	sign = c;           /* save sign indication */
	if (c == '-' || c == '+')
		c = (int)(unsigned char)*nptr++;    /* skip sign*/

	total = 0;

	while (isdigit(c)) {
		total = 10 * total + (c - '0');     /*accumulate digit*/
		c = (int)(unsigned char)*nptr++;
		/* get next char* */
	}

	if (sign == '-')
		return -total;
	else
		return
			total;
	/* return result, negated if
	 * necessary */
}

char *getenv(const char *name)
{
	return NULL;
}

#include "setjmp.h"
void longjmp(jmp_buf env, int val)
{
	if (0 == val)
		++val;
	hal_longjmp(env, val);
}
