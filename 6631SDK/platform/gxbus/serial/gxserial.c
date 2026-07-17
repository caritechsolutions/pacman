#include <sys/ioctl.h>
#include "gxserial.h"
#include "gxos/gxcore_os_core.h"

FILE* serial = NULL;
const char magic[MAGIC_LEN] = GET_MAGIC();
uint32_t seq = 0;

status_t GxSerialServiceInit(handle_t self, int priority_offset)
{
	handle_t sch;
	GxUpdate_SerialInfo sInfo;

	GxBus_MessageRegister(GXMSG_SERIAL_RX, sizeof(SerialData));

	GxBus_MessageRegister(GXMSG_SERIAL_TX, sizeof(SerialData));

	GxBus_MessageListen(self, GXMSG_SERIAL_TX);

	serial = fopen(SERIAL_DEVICE_NAME, "r+");
	if(serial == NULL)
		return -1;
	ioctl(fileno(serial),
			IO_GET_CONFIG_SERIAL_INFO,
			&sInfo,
			sizeof(GxUpdate_SerialInfo));
	gxlogi("Open serial succ: baud = %d, stop = %d, parity = %d, wlen = %d, flags = %d\r\n", sInfo.baud, sInfo.stop, sInfo.parity, sInfo.word_length, sInfo.flags);

	sch = GxBus_SchedulerCreate("SerialMsgScheduler", GXBUS_SCHED_MSG, 1024 * 32, GXOS_DEFAULT_PRIORITY + priority_offset);
	GxBus_ServiceLink(self, sch);

	sch = GxBus_SchedulerCreate("SerialConsoleScheduler", GXBUS_SCHED_CONSOLE, 1024 * 32, GXOS_DEFAULT_PRIORITY + priority_offset);
	GxBus_ServiceLink(self, sch);
	return GXCORE_SUCCESS;
}

void GxSerialServiceDestroy(handle_t self)
{
	GxBus_MessageUnListen(self, GXMSG_SERIAL_TX);

	GxBus_MessageUnregister(GXMSG_SERIAL_RX);
	GxBus_MessageUnregister(GXMSG_SERIAL_TX);
	GxBus_ServiceUnlink(self);
}

GxMsgStatus GxSerialServiceRecvMsg(handle_t self, GxMessage *msg)
{
	SerialData *data;
	MsgHdr *hdr;
	uint8_t ori[2048];
	uint8_t *buf;
	data = GxBus_GetMsgPropertyPtr(msg, SerialData);
	uint32_t crc16;
	uint8_t flags = CHECK_MASK;
	buf = ori;
	memcpy(buf, magic, MAGIC_LEN);
	buf += MAGIC_LEN;
	hdr = (MsgHdr *)buf;
	hdr->cmd = le16(CMD_RSP);
	hdr->type = 0x00;
	hdr->seq = seq;
	hdr->flags = flags;
	hdr->length = le16(data->len + ((hdr->flags&CHECK_MASK)?2:0));
	buf += sizeof(MsgHdr) - 2;
	crc16 = cyg_crc16(ori, buf - ori);
	buf[0] = (crc16&0xFF00)>>8;
	buf[1] = crc16&0xFF;
	buf += 2;

	memcpy(buf, data->data, data->len);
	if (hdr->flags&CHECK_MASK){
		crc16 = cyg_crc16(buf, data->len);
		buf += data->len;
		buf[0] = (crc16&0xFF00)>>8;
		buf[1] = crc16&0xFF;
		buf += 2;
	}else
		buf += data->len;
	fwrite(ori, 1, buf - ori, serial);
	return 0;
}

void Wait(const char *s, int len, char *out)
{
	int pos = 0;
	uint8_t buf[1024];
	int flag;
	while ( pos < len && pos < 1024) {
		buf[pos] = fgetc(serial);
		{
			flag = 0;
			if (buf[pos] == '\r') {
				buf[pos] = '\n';
				flag = 1;
			}
			if (s[pos] == buf[pos]) {
				pos ++;
			}
			else {
				if (buf[pos] == '\n' && flag == 1)
					buf[pos] = '\r';
				pos = 0;
			}
		}
	}
	memcpy(out, s, pos);
}

int message_received(uint8_t **data)
{
	uint16_t crc16 = 0;
	uint16_t bodylen;
	uint32_t read_size = 0;
	uint32_t size = 0;
	uint32_t i = 0;
	uint8_t *msg;
	MsgHdr *msg_hdr;

	msg = GxCore_Malloc(sizeof(MsgHdr) + MAGIC_LEN);
	if (!msg){
		gxlogi("No enough buffer!!!\r\n");
		return -1;
	}
	Wait(magic, MAGIC_LEN, (char *)msg);
	msg_hdr = (MsgHdr *)(msg + MAGIC_LEN);
	read_size = fread(msg_hdr, 1, sizeof(MsgHdr), serial);
	if (read_size < sizeof(MsgHdr)){
		gxlogi("RECV MSG LENGTH ERROR!!!\r\n");
		GxCore_Free(msg);
		return -1;
	}

	seq = msg_hdr->seq;
	bodylen = le16(msg_hdr->length);
	crc16 = le16(cyg_crc16(msg,  sizeof(MsgHdr) + MAGIC_LEN - 2));
	if (msg_hdr->crc16 != crc16){
		gxlogi("CRC ERROR, expect 0x%x but real is 0x%x\r\n", crc16, msg_hdr->crc16);
		GxCore_Free(msg);
		return -1;
	}

	switch(le16(msg_hdr->cmd)){
	case CMD_REQ:
	case CMD_RSP:
	case CMD_NTF:
		*data = GxCore_Malloc(bodylen);
		read_size = fread(*data, 1, bodylen, serial);
		if(read_size != bodylen){
			gxlogi("CMD body expect len is %d, but only %d received\r\n", bodylen, size);
			GxCore_Free(msg);
			return -1;
		}
		break;
	default:
		gxlogi("Unknow CMD %x\r\n", msg_hdr->cmd);
		break;
	}
	if ((msg_hdr->flags & CHECK_MASK) && read_size > 2){
		crc16 = le16(cyg_crc16(*data,  read_size - 2));
		if((((*data)[read_size - 2])|((*data)[read_size - 1]<<8)) != crc16){
			gxlogi("CRC ERROR, expect 0x%x but real is 0x%x\r\n", crc16, (((*data)[read_size - 2])|((*data)[read_size - 1]<<8)));
			GxCore_Free(msg);
			return -1;
		}
		read_size -= 2;
	}
	GxCore_Free(msg);
	return read_size;
}

void GxSerialServiceConsole(handle_t self)
{
	GxMessage *msg = NULL;
	int32_t read_size;
	SerialData *serial_data;
	uint8_t *data;

	if(serial == NULL)
		return;
	read_size = message_received(&data);
	if (read_size < 0)
		return;
	msg = GxBus_MessageNew(GXMSG_SERIAL_RX);

	if (!msg) {
		gxlogi("Failed to create a new message.\n");
		return;
	}
	serial_data = GxBus_GetMsgPropertyPtr(msg, SerialData);
	serial_data->len = read_size;
	serial_data->data = data;
	GxBus_MessageSendWait(msg);
	GxCore_Free(data);
	GxBus_MessageFree(msg);

	GxCore_ThreadDelay(2 * 1000);
}

GxServiceClass serial_service = {
	.name = "serial_service",
	.init = GxSerialServiceInit,
	.destroy = GxSerialServiceDestroy,
	.msg_process = GxSerialServiceRecvMsg,
	.console_process = GxSerialServiceConsole,
	.priority_offset = 0,
};

