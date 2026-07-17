#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "termios.h"

#include "module/app_log.h"
#include "../include/module_command.h"
#include "../include/common_autotest.h"
#include "common/gx_crc32.h"

extern unsigned int g_ver1;
extern unsigned int g_ver2;
extern unsigned int g_ver3;

static int __clean_receive_fifo(int handle,int mode)
{
	int fd = 0;
	int ser0 = 0;

	if(handle < 0)
	{
		ser0 = open("/dev/ser0", O_RDONLY);
		if (ser0 == -1) {
			app_log_error("open failed\n");
			return -2;
		}
		fd = ser0;
	}
	else
	{
		fd = handle;
	}

	if(mode == 0)
	{
		tcflush(fd, TCIFLUSH); //清空输入FIFO
	}
	else
	{
		tcflush(fd, TCOFLUSH); //清空输出FIFO
	}

	if(ser0 > 0)
        {
               close(ser0);
	       ser0 = -1;
	}

	return fd;
}

static int __find_first_keyword(char *cmd, char *keyword)
{
	int len = 0;
	char buffer[MAX_CMDBUF_SIZE] = {0};
	if((cmd == NULL) || (keyword == NULL))
	{
		app_log_error("\nAUTO TEST, error, [%s, %d]!\n", __FUNCTION__, __LINE__);
		return -1;
	}
	len = strlen(cmd);
	if((len == 0) || (len >= MAX_CMDBUF_SIZE))
	{
		app_log_error("\nAUTO TEST, error, [%s, %d]!\n", __FUNCTION__, __LINE__);
		return -1;
	}
	if((cmd[0] < 'a') || (cmd[0] > 'z'))
		sscanf(cmd, "%*[^a-z]%s",buffer);
	else
		sscanf(cmd, "%s", buffer);
//	app_log_debug("\ncmd = %s, keyword = %s\n", cmd, buffer);
	len = strlen(buffer);
	if((len == 0) || (len >= MAX_CMDBUF_SIZE))
	{
		app_log_error("\nAUTO TEST, error, [%s, %d]!\n", __FUNCTION__, __LINE__);
		return -1;
	}

	memcpy(keyword, buffer, len);
	return 0;
}

/***************************************************************************
* Function		:parse_line
* Description	:get command param num, command name and params
* Arguments		:[in]line
				 [in]argv[]
* Return 		: return param num -- argc
* Others		:argv[0]--name  argv[1]--params1  argv[2]--params2
****************************************************************************/
static int __parse_line (char *line, char *argv[])
{
	int nargs = 0;
	//app_log_debug("Line: %s|\n",line);
	while (nargs < CONFIG_SYS_MAXARGS) {
		 //skip any white space
		while ((*line == ' ') || (*line == '\t')) {
			++line;
		}

		if(*line == '\n'){
			*line = '\0';
		}

		if (*line == '\0') {	// end of line, no more args
			argv[nargs] = NULL;
			return (nargs);
		}

		argv[nargs++] = line;	// begin of argument string

		// find end of string
		while (*line && (*line != ' ') && (*line != '\t') && (*line != '\n')) {
			++line;
		}

		if(*line == '\n'){
			*line = '\0';
		}

		if (*line == '\0' ) {	// end of line, no more args
			argv[nargs] = NULL;
			return (nargs);
		}

		*line++ = '\0';		//terminate current arg
	}

	app_log_info("** Too many args (max. %d) **\n", CONFIG_SYS_MAXARGS);

	return (nargs);
}

static int _auto_test_cmd_crc_check(char *cmd, int cmdlen)
{
#define KEYWORD_CRC  "crc32"
    int ret = 0;
    unsigned int crc_verify = 0;
    char* p = NULL;
    char Buffer[50] = {0};
    unsigned int crc_value = 0;

    if((cmd == NULL) || (0 == cmdlen))
    {
        return -1;
    }
    p = strstr(cmd, KEYWORD_CRC);
    if((NULL == p) || (p == cmd))
    {
        return -1;
    }
    memset(Buffer, 0, sizeof(Buffer));
    sscanf(p,"%*[^=]=%[^ ]", Buffer);
    sscanf(Buffer, "%x", &crc_verify);
    crc_value = GxCore_Crc32(0, (unsigned char*)cmd,  p-cmd);
    if(crc_verify != crc_value)
    {
        return -1;
    }
    return ret;
}

