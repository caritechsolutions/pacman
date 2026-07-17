#ifndef __APP_SKIN_SECURE_H_202311021530__
#define __APP_SKIN_SECURE_H_202311021530__



int gxapp_skin_secure_decrypt(char *src,  int srclen, char *dst);

//0 ok
int gxapp_skin_secure_buff_md5_check(char *src,int srclen,char *verify_md5);

#endif
