#ifndef ___SM2_VERIFY_H__
#define ___SM2_VERIFY_H__

int sm2_verif_fun(unsigned int src_addr, int length, unsigned int sig_addr);
int rsa_verif_fun(unsigned int src_addr, int length, unsigned int sig_addr);
int rsa_decrypt_sig(unsigned char *sig_addr, unsigned char *pub_key_addr, unsigned char *hash_addr);
int rsa_pubkey_verif_fun(unsigned char *src_addr, int length, unsigned char *pub_key_addr, unsigned char *sig_addr);

#endif
