/*    de/encryption function, DES
 *
 */
#include <string.h>

const unsigned char ECB_pc1[56] = {
    57,49,41,33,25,17,9,
    1,58,50,42,34,26,18,
    10,2,59,51,43,35,27,
    19,11,3,60,52,44,36,
    63,55,47,39,31,23,15,
    7,62,54,46,38,30,22,
    14,6,61,53,45,37,29,
    21,13,5,28,20,12,4
};

static void ECB_pc_1(unsigned char *data)
{
    unsigned char i, j, k=0, tmp[8];

    for (i=0; i<7; i++) {
        tmp[i] = 0;
        for (j=0x80; j!=0; j>>=1) {
            if ((data[(ECB_pc1[k] - 1) / 8] << ((ECB_pc1[k] - 1) % 8)) & 0x80)
                tmp[i] |= j;
            k++;
        }
    }

    memcpy(data, tmp, 8);
}

const unsigned char ipp[64] = {
    58, 50, 42, 34, 26, 18, 10, 2,
    60, 52, 44, 36, 28, 20, 12, 4,
    62, 54, 46, 38, 30, 22, 14, 6,
    64, 56, 48, 40, 32, 24, 16, 8,
    57, 49, 41, 33, 25, 17,  9, 1,
    59, 51, 43, 35, 27, 19, 11, 3,
    61, 53, 45, 37, 29, 21, 13, 5,
    63, 55, 47, 39, 31, 23, 15, 7
};

static void ECB_IP(unsigned char *data)
{
    unsigned char i, j, k=0, tmp[8];

    for (i=0; i<8; i++) {
        tmp[i] = 0;
        for (j=0x80; j!=0; j>>=1) {
            if ((data[(ipp[k] - 1) / 8] << ((ipp[k] - 1) % 8)) & 0x80)
                tmp[i] |= j;
            k++;
        }
    }

    memcpy(data, tmp, 8);
}

static void ECB_FP(unsigned char *data)
{
    unsigned char k, tmp[8];

    for (k=0; k<8; k++) tmp[k] = 0;
    for (k=0; k<64; k++) {
        if ((data[k / 8] << (k % 8)) & 0x80)
            tmp[(ipp[k] - 1) / 8] |= 0x80 >> ((ipp[k] - 1) % 8);
    }

    memcpy(data, tmp, 8);
}


const unsigned char pc_2[48] = {
    13, 16, 10, 23,  0,  4,
    2, 27, 14,  5, 20,  9,
    22, 18, 11,  3, 25,  7,
    15,  6, 26, 19, 12,  1,
    40, 51, 30, 36, 46, 54,
    29, 39, 50, 44, 32, 47,
    43, 48, 38, 55, 33, 52,
    45, 41, 49, 35, 28, 31
};

static void ECB_PC2(const unsigned char *key, unsigned char *key2)
{

    int i,j,x,y;
    int k = 0;

    for (i=0; i<6; i++) {
        key2[i] = 0;
        for (j=0; j<8; j++) {
            key2[i] = key2[i] << 1;
            x = pc_2[k] / 8;
            y = 7 - (pc_2[k] - (x * 8));
            k++;
            key2[i] |= (key[x] >> y) & 0x01;
        }
    }
}

const unsigned char ECB_E[48] = {
    31,  0,  1,  2,  3,  4,  3,  4,
    5,  6,  7,  8,  7,  8,  9, 10,
    11, 12, 11, 12 ,13, 14, 15, 16,
    15, 16, 17, 18, 19, 20, 19, 20,
    21, 22, 23, 24, 23, 24, 25, 26,
    27, 28, 27, 28, 29, 30, 31,  0
};

static void ECB_Expand(unsigned char *data)
{

    int i,k,x,y;
    int j = 0;
    unsigned char tmp[6];

    for (k=0; k<6; k++) {
        tmp[k] = 0;
        for (i=0; i<8; i++) {
            tmp[k] = tmp[k] << 1;
            x = ECB_E[j] / 8;
            y = 7 - (ECB_E[j] - (x * 8));
            j++;
            tmp[k] |= (data[x] >> y) & 0x01;
        }
    }

    data[0] = (tmp[0] >> 2);
    data[1] = (tmp[1] >> 4) | ((tmp[0] << 4) & 0x30);
    data[2] = (tmp[2] >> 6) | ((tmp[1] << 2) & 0x3f);
    data[3] = (tmp[2] & 0x3f);
    data[4] = (tmp[3] >> 2);
    data[5] = (tmp[4] >> 4) | ((tmp[3] << 4) & 0x30);
    data[6] = (tmp[5] >> 6) | ((tmp[4] << 2) & 0x3f);
    data[7] = (tmp[5] & 0x3f);
}

