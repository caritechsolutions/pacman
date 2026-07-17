/**
 * @file gxupdate_serial.c
 * @author lixb
 * @brief goxceed升级架构串口传输层定义
 */
#include <fcntl.h>
#include <sys/ioctl.h>
#include <stdio.h>
#include <zlib.h>

#include "gxcore.h"
#include "update/gxupdate_protocol_serial.h"
#include "gxupdate_debug.h"

#define IO_SET_CONFIG_SERIAL_INFO       (0x0181)
#define IO_GET_CONFIG_SERIAL_INFO       (0x0101)

#define MAX_SIZE_UPDATE_BLOCK           (256)
#define MAX_CMD_SIZE                    (1024*4)
#define MAX_UPDATING_THREAD_STACK       (10*1024)
#define SERIAL_RECV_BUF_SIZE            (2048)
#define INLINE                          inline
#define UART_DEBUG

typedef enum {
	COMMAND_UNKONW,
	COMMAND_START,
	COMMAND_SEND_PGM,
} serial_cmd;

typedef enum {
	REPLY_UNKONW          = 0,
	REPLY_STATE           = 'c',
	REPLY_DEBUG_REACE     = 'd',
	REPLY_PROGRESS        = 'p',
	REPLY_VERIFY_VALUE1   = 'v',
	REPLY_VERIFY_VALUE2   = 'w',
	REPLY_READY           = 'X',
} serial_reply;

typedef enum {
	CLIENT_INIT,
	CLIENT_ACTIVE,
	CLIENT_PGM_BOOT_RUNNING,
	CLIENT_PGM_DRAM_RUNNING,
	CLIENT_PGM_DRAM_GET_DATA_READY,
	CLIENT_DUMP
} update_client_state;

struct update_serial {
	char*                   name;
	FILE*                   r_stream;
	FILE*                   w_stream;
#ifdef USE_GZIP
	gzFile                  gz_stream;
#endif
	handle_t                thread;
	volatile bool           receiving;/*serial port receiving thread running flag*/
	GxUpdate_Terminal       type;
	GxUpdate_SerialInfo     old;
	GxUpdate_SerialInfo     new_info;
	update_client_state     state;
	size_t                  src_size;
	handle_t                ready_sem;
	char                    reply[6];
	GxUpdate_SerialPlatform platform;
	GxUpdate_SerialPgmData  pgmdata;
};

static struct update_serial serial[GXUPDATE_MAX_NUM_SERIAL_OPEN] = {{0,},};


static INLINE void turn_off_stdout(void)
{
#ifdef ECOS_OS
	extern int print_enable;
	print_enable = 0;
#elif defined(LINUX_OS)
	//close(???);
#endif
}

static INLINE void turn_on_stdout(void)
{
#ifdef ECOS_OS
	extern int print_enable;
	print_enable = 1;
#elif defined(LINUX_OS)
	//open(???);
#endif
}

static INLINE uint8_t get_start_cmd(void)
{
	return 'Y';
}

static INLINE uint8_t* get_pgm(const struct update_serial* serial)
{
	return serial->pgmdata.buf;
}

static INLINE int get_pgm_length(const struct update_serial* serial)
{
	return serial->pgmdata.len;
}

static INLINE uint32_t get_data_length(const struct update_serial* serial)
{
	return serial->src_size;
}