static int _auto_test_one_cmd_proccess(char *cmd, int cmdlen)
{
	signed int ret = 0;
	char buffer[MAX_CMDBUF_SIZE] = {0};
	char *argv[CONFIG_SYS_MAXARGS + 1] = {0};
	int argc = 0;
	cmd_tbl_t *cmd_ops = NULL;

    if(g_ver1 > 0){
        ret = _auto_test_cmd_crc_check(cmd, cmdlen);
        if(ret < 0)
        {
            // crc check is wrong
            auto_test_reply("CRC32 Check", "failed: crc32 error!", 4, cmd);
            goto err;
            //return -1;
        }
    }

	ret = __find_first_keyword(cmd,buffer);
	if(ret < 0)
	{
		// cmd is wrong
		app_log_error("\nAUTO TEST, error, [%s, %d]!\n", __FUNCTION__, __LINE__);
		auto_test_reply("find_firt_keywords", "failed: the cmd is not support!", 2, cmd);
       goto err;
		//return -1;
	}
	cmd_ops = find_cmd(buffer);
	if(cmd_ops == NULL)
	{
		// cmd is wrong, cmd is not support
		app_log_error("\nAUTO TEST, error, [%s, %d]!\n", __FUNCTION__, __LINE__);
		auto_test_reply("find_cmd", "failed: the cmd is not support!", 2, cmd);
		goto err;
      //return -1;
	}
	if(cmd_ops->cmd != NULL)
	{
		memset(buffer, 0, MAX_CMDBUF_SIZE);
		memcpy(buffer, cmd, strlen(cmd));
		argc = __parse_line(buffer, argv);
		if((argc == 0) || (argc >= CONFIG_SYS_MAXARGS))
		{
			// cmd is wrong, cmd is not support
			app_log_error("\nAUTO TEST, error, [%s, %d]!\n", __FUNCTION__, __LINE__);
			auto_test_reply(cmd_ops->name, "failed: parameter is wrong!", 2, NULL);
			goto err;
           //return -1;
		}
#if 0
#ifdef ECOS_OS
        // for gx uart module, reset the uart receive fifo
        {// NOTICE: the register address is different from different chips
            //int i = 1000;
            //*(volatile unsigned int*)0xa0400010 |=(1<<4);
            //while(i--);
            //*(volatile unsigned int*)0xa0400010 &=~(1<<4);
            __clean_receive_fifo(app_get_uart_handle(),0);

        }
#endif
#endif
		ret = cmd_ops->cmd(cmd_ops, 0, argc, argv, cmd);
		if(ret < 0)
		{
			// cmd execute failed
			app_log_error("\nAUTO TEST, error, [%s, %d]!\n", __FUNCTION__, __LINE__);
			goto err;
          //  return -1;
		}
	}
	else
	{
		// cmd is wrong, cmd is not support
		app_log_error("\nAUTO TEST, error, [%s, %d]!\n", __FUNCTION__, __LINE__);
		auto_test_reply(cmd_ops->name, "failed: have no function to deal with the cmd!", 1, NULL);
		goto err;
       // return -1;
	}
	return 0;

err:
    // for gx uart module, reset the uart receive fifo
    {// NOTICE: the register address is different from different chips
        int ser0 = app_get_uart_handle();
        GxCore_ThreadDelay(1);
        __clean_receive_fifo(ser0,0);
    }

    return -1;
}

// add in 20210930
static char* _deal_with_special_char(char *in_str)
{
    unsigned int len = 0;
    int i = 0;
    if(NULL == in_str)
    {
        return NULL;
    }
    len = strlen(in_str);
    for(i = 0; i < len; i++)
    {
        if((in_str[i] == '\n') || (in_str[i] == '\r'))
            in_str[i] = 0x20;
    }
    return in_str;
}

