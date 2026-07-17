#include "app.h"
#include "cyassl/openssl/sha.h"
#include <ctype.h>

unsigned int app_debug = 0;
unsigned int app_error = 1;

int app_debug_set(unsigned int modules)
{
    app_debug |= modules;
    return 0;
}

char *trim(char *str)
{
    int len = 0;
    if(NULL == str || *str == '\0') {
        return str;
    }

    char *p = str;
    while(*p != '\0' && isspace(*p)) {
        ++p;
        ++len;
    }
    memmove(str, p, strlen(str) - len + 1);

    if(NULL == str || *str == '\0') {
        return str;
    }
    len = strlen(str);
    p = str + len - 1;
    while(p >= str && isspace(*p)) {
        *p = '\0';
        --p;
    }
    return str;
}

int SHA1(const unsigned char *data, unsigned long len, unsigned char *buf)
{
    SHA_CTX ctx;

    if(NULL == data || NULL == buf || 0 == len)
        return -1;

    SHA_Init(&ctx);
    SHA_Update(&ctx, data, len);
    SHA_Final(buf, &ctx);
    return 0;
}

unsigned char char2hex(unsigned char c)
{
    unsigned char hex = 0xff;

    if(c >='0' && c <= '9')
    {
        hex = c - '0';
    }
    else if(c >='a' && c <= 'f')
    {
        hex = c-'a'+10;
    }
    else if(c >='A' && c <= 'F')
    {
        hex = c-'A'+10;
    }

    return hex;
}

/*
 * hex: 16进制数组
 * str: 字符串数组
 * hexlen： 16进制数组长度
 * strlen： 字符串数组长度
*/
int hex2str(unsigned char* hex,unsigned char* str, int hexlen, int strlen)
{
    int i =0;
    char buffer[3] = {0};

    if(strlen/2 < hexlen)
    {
        APP_ERR(APP_MISC,"!!!!!!strlen is short!!!!!");
        return -1;
    }

    memset(str, 0, strlen);
    for(i = 0; i < hexlen; i++)
    {
        snprintf(buffer,3,"%02x",hex[i]);
        if(i == 0)
        {
            strncpy((char*)str,buffer,2);
        }
        else
        {
            strcat((char*)str, buffer);
        }
    }

    return 0;
}

int str2hex(unsigned char* str,unsigned char* hex, int strlen, int hexlen)
{
    int i = 0;
    unsigned char h = 0xff, l = 0xff;

    if(hexlen*2 < strlen)
    {
        APP_ERR(APP_MISC,"!!!!!!hexlen is short!!!!!");
        return -1;
    }

    memset(hex, 0, hexlen);
    for(i=0 ; i< hexlen ; i++)
    {
        h = char2hex(str[2*i]);
        l = char2hex(str[2*i+1]);

        if(h == 0xff || l == 0xff)
        {
            APP_ERR(APP_MISC, "!!!!!!the %d char is not hex!!!!!", (h == 0xff) ? 2*i : (2*i+1));
            return -1;
        }

        hex[i] = (h<<4) + l;
    }

    return 0;
}

char *get_size_str(char *size_str, int size)
{
    if(size % SIZE_UNIT_K){
        sprintf(size_str, "%d B", size);
    }else{
        size /= SIZE_UNIT_K;		//k
        if(size % SIZE_UNIT_K){
            sprintf(size_str, "%d KB", size);
        }else{
            size /= SIZE_UNIT_K; //m
            sprintf(size_str, "%d MB", size);
        }
    }

    return size_str;
}

