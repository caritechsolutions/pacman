#include <gxcore.h>
#include <gxbus.h>
#include <gxmsg.h>
#include <cyg/crc/crc.h>

#define SERIAL_DEVICE_NAME        "/dev/ser0"
#define IO_GET_CONFIG_SERIAL_INFO (0x0101)

#define MSG_BODY_LEN_MAX   1024
#define MAGIC_LEN 4
#define GET_MAGIC() {0xaa,0x55,0xa5,0xa5}
#define CHECK_MASK 0x1

#define CMD_REQ 0x01
#define CMD_RSP 0x02
#define CMD_NTF 0x03

#define le16(m) ((((m)&0xFF)<<8) | (((m)&0xFF00)>>8))

typedef struct __attribute__((packed)) {
	uint16_t cmd;
	uint8_t  type;
	uint8_t  seq;
	uint8_t  flags;
	uint16_t length;
	uint16_t crc16;
} MsgHdr;

typedef struct {
	uint16_t len;
	uint8_t *data;
} SerialData;

typedef enum {
	SERIAL_BAUD_50 = 1,
        SERIAL_BAUD_75,
        SERIAL_BAUD_110,
        SERIAL_BAUD_134_5,
        SERIAL_BAUD_150,
        SERIAL_BAUD_200,
        SERIAL_BAUD_300,
        SERIAL_BAUD_600,
        SERIAL_BAUD_1200,
        SERIAL_BAUD_1800,
        SERIAL_BAUD_2400,
        SERIAL_BAUD_3600,
        SERIAL_BAUD_4800,
        SERIAL_BAUD_7200,
        SERIAL_BAUD_9600,
        SERIAL_BAUD_14400,
        SERIAL_BAUD_19200,
        SERIAL_BAUD_38400,
        SERIAL_BAUD_57600,
        SERIAL_BAUD_115200,
        SERIAL_BAUD_230400,
        SERIAL_BAUD_460800,
        SERIAL_BAUD_921600
} GxUpdate_SerialBaudRate;

typedef enum {
        SERIAL_STOP_1 = 1,
        SERIAL_STOP_1_5,
        SERIAL_STOP_2
} GxUpdate_SerialStopBits;

typedef enum {
        SERIAL_PARITY_NONE = 0,
        SERIAL_PARITY_EVEN,
        SERIAL_PARITY_ODD,
        SERIAL_PARITY_MARK,
        SERIAL_PARITY_SPACE
} GxUpdate_SerialParity;

typedef enum {
        SERIAL_WORD_LENGTH_5 = 5,
        SERIAL_WORD_LENGTH_6,
        SERIAL_WORD_LENGTH_7,
        SERIAL_WORD_LENGTH_8
} GxUpdate_SerialWordLength;

typedef struct {
        GxUpdate_SerialBaudRate     baud;           /**波特率*/
        GxUpdate_SerialStopBits     stop;           /**停止位*/
        GxUpdate_SerialParity       parity;         /**奇偶校验位*/
        GxUpdate_SerialWordLength   word_length;    /**字宽*/
        uint32_t                    flags;          /**流控*/
} GxUpdate_SerialInfo;