int auto_test_reply_with_crc32(char *cmd, char *str, unsigned int error_code, char *result)
{
#define CRC32_STR       "crc32:"
#define END_FLAG_STR    "\r\n"
#define REPLAY_CMD_LEN  MAX_CMDBUF_SIZE*2
	char buffer[REPLAY_CMD_LEN] = {0};
    int ret = 0;
    int protect_len = strlen(END_FLAG_STR) + (1 + strlen(CRC32_STR) + 8);
    unsigned int crc_value = 0;
    char crc_str[16] = {0};

    if(result != NULL){
        sprintf(buffer, "code:%d ", error_code);
        strcat(buffer, "data: ");
        strcat(buffer, _deal_with_special_char(result));
        strcat(buffer, " message: ");
    }else{
        sprintf(buffer, "code:%d message: ", error_code);
    }
    strcat(buffer, "reply ");
    strcat(buffer, cmd);
    strcat(buffer, " ");
    if((strlen(buffer) + strlen(str)) >= (REPLAY_CMD_LEN - protect_len)){
        char *p = NULL;
        p = strchr(str,':');
        if(p == NULL)
            strcat(buffer, "failed");
        else
            memcpy(buffer+strlen(buffer), str, (REPLAY_CMD_LEN - strlen(buffer) - protect_len));
    }else{
        strcat(buffer, str);
    }

    strcat(buffer, " ");

    // for crc32
    crc_value = GxCore_Crc32(0, (unsigned char*)buffer, strlen(buffer));
    memset(crc_str, 0, strlen(crc_str));
    strcat(buffer, CRC32_STR);
    sprintf(crc_str, "%08x", crc_value);
    strcat(buffer, crc_str);

    // end flag
    strcat(buffer, END_FLAG_STR);

    ret = uart_send((unsigned char*)buffer,strlen(buffer));
    if(ret < 0){
        app_log_error("\nAUTO TEST, error, [%s, %d]!\n", __FUNCTION__, __LINE__);
        return -1;
    }
    return 0;
}

int auto_test_reply_without_crc32(char *cmd, char *str, unsigned int error_code, char *result)
{
#define REPLAY_CMD_LEN  MAX_CMDBUF_SIZE*2
    char buffer[REPLAY_CMD_LEN] = {0};
    int ret = 0;

    if(result != NULL){
        sprintf(buffer, "code:%d ", error_code);
        strcat(buffer, "data: ");
        strcat(buffer, _deal_with_special_char(result));
        strcat(buffer, " message: ");
    }else{
        sprintf(buffer, "code:%d message: ", error_code);
    }
    strcat(buffer, "reply ");
	strcat(buffer, cmd);
	strcat(buffer, " ");
	if((strlen(buffer) + strlen(str)) >= (REPLAY_CMD_LEN - 2))
	{
		char *p = NULL;
		p = strchr(str,':');
		if(p == NULL)
		{
			strcat(buffer, "failed");
		}
		else
		{
			memcpy(buffer+strlen(buffer), str, (REPLAY_CMD_LEN - strlen(buffer)));
		}

	}
	else
	{
		strcat(buffer, str);
	}
	strcat(buffer, "\r\n");

	ret = uart_send((unsigned char*)buffer,strlen(buffer));
	if(ret < 0)
	{
		app_log_error("\nAUTO TEST, error, [%s, %d]!\n", __FUNCTION__, __LINE__);
		return -1;
	}
	return 0;
}

// export
int auto_test_reply(char *cmd, char *str, unsigned int error_code, char *result)
{
    if((cmd == NULL) || (str == NULL))
        app_log_info("\nreply failed!!!\n");

    if(g_ver1>0)
        return auto_test_reply_with_crc32(cmd, str, error_code, result);
    else
        return auto_test_reply_without_crc32(cmd, str, error_code, result);
}

int auto_test_protocol_data_receive(unsigned char* MemBuffer, int MemSize,
											 int *pReadPos,int *pWritePos,
											 unsigned char *ReceiveData,int ReceiveSize)
{
	if((MemBuffer == NULL)
		|| (ReceiveData == NULL)
		|| (pReadPos == NULL)
		|| (pWritePos == NULL)
		|| (MemSize*2 < (ReceiveSize + *pWritePos))
		|| (MemSize < ReceiveSize)
		|| (ReceiveSize <= 0))
	{
		app_log_error("\nPROTOCOL, parameter error\n");
		return -1;
	}

	if((*pWritePos + ReceiveSize) > MemSize)
	{
		memcpy((MemBuffer + *pWritePos), ReceiveData, (MemSize - *pWritePos));
		memcpy(MemBuffer,(ReceiveData + (MemSize - *pWritePos)),(ReceiveSize - (MemSize - *pWritePos)));
		*pWritePos = ReceiveSize - (MemSize - *pWritePos);
		if(*pWritePos >= *pReadPos)
		{
			*pWritePos = 0;
			*pReadPos = 0;
			app_log_error("\nPROTOCOL, buff is full\n");
			return -2;
		}
	}
	else
	{
		memcpy((MemBuffer + *pWritePos), ReceiveData, ReceiveSize);
		if((*pWritePos + ReceiveSize) == MemSize)
			*pWritePos = 0;
		else
			*pWritePos +=  ReceiveSize;
	}
	return 0;
}