const unsigned char ECB_P[32] = {
    15,  6, 19, 20,
    28, 11, 27, 16,
    0, 14, 22, 25,
    4, 17, 30,  9,
    1,  7, 23, 13,
    31, 26,  2,  8,
    18, 12, 29,  5,
    21, 10,  3, 24
};

static void ECB_Permute(unsigned char *data)
{

    unsigned char tmp[4];
    int i,j,x,y;
    int k=0;

    for (i=0; i<4; i++) {
        tmp[i] = 0;
        for (j=0; j<8; j++) {
            tmp[i] = tmp[i] << 1;
            x = ECB_P[k] / 8;
            y = 7 - (ECB_P[k] - (x * 8));
            k++;
            tmp[i] |= (data[x] >> y) & 0x01;
        }
    }

    memcpy(data, tmp, 4);
}

const unsigned char ECB_S[512] = {
    224, 64,208, 16, 32,240,176,128, 48,160, 96,192, 80,144,  0,112,
    0,240,112, 64,224, 32,208, 16,160, 96,192,176,144, 80, 48,128,
    64, 16,224,128,208, 96, 32,176,240,192,144,112, 48,160, 80,  0,
    240,192,128, 32, 64,144, 16,112, 80,176, 48,224,160,  0, 96,208,
    15,  1,  8, 14,  6, 11,  3,  4,  9,  7,  2, 13, 12,  0,  5, 10,
    3, 13,  4,  7, 15,  2,  8, 14, 12,  0,  1, 10,  6,  9, 11,  5,
    0, 14,  7, 11, 10,  4, 13,  1,  5,  8, 12,  6,  9,  3,  2, 15,
    13,  8, 10,  1,  3, 15,  4,  2, 11,  6,  7, 12,  0,  5, 14,  9,
    160,  0,144,224, 96, 48,240, 80, 16,208,192,112,176, 64, 32,128,
    208,112,  0,144, 48, 64, 96,160, 32,128, 80,224,192,176,240, 16,
    208, 96, 64,144,128,240, 48,  0,176, 16, 32,192, 80,160,224,112,
    16,160,208,  0, 96,144,128,112, 64,240,224, 48,176, 80, 32,192,
    7, 13, 14,  3,  0,  6,  9, 10,  1,  2,  8,  5, 11, 12,  4, 15,
    13,  8, 11,  5,  6, 15,  0,  3,  4,  7,  2, 12,  1, 10, 14,  9,
    10,  6,  9,  0, 12, 11,  7, 13, 15,  1,  3, 14,  5,  2,  8,  4,
    3, 15,  0,  6, 10,  1, 13,  8,  9,  4,  5, 11, 12,  7,  2, 14,
    32,192, 64, 16,112,160,176, 96,128, 80, 48,240,208,  0,224,144,
    224,176, 32,192, 64,112,208, 16, 80,  0,240,160, 48,144,128, 96,
    64, 32, 16,176,160,208,112,128,240,144,192, 80, 96, 48,  0,224,
    176,128,192,112, 16,224, 32,208, 96,240,  0,144,160, 64, 80, 48,
    12,  1, 10, 15,  9,  2,  6,  8,  0, 13,  3,  4, 14,  7,  5, 11,
    10, 15,  4,  2,  7, 12,  9,  5,  6,  1, 13, 14,  0, 11,  3,  8,
    9, 14, 15,  5,  2,  8, 12,  3,  7,  0,  4, 10,  1, 13, 11,  6,
    4,  3,  2, 12,  9,  5, 15, 10, 11, 14,  1,  7,  6,  0,  8, 13,
    64,176, 32,224,240,  0,128,208, 48,192,144,112, 80,160, 96, 16,
    208,  0,176,112, 64,144, 16,160,224, 48, 80,192, 32,240,128, 96,
    16, 64,176,208,192, 48,112,224,160,240, 96,128,  0, 80,144, 32,
    96,176,208,128, 16, 64,160,112,144, 80,  0,240,224, 32, 48,192,
    13,  2,  8,  4,  6, 15, 11,  1, 10,  9,  3, 14,  5,  0, 12,  7,
    1, 15, 13,  8, 10,  3,  7,  4, 12,  5,  6, 11,  0, 14,  9,  2,
    7, 11,  4,  1,  9, 12, 14,  2,  0,  6, 10, 13, 15,  3,  5,  8,
    2,  1, 14,  7,  4, 10,  8, 13, 15, 12,  9,  0,  3,  5,  6, 11
};

