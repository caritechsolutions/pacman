#ifndef __MESSAGE_H__
#define __MESSAGE_H__

#define MAX_MT_STR_LEN 768
typedef struct _MTMessageClass{
#define MT_ERR 1
    char str[MAX_MT_STR_LEN];
    char err_flag;
    unsigned int err_count;
}MTMessageClass;
#endif