int auto_test_protocol_data_dealwith(unsigned char* MemBuffer, int MemSize,
											 int *pReadPos,int *pWritePos,
											 int ByteToDo)
{
	char *pData 							= NULL;
	char *pTemp 							= NULL;
	char *pTempParser						= NULL;
	char *p									= NULL;
	int ByteToDealWith 						= ByteToDo;
	int RealDoSize 							= 0;
	int Ret									= 0;
	int Length								= 0;
	int i									= 0;

	if((MemBuffer == NULL)
		|| (pReadPos == NULL)
		|| (pWritePos == NULL)
		|| (MemSize < *pReadPos)
		|| (MemSize < ByteToDealWith)
		|| (ByteToDealWith <= 0))
	{
		app_log_error("\nSK PROTOCOL, parameter error\n");
		return -1;
	}
	if(*pReadPos == *pWritePos)
	{
		return -1;
	}
	app_log_debug("\nPROTOCOL, Read = %d, Write = %d\n",*pReadPos,*pWritePos);

	if(*pReadPos > *pWritePos)
	{
		if(((MemSize - *pReadPos) + *pWritePos) < ByteToDealWith)
			ByteToDealWith = (MemSize - *pReadPos) + *pWritePos;
	}
	else
	{
		if((*pWritePos - *pReadPos) < ByteToDealWith)
			ByteToDealWith = *pWritePos - *pReadPos;
	}

	pTemp = calloc(1,ByteToDealWith+1);
	if(pTemp == NULL)
	{
		UartPrintf("PROTOCOL, calloc error\n");
		return -1;
	}

	pTempParser = calloc(1,ByteToDealWith+1);
	if(pTempParser == NULL)
	{
		app_log_error("PROTOCOL, calloc error\n");
		free(pTemp);
		return -1;
	}

	if((*pReadPos + ByteToDealWith) <= MemSize)
	{
		pData = (char*)MemBuffer + *pReadPos;
		memcpy(pTemp,pData,ByteToDealWith);
	}
	else
	{
		pData = (char*)MemBuffer + *pReadPos;
		memcpy(pTemp,pData,(MemSize - *pReadPos));
		pData = (char*)MemBuffer;
		memcpy((pTemp + (MemSize - *pReadPos)),pData,(ByteToDealWith - (MemSize - *pReadPos)));
	}
	// do the data
	pData = pTemp;
	p = strstr((char*)pData,CMD_END_FLAG);
	if(p == NULL)
	{
		//pTemp += ByteToDealWith;
		app_log_info("PROTOCOL, bad data...\n");
		goto BACK;
	}
	while(1)
	{
		memset(pTempParser, 0, ByteToDealWith+1);
		Length = p - pData ;
		memcpy(pTempParser,pData,Length);

		for(i = 0; i < Length; i++)
		{
			if(pTempParser[i] == '\r')
				pTempParser[i] = ' ';
		}
		Ret = _auto_test_one_cmd_proccess(pTempParser,Length);
		if(Ret < 0)
		{
			//app_log_error("\nPROTOCOL, parser cmd failed...\n");
			//break;
		}
		pData += (Length + strlen(CMD_END_FLAG));
		p = strstr((char*)pData,CMD_END_FLAG);
		if(p == NULL)
		{
			app_log_error("\nPROTOCOL, not enough data to deal with...\n");
			break;
		}
	}

BACK:
	// successfull, set the read position
	RealDoSize = pData - pTemp;
	if((*pReadPos + RealDoSize) > MemSize)
	{
		*pReadPos = (*pReadPos + RealDoSize) - MemSize;
	}
	else if((*pReadPos + RealDoSize) == MemSize)
	{
		*pReadPos = 0;
	}
	else
	{
		*pReadPos += RealDoSize;
	}
	free(pTemp);
	free(pTempParser);
	return 0;
}