static void ECB_F(unsigned char *Rightn, unsigned char *key)
{

    unsigned char key2[8], data[8];
    unsigned char tmp;
    int i;

    ECB_Expand(Rightn);
    key2[0] = (key[0] >> 2);
    key2[1] = (key[1] >> 4) | ((key[0] << 4) & 0x30);
    key2[2] = (key[2] >> 6) | ((key[1] << 2) & 0x3f);
    key2[3] = (key[2] & 0x3f);
    key2[4] = (key[3] >> 2);
    key2[5] = (key[4] >> 4) | ((key[3] << 4) & 0x30);
    key2[6] = (key[5] >> 6) | ((key[4] << 2) & 0x3f);
    key2[7] = (key[5] & 0x3f);

    for (i=0; i<8; i++) {
        tmp = (key2[i] ^ Rightn[i]) & 0x3f;
        data[i] = ECB_S[(i*64) + 16*(((tmp >> 4) & 2) | (tmp & 1)) + ((tmp >> 1) & 0x0f)];
    }
    Rightn[0] = data[0] | data[1];
    Rightn[1] = data[2] | data[3];
    Rightn[2] = data[4] | data[5];
    Rightn[3] = data[6] | data[7];

    ECB_Permute(Rightn);
}

static void ECB_RoRKey(unsigned char *key)
{

    unsigned char tmp1, tmp2;

    tmp1 = key[3];
    key[3] = (key[3] >> 1) | (key[2] << 7);
    key[2] = (key[2] >> 1) | (key[1] << 7);
    key[1] = (key[1] >> 1) | (key[0] << 7);
    key[0] = (key[0] >> 1) | ((tmp1 << 3) & 0x80);
    tmp2 = key[6];
    key[6] = (key[6] >> 1) | (key[5] << 7);
    key[5] = (key[5] >> 1) | (key[4] << 7);
    key[4] = (key[4] >> 1) | (tmp1 << 7);
    key[3] = (key[3] & 0xf7) | ((tmp2 << 3) & 8);
}

static void ECB_RoLKey(unsigned char *key)
{

    unsigned char tmp1, tmp2;

    tmp1 = key[0];
    key[0] = (key[0] << 1) | (key[1] >> 7);
    key[1] = (key[1] << 1) | (key[2] >> 7);
    key[2] = (key[2] << 1) | (key[3] >> 7);
    tmp2 = key[3];
    key[3] = ((key[3] << 1) & 0xef) | ((tmp1 >> 3) & 0x10);
    key[3] = (key[3] & 0xfe) | (key[4] >> 7);
    key[4] = (key[4] << 1) | (key[5] >> 7);
    key[5] = (key[5] << 1) | (key[6] >> 7);
    key[6] = (key[6] << 1) | ((tmp2 >> 3) & 0x01);
}


const unsigned char SLt[16] = {
    1,1,2,2,2,2,2,2,1,2,2,2,2,2,2,1
};

const unsigned char SRt[16] = {
    1,2,2,2,2,2,2,1,2,2,2,2,2,2,1,1
};

static void ecb_des(unsigned char *data, unsigned char *ikey, const unsigned char de)
{

    int i,j;
    unsigned char TRightn[4], Leftn[4], Rightn[8], key2[7];
    unsigned char key[8];

    memcpy(key, ikey, 8);

    ECB_pc_1(key);
    ECB_IP(data);

    memcpy(Leftn, data, 4);
    memcpy(Rightn, data+4, 4);

    for (i=0; i<16; i++) {
        if (!de) for (j=0; j<SLt[i]; j++) ECB_RoLKey(key);

        ECB_PC2(key,key2);
        memcpy(TRightn, Rightn, 4);
        ECB_F(Rightn,key2);

        for (j=0; j<4; j++) {
            Rightn[j] ^= Leftn[j];
            Leftn[j] = TRightn[j];
        }
        if (de) for (j=0; j<SRt[i]; j++) ECB_RoRKey(key);
    }

    memcpy(data, Rightn, 4);
    memcpy(data+4, Leftn, 4);

    ECB_FP(data);

}

static void ecb_3des(unsigned char *data, unsigned char *ikey, const unsigned char de_en)
{

    unsigned char tmpkey[8];

    memcpy(tmpkey, ikey, 8);
    ecb_des(data, tmpkey, de_en ? 1 : 0);
    memcpy(tmpkey, ikey+8, 8);
    ecb_des(data, tmpkey, de_en ? 0 : 1);
    memcpy(tmpkey, ikey, 8);
    ecb_des(data, tmpkey, de_en ? 1 : 0);

}
