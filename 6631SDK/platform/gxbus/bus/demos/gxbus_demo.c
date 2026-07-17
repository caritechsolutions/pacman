#include <string.h>
#include <gxcore.h>

#include <gxmsg.h>
#include <gxbus.h>

#define GXMSG_CA_1       (200)
#define GXMSG_CA_2       (201)
#define GXMSG_CA_3       (202)
#define GXMSG_CA_4       (203)
#define GXMSG_CA_5       (204)

#define GXBUS_DemoTrace( ... )                     gxlogd( __VA_ARGS__ )


status_t GxBus_DemoInit(handle_t self)
{
	handle_t sch;

	GxBus_MessageRegister(GXMSG_CA_1, sizeof(int));
	GxBus_MessageRegister(GXMSG_CA_2, sizeof(int));
	GxBus_MessageRegister(GXMSG_CA_3, sizeof(int));
	GxBus_MessageRegister(GXMSG_CA_4, sizeof(int));
	GxBus_MessageRegister(GXMSG_CA_5, sizeof(int));

	GxBus_MessageListen(self, GXMSG_CA_1);
	GxBus_MessageListen(self, GXMSG_CA_2);
	GxBus_MessageListen(self, GXMSG_CA_3);
	GxBus_MessageListen(self, GXMSG_CA_4);
	GxBus_MessageListen(self, GXMSG_CA_5);

	sch = GxBus_SchedulerCreate("MsgScheduler", 0, 1024 * 10,GXOS_DEFAULT_PRIORITY);
	GxBus_ServiceLink(self, sch);

	sch = GxBus_SchedulerCreate("ConsoleScheduler", 1, 1024 * 10, GXOS_DEFAULT_PRIORITY);
	GxBus_ServiceLink(self, sch);

	return 0;
}

void GxBus_DemoDestroy(handle_t self)
{
	GxBus_MessageUnListen(self, GXMSG_CA_1);
	GxBus_MessageUnListen(self, GXMSG_CA_2);
	GxBus_MessageUnListen(self, GXMSG_CA_3);
	GxBus_MessageUnListen(self, GXMSG_CA_4);
	GxBus_MessageUnListen(self, GXMSG_CA_5);

	GxBus_MessageUnregister(GXMSG_CA_1);
	GxBus_MessageUnregister(GXMSG_CA_2);
	GxBus_MessageUnregister(GXMSG_CA_3);
	GxBus_MessageUnregister(GXMSG_CA_4);
	GxBus_MessageUnregister(GXMSG_CA_5);

	GxBus_ServiceUnlink(self);
}

GxMsgStatus GxBus_DemoMsgProcess(handle_t self, GxMessage *message)
{
	static int32_t  num = 0;
	int* param;

	GxCore_ThreadDelay(1000);

	if (num % 10 == 0) {
		gxlogd("start clean\n");
		GxBus_MessageEmpty(self);
		gxlogd("clean end\n");
	}

	GXBUS_DemoTrace("\nreceive num=%d\n", num++);
	switch (message->msg_id) {
		case GXMSG_CA_1: // NORMAL message
			param = GxBus_GetMsgPropertyPtr(message,int);
			GXBUS_DemoTrace("GXMSG_CA1, param=%d\n", *param);
			return GXMSG_OK;

		case GXMSG_CA_2: { // REPLY message
					 param = GxBus_GetMsgPropertyPtr(message,int);
					 GXBUS_DemoTrace("GXMSG_CA_2, param=%d\n", *param);
					 GxMessage *msg = GxBus_MessageNew(GXMSG_CA_5);
					 *GxBus_GetMsgPropertyPtr(msg,int) = 11111;
					 GxBus_MessageSend(msg);
					 param = GxBus_GetMsgPropertyPtr(message,int);
					 return GXMSG_OK;
				 }
		case GXMSG_CA_3: // NORMAL message
				 param = GxBus_GetMsgPropertyPtr(message,int);
				 GXBUS_DemoTrace("GXMSG_CA3, param=%d\n", *param);
				 return GXMSG_OK;

		case GXMSG_CA_4: // SYNC message
				 param = GxBus_GetMsgPropertyPtr(message,int);
				 GXBUS_DemoTrace("GXMSG_CA4, param=%d\n", *param);
				 *param = 8888;
				 return GXMSG_OK;

		case GXMSG_CA_5: // REPLY message replied
				 param = GxBus_GetMsgPropertyPtr(message,int);
				 GXBUS_DemoTrace("GXMSG_CA5, param=%d\n", *param);
				 return GXMSG_OK;
		default:
				 break;
	}

	return GXMSG_OK;
}

void GxBus_DemoConsoleProcess(handle_t self)
{
	static uint32_t num = 0;
	int* param;

//	GXBUS_DemoTrace("\nsend\n");
	GxMessage *msg = GxBus_MessageNew(GXMSG_CA_1);
	if (msg == NULL) {
		return;
	}
//	GXBUS_DemoTrace("GXMSG_CA_1 msg=%p\n",msg);
	param = GxBus_GetMsgPropertyPtr(msg,int);
	*param = num++;
	GxBus_MessageSend(msg);
#if 0
	msg = GxBus_MessageNew(GXMSG_CA_2);
	if (msg == NULL) {
		return;
	}
//	GXBUS_DemoTrace("GXMSG_CA_2 msg=%p\n",msg);
	param = GxBus_GetMsgPropertyPtr(msg,int);
	*param = num++;
	GxBus_MessageSend(msg);
	num++;

	msg = GxBus_MessageNew(GXMSG_CA_3);
	if (msg == NULL) {
		return;
	}
//	GXBUS_DemoTrace("GXMSG_CA_3 msg=%p\n",msg);
	param = GxBus_GetMsgPropertyPtr(msg,int);
	*param = num++;
	GxBus_MessageSend(msg);
	num++;

	GxBus_MessageEmpty(self);

	msg = GxBus_MessageNew(GXMSG_CA_4);
	if (msg == NULL) {
		return;
	}
//	GXBUS_DemoTrace("GXMSG_CA_4 msg=%p\n",msg);
	param = GxBus_GetMsgPropertyPtr(msg,int);
	*param = num++;
	GxBus_MessageSendWait(msg);
	GXBUS_DemoTrace("Sync GXMSG_CA4, param=%d\n", *param);
	GxBus_MessageFree(msg);

	GXBUS_DemoTrace("send num=%d\n", num++);
#endif
}

int GxCore_Startup(int argc, char **argv)
{
	GxServiceOps services[] = {
		{"demo", GxBus_DemoInit, GxBus_DemoDestroy, GxBus_DemoMsgProcess, GxBus_DemoConsoleProcess},
	};

	GxBus_Init(services, 1);

	GxCore_Loop();

	return 0;
}