int Wait(const struct update_serial*    serial,const char *s, int echo, int timeout)
{
	int pos = 0;
	uint8_t buf[1024];
	int len = strlen(s);
	int flag;
	int timeout_flag;

	while ( pos < len && pos < 1024) {
		timeout_flag=1;

		while(serial->receiving) {
			buf[pos] = fgetc(serial->r_stream);
			break;
		}
		{
			flag = 0;
			timeout_flag=0;
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
		if((timeout!=0)&&(timeout_flag==1))
		{
			return -1;
		}
	}
	buf[pos] = 0;

	return strncmp((const char*)buf, s, len);
}

#define MIN(a,b) ((a) < (b) ? (a) : (b))
static void send_cmd(const struct update_serial* serial, serial_cmd cmd)
{
	switch (cmd) {
	case COMMAND_START:
		{
			uint8_t*  data = get_pgm(serial);
			unsigned len = get_pgm_length(serial);
			int i, size;
			unsigned char buf[128];
			unsigned char *pdata = (unsigned char *)data;
			uint32_t crc;
			unsigned int chip_id = (pdata[0] | (pdata[1]<<8));
			unsigned char version = pdata[2];

			switch(chip_id) {
			case 0x3211:
			case 0x8010:
				size = len > 8192 ? 8192 : len;
				break;
			default:
				size = len > 4096 ? 4096 : len;
				break;
			}

			size = size / 4;

			buf[0] = 'Y';
			buf[1] = (size >>  0) & 0xFF;
			buf[2] = (size >>  8) & 0xFF;
			buf[3] = (size >> 16) & 0xFF;
			buf[4] = (size >> 24) & 0xFF;
			fwrite(buf, sizeof(uint8_t), 5, serial->w_stream);
			fflush(serial->w_stream);

			crc = 0;
			for (i=0; i < len; i++)
				crc += pdata[i];

			if (version == 0x01) {
				size = size * 4 - 4;
				fwrite(pdata + 4, sizeof(uint8_t), size, serial->w_stream);
				fflush(serial->w_stream);
				fwrite("boot", sizeof(uint8_t), 4, serial->w_stream);
				fflush(serial->w_stream);
			}
			else {
				size = size * 4 - 12;
				fwrite(pdata + 4, sizeof(uint8_t), size, serial->w_stream);
				fflush(serial->w_stream);
				fwrite(&crc, sizeof(uint8_t), 4, serial->w_stream);
				fflush(serial->w_stream);
				fwrite(&len, sizeof(uint8_t), 4, serial->w_stream);
				fflush(serial->w_stream);
				fwrite("boot", sizeof(uint8_t), 4, serial->w_stream);
				fflush(serial->w_stream);
			}
			if (Wait(serial,"GET", 0, 10) != 0) {
				gxlogd("BootROM error: wait for \"GET\",please check stb UART receive !\n");
			}
			if (version == 0x01) {
				fwrite(&crc, sizeof(uint8_t), 4, serial->w_stream);
				fflush(serial->w_stream);
				fwrite(&len, sizeof(uint8_t), 4, serial->w_stream);
				fflush(serial->w_stream);
			}

			// send bootloader code
			while (1) {
				fflush(serial->w_stream);
				i = 0;
				while (i < len) {
					i += fwrite(pdata + i, sizeof(uint8_t), MIN(len - i, 2048), serial->w_stream);
					fflush(serial->w_stream);
				}

				fflush(serial->w_stream);

				while(serial->receiving) {
					buf[0] = fgetc(serial->r_stream);
					break;
				}

				if (buf[0] == 'O'){
					break;
				}
				else if (buf[0] == 'E'){
				}else {
				}
			}
			if (Wait(serial,"boot> ", 0, 3) != 0) {
			}
			break;
		}
	case COMMAND_SEND_PGM:
		{
			char cmd[1024]={0};
			uint32_t len = get_data_length(serial);
			snprintf(cmd, sizeof(cmd), "serialdown 0 %d\n",len);
			fwrite(&cmd, sizeof(uint8_t), strlen(cmd), serial->w_stream);
			fflush(serial->w_stream);
			break;
		}
	default:
		break;
	}
}

static char* get_reply(const struct update_serial*  serial,
		char*                        reply,
		size_t                       size)
{
	int32_t ret;

	ASSERT(serial != NULL);
	ASSERT(reply != NULL);

	ret = GxCore_SemWait(serial->ready_sem);
	ASSERT(ret == 0);

	memset(reply, 0, size);
	memcpy(reply, serial->reply, 6);

	return reply;
}

int my_getline(uint8_t* buf, int* sz, FILE* s)
{
	int c;
	*sz = 0;
	do {
		c = *buf++ = (uint8_t)fgetc(s);
		(*sz)++;
	} while (c != 0x0A);
	(*buf) = '\0';
	return *sz;
}

static void serial_recving(void* args)
{
	struct update_serial* serial = (struct update_serial*)args;

	if (serial == NULL) {
		return;
	}

	if (serial->state == CLIENT_INIT) {
		while(serial->receiving) {
			while(fgetc(serial->r_stream) != 'X') {
				;
			}
			send_cmd(serial, COMMAND_START);
			serial->state = CLIENT_ACTIVE;
			send_cmd(serial, COMMAND_SEND_PGM);
			goto client_active;
		}
	}

client_active:
	while(serial->receiving) {
		if(Wait(serial,"~sta~", 0, 3) == 0)
		{
			GxCore_SemPost(serial->ready_sem);
			break;
		}
	}
	while(serial->receiving) {
		if(Wait(serial,"~fin~", 0, 3) == 0)
		{
			GxCore_SemPost(serial->ready_sem);
			return;
		}
	}
}

static INLINE void is_client_ready(struct update_serial* serial)
{
	if (serial->state != CLIENT_PGM_DRAM_GET_DATA_READY) {
		GxCore_SemWait(serial->ready_sem);
		serial->state = CLIENT_PGM_DRAM_GET_DATA_READY;
	}
}

static INLINE void wait_client_finished(struct update_serial* serial)
{
	char    buf[10];
	char*   reply;
	while(serial->receiving) {
		reply = get_reply(serial, buf, 10);
		return;
	}
	return;
}


static handle_t gxupdate_serial_open(const char* name)
{
	int32_t                 i;

	ASSERT(name != NULL);

	if (name == NULL) {
		return (handle_t)E_INVALID_HANDLE;
	}

	for (i = 0; i < GXUPDATE_MAX_NUM_SERIAL_OPEN; i++) {
		if (serial[i].name != NULL && strcmp(serial[i].name, name) == 0) {
			return (handle_t)&serial[i];
		}
	}

	for (i = 0; i < GXUPDATE_MAX_NUM_SERIAL_OPEN; i++) {
		if (serial[i].name == NULL) {
			memset(&serial[i],0,sizeof(struct update_serial));
			serial[i].name = strdup(name);
			if (serial[i].name == NULL) {
				return (handle_t)E_INVALID_HANDLE;
			}


			serial[i].r_stream = fopen(SERIAL_DEVICE_NAME, "r+");
			serial[i].w_stream = fopen(SERIAL_DEVICE_NAME, "r+");
#ifdef USE_GZIP
			serial[i].gz_stream = gzdopen(fileno(serial[i].w_stream), "wb8");
			if (serial[i].gz_stream == NULL) {
				GxCore_Free(serial[i].name);
				return (handle_t)E_INVALID_HANDLE;
			}
#endif
#ifdef UART_DEBUG
			//uart_init(0x0E);
#endif

			if (serial[i].r_stream == NULL
					|| serial[i].w_stream == NULL) {
				GxCore_Free(serial[i].name);
				return (handle_t)E_INVALID_HANDLE;
			}
			turn_off_stdout();

			ioctl(fileno((serial[i]).r_stream),
					IO_GET_CONFIG_SERIAL_INFO,
					&(serial[i]).old,
					sizeof(GxUpdate_SerialInfo));
			GxCore_SemCreate(&serial[i].ready_sem, 0);

			return (handle_t)&serial[i];
		}
	}

	return (handle_t)E_INVALID_HANDLE;
}

static GxUpdate_Terminal  gxupdate_serial_get_type(handle_t handle)
{
	struct update_serial* serial = (struct update_serial*)handle;
	return serial->type;
}

static int32_t gxupdate_serial_set_size(handle_t handle, size_t size)
{
	struct update_serial* serial = (struct update_serial*)handle;

	ASSERT(serial != NULL);
	serial->receiving = TRUE;
	serial->src_size = size;


	GxCore_ThreadCreate("serial recv thread", &serial->thread,
			serial_recving,
			serial,
			MAX_UPDATING_THREAD_STACK,
			GXOS_DEFAULT_PRIORITY - 5);
	return E_OK;
}

static int32_t  gxupdate_serial_ioctl(handle_t  handle,
		int32_t   key,
		void*     buf,
		size_t    size)
{
	struct update_serial* serial = (struct update_serial*)handle;

	ASSERT(serial != NULL);
	ASSERT(buf != NULL);

	switch(key) {
	case GXUPDATE_SERIAL_SELECT_TERMINAL_TYPE:
		if (size != sizeof(GxUpdate_ConfigSerialTerminalType)) {
			return E_FAILURE;
		}
		serial->type = ((GxUpdate_ConfigSerialTerminalType*)buf)->type;
		return E_OK;
	case GXUPDATE_SET_SERIAL_INFO:
		if (size != sizeof(GxUpdate_SerialInfo)) {
			return E_FAILURE;
		}
		serial->new_info = *(GxUpdate_SerialInfo*)buf;
		return E_OK;
	case GXUPDATE_SET_CONFIG_PLATFORM:
		if (size != sizeof(GxUpdate_SerialPlatform)) {
			return E_FAILURE;
		}
		serial->platform = *((GxUpdate_SerialPlatform*)buf);
		return E_OK;
	case GXUPDATE_SERIAL_REGISTER_PGMDATA:
		if (size != sizeof(GxUpdate_SerialPgmData)) {
			return E_FAILURE;
		}
		serial->pgmdata = *((GxUpdate_SerialPgmData*)buf);
		return E_OK;
	default:
		break;
	}
	return E_FAILURE;
}


static uint32_t gxupdate_serial_read(handle_t handle, uint8_t* buf, ssize_t* size)
{
	struct update_serial*       serial = (struct update_serial*)handle;
	ssize_t                     read_size;

	if (serial == NULL || buf == NULL) {
		return -1;
	}

	read_size = fread(buf, 1, *size, serial->r_stream);
	if(read_size < *size)
	{
		*size = read_size;
		return GXUPDATE_STREAM_FINISH;
	}

	return GXUPDATE_STREAM_CONTINUE;
}

static ssize_t gxupdate_serial_write(handle_t       handle,
		const uint8_t* ptr,
		size_t         size)
{
	struct update_serial*   serial = (struct update_serial*)handle;
	static int32_t to_size = 1024*1024*2;
	if (serial == NULL || ptr == NULL) {
		return -1;
	}

	is_client_ready(serial);
	to_size -= size;
#ifdef USE_GZIP
	gzwrite(serial->gz_stream, ptr, size);
#else
	fwrite(ptr, 1, size, serial->w_stream);
#endif
	if (size != GXUPDATE_STREAM_BLOCK_SIZE) {
#ifdef USE_GZIP
		gzflush(serial->gz_stream, Z_FINISH);
#endif
		wait_client_finished(serial);
	}
	return size;
}

static int32_t gxupdate_serial_close(handle_t handle)
{
	struct update_serial*   serial = (struct update_serial*)handle;

	ASSERT(serial != NULL);

	GxCore_SemDelete(serial->ready_sem);
	ioctl(fileno(serial->r_stream),
			IO_SET_CONFIG_SERIAL_INFO,
			&serial->old,
			sizeof(GxUpdate_SerialInfo));

	GxCore_Free(serial->name);
	serial->name = NULL;
#ifdef USE_GZIP
	gzclose(serial);
#endif
	fclose(serial->r_stream);
	fclose(serial->w_stream);

	turn_on_stdout();

	return E_OK;
}

GxUpdate_ProtocolOps gxupdate_protocol_serial = {
	GXUPDATE_PROTOCOL_SERIAL,
	gxupdate_serial_open,
	gxupdate_serial_get_type,
	gxupdate_serial_set_size,
	gxupdate_serial_ioctl,
	gxupdate_serial_read,
	gxupdate_serial_write,
	gxupdate_serial_close
};
