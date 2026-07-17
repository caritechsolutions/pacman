#ifndef UTILS_H_INCLUDED
#define UTILS_H_INCLUDED
#include "gxcore.h"
#include "time.h"

typedef struct node {
  uint32_t	CRC_Tag;
  unsigned char * EmmData;
  unsigned char EmmType;
  unsigned short SID;
  short NumberOf;
  unsigned char Interval;
  time_t	TimeForNext;
  struct node * next;
} Node;

typedef struct list {
  Node * head;
} List;

List * initList(void);
Node * add(uint32_t CRC_Tag, unsigned short DataSize, List * list);
unsigned char findSid(unsigned short SID, List * list);
void del(uint32_t CRC_Tag, List * list);
void display(List * list);
void reverse(List * list);
void destroy(List * list);


const unsigned short days[4][12] =
{
    {   0,  31,  60,  91, 121, 152, 182, 213, 244, 274, 305, 335},
    { 366, 397, 425, 456, 486, 517, 547, 578, 609, 639, 670, 700},
    { 731, 762, 790, 821, 851, 882, 912, 943, 974,1004,1035,1065},
    {1096,1127,1155,1186,1216,1247,1277,1308,1339,1369,1400,1430},
};

typedef struct
{
    unsigned char second; // 0-59
    unsigned char minute; // 0-59
    unsigned char hour;   // 0-23
    unsigned char day;    // 1-31
    unsigned char month;  // 1-12
    unsigned char year;   // 0-99 (representing 2000-2099)
}
date_time_t;

#endif // UTILS_H_INCLUDED
