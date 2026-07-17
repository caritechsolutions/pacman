#include "Utils.h"

static Node * createnode(uint32_t CRC_Tag, unsigned short DataSize){
	Node * newNode = malloc(sizeof(Node));
	newNode->CRC_Tag = CRC_Tag;
	newNode->EmmType = 0;
	newNode->NumberOf = 0;
    newNode->Interval = 0;
    newNode->TimeForNext =0xFFFFFFFF;
	newNode->EmmData = malloc(DataSize);
	newNode->next = NULL;
	return newNode;
}

List * initList(void){
  List * list = malloc(sizeof(List));
  list->head = NULL;
  return list;
}

void display(List * list) {
	Node * current = list->head;
	if(list->head == NULL)
		return;
	printf("-------------\r\nEMM Crc32 List -> ");
	while(current->next != NULL){
        printf("%u,", current->CRC_Tag);
		current = current->next;
	}
    printf("%u\n", current->CRC_Tag);
}

// Add new if not already there
Node * add(uint32_t CRC_Tag, unsigned short DataSize, List * list){
	Node * current = NULL;
	if(list->head == NULL){
		list->head = createnode(CRC_Tag, DataSize);
		return list->head;
	}
	else {
		current = list->head;
		// Reject if we already have this EMM in list
		if (current->CRC_Tag == CRC_Tag)
			return NULL;
		while (current->next!=NULL){
			current = current->next;
			if (current->CRC_Tag == CRC_Tag)
				return NULL;
		}
	current->next = createnode(CRC_Tag, DataSize);
	return current->next;
	}
	return 0;
}

// Check if we already have a Fingerprint on that SID
unsigned char findSid(unsigned short SID, List * list){
	Node * current = NULL;
	if(list->head == NULL){
		return 0;
	}
	else {
		current = list->head;
		// Reject if we already have this EMM in list
		if (current->SID == SID)
			return 1;
		while (current->next!=NULL){
			current = current->next;
			if (current->SID == SID)
				return 1;
		}
	}
	return 0;
}

void del(uint32_t CRC_Tag, List * list){
  Node * current = list->head;
  Node * previous = current;
  while(current != NULL){
    if(current->CRC_Tag == CRC_Tag){
      previous->next = current->next;
      if(current == list->head)
        list->head = current->next;
	  free(current->EmmData);
      free(current);
      return;
    }
    previous = current;
    current = current->next;
  }
}

void reverse(List * list){
  Node * reversed = NULL;
  Node * current = list->head;
  Node * temp = NULL;
  while(current != NULL){
    temp = current;
    current = current->next;
    temp->next = reversed;
    reversed = temp;
  }
  list->head = reversed;
}

void destroy(List * list){
  Node * current = list->head;
  Node * next;
  while(current != NULL){
    next = current->next;
    free(current);
    current = next;
  }
  free(list);
}

#define OFFSET_2000     946684800
//#define EMM_REJECT_TIME 6*60*60 // 60*10  10 Minutes
static unsigned int date_time_to_epoch(date_time_t* date_time)
{
    unsigned int second = date_time->second;  // 0-59
    unsigned int minute = date_time->minute;  // 0-59
    unsigned int hour   = date_time->hour;    // 0-23
    unsigned int day    = date_time->day-1;   // 0-30
    unsigned int month  = date_time->month-1; // 0-11
    unsigned int year   = date_time->year;    // 0-99
    return (((year/4*(365*4+1)+days[year%4][month]+day)*24+hour)*60+minute)*60+second;
}


static time_t getEMMtime(unsigned char *emm) {
    date_time_t msgTime;
    int y, m, d, k;
    unsigned short mjd;

    mjd = (unsigned short) emm[0] << 8 | (unsigned short) emm[1];

    y = (int) ((mjd - 15078.2) / 365.25);
    m = (int) ((mjd - 14956.1 - ((int) (y * 365.25))) / 30.6001);
    d = mjd - 14956 - ((int) (y * 365.25)) - ((int) (m * 30.6001));
    if((m == 14) || (m == 15)) {
        k = 1;
    } else {
        k = 0;
    }
    y += k;
    //y += 1900;
    y -= 100;
    m = m - 1 - k * 12;

    msgTime.second  = ((emm[4]>>4)*10 + (emm[4] & 0x0F));
    msgTime.minute  = ((emm[3]>>4)*10 + (emm[3] & 0x0F));
    msgTime.hour = ((emm[2]>>4)*10 + (emm[2] & 0x0F));
    msgTime.day = d;
    msgTime.month = m;
    msgTime.year = y;

    return date_time_to_epoch(&msgTime) + OFFSET_2000;
}

