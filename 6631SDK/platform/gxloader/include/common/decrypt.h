#ifndef __DECRYPT_H__
#define __DECRYPT_H__


/*
 * decrypt()
 *
 *  DESCRIPTION
 *     Decrypt data, mainly based on decryption stage2, need to modify the subsequent need to decrypt the kernel and APP data
 *
 *  PARAMETER
 *  @data_addr
 *      Data address that needs to be decrypted
 *  @data_len
 *      Data length that needs to be decrypted
 *  @addition_buf
 *      Store the memory address of the entire loader data
 *  RETURN VALUE
 *      On sucess, zero is returned
 *      On error, a negative value is returned
 *
 *  NOTES
 *
 * */
int decrypt(unsigned char *data_addr, int data_len, unsigned char *addition_buf);

#endif
