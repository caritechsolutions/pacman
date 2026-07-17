#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <ctype.h>
#include <stdbool.h>
#include <getopt.h>
#include <string.h>
#include <sys/stat.h>


#define roundup(x, y) (					\
{							\
	const typeof(y) __y = y;			\
	(((x) + (__y - 1)) / __y) * __y;		\
}							\
)

/*
	T		data_size(byte)		ecc_size(byte)	
	4		512			7
	8		512			14
	12		512			21
	16		512			28
	26		1024			46
	30		1024			53
	32		1024			56
*/

/* This function comes from CDD department. */
void nand_ecc_cal(unsigned char *data, int data_size, int ecc_size, unsigned char *ecc_data)
{
	unsigned int data_bit[8192];
	int n;
	unsigned int CheckBits[448];/*ecc_size*8*/
	unsigned int ecc_bit[448];
	int i;
	unsigned int tmp[4];

	for (i=0; i<4; i++)
		tmp[i] = 0;

	for (i=0; i<448; i++) {
		CheckBits[i] = 0;
		ecc_bit[i] = 0;
	}

	//Left-MSB
	for (i=0; i<data_size; i++) {
		for(n=0; n<8; n++)
			data_bit[i*8+n] = (data[i] >> (7-n)) & 1;
	}

	for(i=0; i<data_size*2; i++) {
		if(i%2 == 0) {
			tmp[0] = data_bit[i*4+7];
			tmp[1] = data_bit[i*4+6];
			tmp[2] = data_bit[i*4+5];
			tmp[3] = data_bit[i*4+4];
		} else {
			tmp[0] = data_bit[i*4-1];
			tmp[1] = data_bit[i*4-2];
			tmp[2] = data_bit[i*4-3];
			tmp[3] = data_bit[i*4-4];
		}

		if (ecc_size == 56) {
			CheckBits[0] = ecc_bit[1]^ecc_bit[4]^tmp[1];
			CheckBits[1] = (ecc_bit[0]^ecc_bit[2])^(ecc_bit[5]^tmp[0])^tmp[2];
			CheckBits[2] = (ecc_bit[1]^ecc_bit[3])^(ecc_bit[6]^tmp[1])^tmp[3];
			CheckBits[3] = (ecc_bit[0]^ecc_bit[1])^(ecc_bit[2]^ecc_bit[7])^(tmp[0]^tmp[1])^tmp[2];
			CheckBits[4] = (ecc_bit[1]^ecc_bit[2])^(ecc_bit[3]^ecc_bit[8])^(tmp[1]^tmp[2])^tmp[3];
			CheckBits[5] = (ecc_bit[0]^ecc_bit[1])^(ecc_bit[2]^ecc_bit[3])^(ecc_bit[9]^tmp[0])^(tmp[1]^tmp[2])^tmp[3];
			CheckBits[6] = (ecc_bit[2]^ecc_bit[3])^(ecc_bit[10]^tmp[2])^tmp[3];
			CheckBits[7] = (ecc_bit[1]^ecc_bit[3])^(ecc_bit[11]^tmp[1])^tmp[3];
			CheckBits[8] = (ecc_bit[1]^ecc_bit[2])^(ecc_bit[12]^tmp[1])^tmp[2];
			CheckBits[9] = (ecc_bit[2]^ecc_bit[3])^(ecc_bit[13]^tmp[2])^tmp[3];
			CheckBits[10] = (ecc_bit[0]^ecc_bit[1])^(ecc_bit[3]^ecc_bit[14])^(tmp[0]^tmp[1])^tmp[3];
			CheckBits[11] = (ecc_bit[0]^ecc_bit[2])^(ecc_bit[15]^tmp[0])^tmp[2];
			CheckBits[12] = (ecc_bit[1]^ecc_bit[3])^(ecc_bit[16]^tmp[1])^tmp[3];
			CheckBits[13] = (ecc_bit[1]^ecc_bit[2])^(ecc_bit[17]^tmp[1])^tmp[2];
			CheckBits[14] = (ecc_bit[2]^ecc_bit[3])^(ecc_bit[18]^tmp[2])^tmp[3];
			CheckBits[15] = (ecc_bit[1]^ecc_bit[3])^(ecc_bit[19]^tmp[1])^tmp[3];
			CheckBits[16] = (ecc_bit[0]^ecc_bit[1])^(ecc_bit[2]^ecc_bit[20])^(tmp[0]^tmp[1])^ tmp[2];
			CheckBits[17] = (ecc_bit[0]^ecc_bit[1])^(ecc_bit[2]^ecc_bit[3])^(ecc_bit[21]^tmp[0])^(tmp[1]^tmp[2])^tmp[3];
			CheckBits[18] = (ecc_bit[0]^ecc_bit[2])^(ecc_bit[3]^ecc_bit[22])^(tmp[0]^tmp[2])^ tmp[3];
			CheckBits[19] = (ecc_bit[0]^ecc_bit[3])^(ecc_bit[23]^tmp[0])^tmp[3];
			CheckBits[20] = ecc_bit[0]^ecc_bit[24]^tmp[0];
			CheckBits[21] = ecc_bit[1]^ecc_bit[25]^tmp[1];
			CheckBits[22] = ecc_bit[2]^ecc_bit[26]^tmp[2];
			CheckBits[23] = (ecc_bit[0]^ecc_bit[3])^(ecc_bit[27]^tmp[0])^tmp[3];
			CheckBits[24] = ecc_bit[28];
			CheckBits[25] = ecc_bit[29];
			CheckBits[26] = ecc_bit[30];
			CheckBits[27] = ecc_bit[31];
			CheckBits[28] = ecc_bit[0]^ecc_bit[32]^tmp[0];
			CheckBits[29] = ecc_bit[1]^ecc_bit[33]^tmp[1];
			CheckBits[30] = (ecc_bit[0]^ecc_bit[2])^(ecc_bit[34]^tmp[ 0])^tmp[2];
			CheckBits[31] = (ecc_bit[1]^ecc_bit[3])^(ecc_bit[35]^tmp[ 1])^tmp[3];
			CheckBits[32] = (ecc_bit[1]^ecc_bit[2])^(ecc_bit[36]^tmp[ 1])^tmp[2];
			CheckBits[33] = (ecc_bit[0]^ecc_bit[2])^(ecc_bit[ 3]^ecc_bit[37])^(tmp[0]^tmp[2])^tmp[3];
			CheckBits[34] = (ecc_bit[0]^ecc_bit[3])^(ecc_bit[38]^tmp[ 0])^tmp[3];
			CheckBits[35] = ecc_bit[0]^ecc_bit[39]^tmp[0];
			CheckBits[36] = ecc_bit[1]^ecc_bit[40]^tmp[1];
			CheckBits[37] = (ecc_bit[0]^ecc_bit[ 2])^(ecc_bit[41]^tmp[ 0])^ tmp[ 2];
			CheckBits[38] = (ecc_bit[0]^ecc_bit[ 1])^(ecc_bit[ 3]^ecc_bit[42])^(tmp[ 0]^tmp[1])^tmp[3];
			CheckBits[39] = (ecc_bit[0]^ecc_bit[ 2])^(ecc_bit[43]^tmp[ 0])^ tmp[ 2];
			CheckBits[40] = (ecc_bit[1]^ecc_bit[ 3])^(ecc_bit[44]^tmp[ 1])^ tmp[ 3];
			CheckBits[41] = (ecc_bit[0]^ecc_bit[ 1])^(ecc_bit[ 2]^ecc_bit[45])^(tmp[ 0]^tmp[1])^ tmp[2];
			CheckBits[42] = (ecc_bit[1]^ecc_bit[ 2])^(ecc_bit[ 3]^ecc_bit[46])^(tmp[ 1]^tmp[2])^ tmp[3];
			CheckBits[43] = (ecc_bit[0]^ecc_bit[ 1])^(ecc_bit[ 2]^ecc_bit[ 3])^(ecc_bit[47]^tmp[0])^(tmp[1]^tmp[2])^tmp[3];
			CheckBits[44] = (ecc_bit[0]^ecc_bit[ 2])^(ecc_bit[ 3]^ecc_bit[48])^(tmp[ 0]^tmp[2])^ tmp[3];
			CheckBits[45] = ecc_bit[3]^ecc_bit[49]^tmp[ 3];
			CheckBits[46] = ecc_bit[0]^ecc_bit[1]^ecc_bit[50]^tmp[0]^tmp[1];
			CheckBits[47] = ecc_bit[0]^ecc_bit[1]^ecc_bit[2]^ecc_bit[51]^tmp[0]^tmp[1]^tmp[2];
			CheckBits[48] = ecc_bit[0]^ecc_bit[1]^ecc_bit[2]^ecc_bit[3]^ecc_bit[52]^tmp[0]^tmp[1]^tmp[2]^tmp[3];
			CheckBits[49] = ecc_bit[0]^ecc_bit[2]^ecc_bit[3]^ecc_bit[53]^tmp[0]^tmp[2]^tmp[3];
			CheckBits[50] = ecc_bit[3]^ecc_bit[54]^tmp[3];
			CheckBits[51] = ecc_bit[0]^ecc_bit[1]^ecc_bit[55]^tmp[0]^tmp[1];
			CheckBits[52] = ecc_bit[0]^ecc_bit[1]^ecc_bit[2]^ecc_bit[56]^tmp[0]^tmp[1]^tmp[2];
			CheckBits[53] = ecc_bit[1]^ecc_bit[2]^ecc_bit[3]^ecc_bit[57]^tmp[1]^tmp[2]^tmp[3];
			CheckBits[54] = ecc_bit[0]^ecc_bit[1]^ecc_bit[2]^ecc_bit[3]^ecc_bit[58]^tmp[0]^tmp[1]^tmp[2]^tmp[3];
			CheckBits[55] = ecc_bit[0]^ecc_bit[2]^ecc_bit[3]^ecc_bit[59]^tmp[0]^tmp[2]^tmp[3];
			CheckBits[56] = ecc_bit[0]^ecc_bit[3]^ecc_bit[60]^tmp[0]^tmp[3];
			CheckBits[57] = ecc_bit[0]^ecc_bit[61]^tmp[0];
			CheckBits[58] = ecc_bit[0]^ecc_bit[1]^ecc_bit[62]^tmp[0]^tmp[1];
			CheckBits[59] = ecc_bit[0]^ecc_bit[1]^ecc_bit[2]^ecc_bit[63]^tmp[0]^tmp[1]^tmp[2];
			CheckBits[60] = ecc_bit[1]^ecc_bit[2]^ecc_bit[3]^ecc_bit[64]^tmp[1]^tmp[2]^tmp[3];
			CheckBits[61] = ecc_bit[0]^ecc_bit[1]^ecc_bit[2]^ecc_bit[3]^ecc_bit[65]^tmp[0]^tmp[1]^tmp[2]^tmp[3];
			CheckBits[62] = ecc_bit[2]^ecc_bit[3]^ecc_bit[66]^tmp[2]^tmp[3];
			CheckBits[63] = ecc_bit[1]^ecc_bit[3]^ecc_bit[67]^tmp[1]^tmp[3];
			CheckBits[64] = ecc_bit[0]^ecc_bit[1]^ecc_bit[2]^ecc_bit[68]^tmp[0]^tmp[1]^tmp[2];
			CheckBits[65] = ecc_bit[0]^ecc_bit[1]^ecc_bit[2]^ecc_bit[3]^ecc_bit[69]^tmp[0]^tmp[1]^tmp[2]^tmp[3];
			CheckBits[66] = ecc_bit[0]^ecc_bit[2]^ecc_bit[3]^ecc_bit[70]^tmp[0]^tmp[2]^tmp[3];
			CheckBits[67] = ecc_bit[3]^ecc_bit[71]^tmp[3];
			CheckBits[68] = ecc_bit[1]^ecc_bit[72]^tmp[1];
			CheckBits[69] = ecc_bit[0]^ecc_bit[2]^ecc_bit[73]^tmp[0]^tmp[2];
			CheckBits[70] = ecc_bit[0]^ecc_bit[1]^ecc_bit[3]^ecc_bit[74]^tmp[0]^tmp[1]^tmp[3];
			CheckBits[71] = ecc_bit[0]^ecc_bit[2]^ecc_bit[75]^tmp[0]^tmp[2];
			CheckBits[72] = ecc_bit[0]^ecc_bit[1]^ecc_bit[3]^ecc_bit[76]^tmp[0]^tmp[1]^tmp[3];
			CheckBits[73] = ecc_bit[0]^ecc_bit[2]^ecc_bit[77]^tmp[0]^tmp[2];
			CheckBits[74] = ecc_bit[1]^ecc_bit[3]^ecc_bit[78]^tmp[1]^tmp[3];
			CheckBits[75] = ecc_bit[0]^ecc_bit[1]^ecc_bit[2]^ecc_bit[79]^tmp[0]^tmp[1]^tmp[2];
			CheckBits[76] = ecc_bit[1]^ecc_bit[2]^ecc_bit[3]^ecc_bit[80]^tmp[1]^tmp[2]^tmp[3];
			CheckBits[77] = ecc_bit[1]^ecc_bit[2]^ecc_bit[3]^ecc_bit[81]^tmp[1]^tmp[2]^tmp[3];
			CheckBits[78] = ecc_bit[1]^ecc_bit[2]^ecc_bit[3]^ecc_bit[82]^tmp[1]^tmp[2]^tmp[3];
			CheckBits[79] = ecc_bit[1]^ecc_bit[2]^ecc_bit[3]^ecc_bit[83]^tmp[1]^tmp[2]^tmp[3];
			CheckBits[80] = ecc_bit[1]^ecc_bit[2]^ecc_bit[3]^ecc_bit[84]^tmp[1]^tmp[2]^tmp[3];
			CheckBits[81] = ecc_bit[0]^ecc_bit[1]^ecc_bit[2]^ecc_bit[3]^ecc_bit[85]^tmp[0]^tmp[1]^tmp[2]^tmp[3];
			CheckBits[82] = ecc_bit[2]^ecc_bit[3]^ecc_bit[86]^tmp[2]^tmp[3];
			CheckBits[83] = ecc_bit[0]^ecc_bit[1]^ecc_bit[3]^ecc_bit[87]^tmp[0]^tmp[1]^tmp[3];
			CheckBits[84] = ecc_bit[0]^ecc_bit[2]^ecc_bit[88]^tmp[0]^tmp[2];
			CheckBits[85] = ecc_bit[1]^ecc_bit[3]^ecc_bit[89]^tmp[1]^tmp[3];
			CheckBits[86] = ecc_bit[1]^ecc_bit[2]^ecc_bit[90]^tmp[1]^tmp[2];
			CheckBits[87] = ecc_bit[2]^ecc_bit[3]^ecc_bit[91]^tmp[2]^tmp[3];
			CheckBits[88] = ecc_bit[1]^ecc_bit[3]^ecc_bit[92]^tmp[1]^tmp[3];
			CheckBits[89] = ecc_bit[1]^ecc_bit[2]^ecc_bit[93]^tmp[1]^tmp[2];
			CheckBits[90] = ecc_bit[2]^ecc_bit[3]^ecc_bit[94]^tmp[2]^tmp[3];
			CheckBits[91] = ecc_bit[0]^ecc_bit[1]^ecc_bit[3]^ecc_bit[95]^tmp[0]^tmp[1]^tmp[3];
			CheckBits[92] = ecc_bit[0]^ecc_bit[2]^ecc_bit[96]^tmp[0]^tmp[2];
			CheckBits[93] = ecc_bit[1]^ecc_bit[3]^ecc_bit[97]^tmp[1]^tmp[3];
			CheckBits[94] = ecc_bit[1]^ecc_bit[2]^ecc_bit[98]^tmp[1]^tmp[2];
			CheckBits[95] = ecc_bit[2]^ecc_bit[3]^ecc_bit[99]^tmp[2]^tmp[3];
			CheckBits[96] = ecc_bit[0]^ecc_bit[1]^ecc_bit[3]^ecc_bit[100]^tmp[0]^tmp[1]^tmp[3];
			CheckBits[97] = ecc_bit[2]^ecc_bit[101]^tmp[2];
			CheckBits[98] = ecc_bit[0]^ecc_bit[3]^ecc_bit[102]^tmp[0]^tmp[3];
			CheckBits[99] = ecc_bit[103];
			CheckBits[100] = ecc_bit[0]^ecc_bit[104]^tmp[0];
			CheckBits[101] = ecc_bit[1]^ecc_bit[105]^tmp[1];
			CheckBits[102] = ecc_bit[2]^ecc_bit[106]^tmp[2];
			CheckBits[103] = ecc_bit[0]^ecc_bit[3]^ecc_bit[107]^tmp[0]^tmp[3];
			CheckBits[104] = ecc_bit[108];
			CheckBits[105] = ecc_bit[109];
			CheckBits[106] = ecc_bit[0]^ecc_bit[110]^tmp[0];
			CheckBits[107] = ecc_bit[0]^ecc_bit[1]^ecc_bit[111]^tmp[0]^tmp[1];
			CheckBits[108] = ecc_bit[0]^ecc_bit[1]^ecc_bit[2]^ecc_bit[112]^tmp[0]^tmp[1]^tmp[2];
			CheckBits[109] = ecc_bit[1]^ecc_bit[2]^ecc_bit[3]^ecc_bit[113]^tmp[1]^tmp[2]^tmp[3];
			CheckBits[110] = ecc_bit[0]^ecc_bit[1]^ecc_bit[2]^ecc_bit[3]^ecc_bit[114]^tmp[0]^tmp[1]^tmp[2]^tmp[3];
			CheckBits[111] = ecc_bit[0]^ecc_bit[2]^ecc_bit[3]^ecc_bit[115]^tmp[0]^tmp[2]^tmp[3];
			CheckBits[112] = ecc_bit[3]^ecc_bit[116]^tmp[3];
			CheckBits[113] = ecc_bit[0]^ecc_bit[1]^ecc_bit[117]^tmp[0]^tmp[1];
			CheckBits[114] = ecc_bit[1]^ecc_bit[2]^ecc_bit[118]^tmp[1]^tmp[2];
			CheckBits[115] = ecc_bit[0]^ecc_bit[2]^ecc_bit[3]^ecc_bit[119]^tmp[0]^tmp[2]^tmp[3];
			CheckBits[116] = ecc_bit[3]^ecc_bit[120]^tmp[3];
			CheckBits[117] = ecc_bit[0]^ecc_bit[1]^ecc_bit[121]^tmp[0]^tmp[1];
			CheckBits[118] = ecc_bit[1]^ecc_bit[2]^ecc_bit[122]^tmp[1]^tmp[2];
			CheckBits[119] = ecc_bit[2]^ecc_bit[3]^ecc_bit[123]^tmp[2]^tmp[3];
			CheckBits[120] = ecc_bit[1]^ecc_bit[3]^ecc_bit[124]^tmp[1]^tmp[3];
			CheckBits[121] = ecc_bit[1]^ecc_bit[2]^ecc_bit[125]^tmp[1]^tmp[2];
			CheckBits[122] = ecc_bit[0]^ecc_bit[2]^ecc_bit[3]^ecc_bit[126]^tmp[0]^tmp[2]^tmp[3];
			CheckBits[123] = ecc_bit[0]^ecc_bit[3]^ecc_bit[127]^tmp[0]^tmp[3];
			CheckBits[124] = ecc_bit[128];
			CheckBits[125] = ecc_bit[0]^ecc_bit[129]^tmp[0];
			CheckBits[126] = ecc_bit[0]^ecc_bit[1]^ecc_bit[130]^tmp[0]^tmp[1];
			CheckBits[127] = ecc_bit[0]^ecc_bit[1]^ecc_bit[2]^ecc_bit[131]^tmp[0]^tmp[1]^tmp[2];
			CheckBits[128] = ecc_bit[1]^ecc_bit[2]^ecc_bit[3]^ecc_bit[132]^tmp[1]^tmp[2]^tmp[3];
			CheckBits[129] = ecc_bit[0]^ecc_bit[1]^ecc_bit[2]^ecc_bit[3]^ecc_bit[133]^tmp[0]^tmp[1]^tmp[2]^tmp[3];
			CheckBits[130] = ecc_bit[0]^ecc_bit[2]^ecc_bit[3]^ecc_bit[134]^tmp[0]^tmp[2]^tmp[3];
			CheckBits[131] = ecc_bit[0]^ecc_bit[3]^ecc_bit[135]^tmp[0]^tmp[3];
			CheckBits[132] = ecc_bit[136];
			CheckBits[133] = ecc_bit[137];
			CheckBits[134] = ecc_bit[0]^ecc_bit[138]^tmp[0];
			CheckBits[135] = ecc_bit[1]^ecc_bit[139]^tmp[1];
			CheckBits[136] = ecc_bit[2]^ecc_bit[140]^tmp[2];
			CheckBits[137] = ecc_bit[0]^ecc_bit[3]^ecc_bit[141]^tmp[0]^tmp[3];
			CheckBits[138] = ecc_bit[0]^ecc_bit[142]^tmp[0];
			CheckBits[139] = ecc_bit[0]^ecc_bit[1]^ecc_bit[143]^tmp[0]^tmp[1];
			CheckBits[140] = ecc_bit[0]^ecc_bit[1]^ecc_bit[2]^ecc_bit[144]^tmp[0]^tmp[1]^tmp[2];
			CheckBits[141] = ecc_bit[1]^ecc_bit[2]^ecc_bit[3]^ecc_bit[145]^tmp[1]^tmp[2]^tmp[3];
			CheckBits[142] = ecc_bit[0]^ecc_bit[1]^ecc_bit[2]^ecc_bit[3]^ecc_bit[146]^tmp[0]^tmp[1]^tmp[2]^tmp[3];
			CheckBits[143] = ecc_bit[2]^ecc_bit[3]^ecc_bit[147]^tmp[2]^tmp[3];
			CheckBits[144] = ecc_bit[0]^ecc_bit[1]^ecc_bit[3]^ecc_bit[148]^tmp[0]^tmp[1]^tmp[3];
			CheckBits[145] = ecc_bit[0]^ecc_bit[2]^ecc_bit[149]^tmp[0]^tmp[2];
			CheckBits[146] = ecc_bit[1]^ecc_bit[3]^ecc_bit[150]^tmp[1]^tmp[3];
			CheckBits[147] = ecc_bit[0]^ecc_bit[1]^ecc_bit[2]^ecc_bit[151]^tmp[0]^tmp[1]^tmp[2];
			CheckBits[148] = ecc_bit[1]^ecc_bit[2]^ecc_bit[3]^ecc_bit[152]^tmp[1]^tmp[2]^tmp[3];
			CheckBits[149] = ecc_bit[0]^ecc_bit[1]^ecc_bit[2]^ecc_bit[3]^ecc_bit[153]^tmp[0]^tmp[1]^tmp[2]^tmp[3];
			CheckBits[150] = ecc_bit[2]^ecc_bit[3]^ecc_bit[154]^tmp[2]^tmp[3];
			CheckBits[151] = ecc_bit[1]^ecc_bit[3]^ecc_bit[155]^tmp[1]^tmp[3];
			CheckBits[152] = ecc_bit[1]^ecc_bit[2]^ecc_bit[156]^tmp[1]^tmp[2];
			CheckBits[153] = ecc_bit[0]^ecc_bit[2]^ecc_bit[3]^ecc_bit[157]^tmp[0]^tmp[2]^tmp[3];
			CheckBits[154] = ecc_bit[0]^ecc_bit[3]^ecc_bit[158]^tmp[0]^tmp[3];
			CheckBits[155] = ecc_bit[0]^ecc_bit[159]^tmp[0];
			CheckBits[156] = ecc_bit[1]^ecc_bit[160]^tmp[1];
			CheckBits[157] = ecc_bit[0]^ecc_bit[2]^ecc_bit[161]^tmp[0]^tmp[2];
			CheckBits[158] = ecc_bit[1]^ecc_bit[3]^ecc_bit[162]^tmp[1]^tmp[3];
			CheckBits[159] = ecc_bit[1]^ecc_bit[2]^ecc_bit[163]^tmp[1]^tmp[2];
			CheckBits[160] = ecc_bit[2]^ecc_bit[3]^ecc_bit[164]^tmp[2]^tmp[3];
			CheckBits[161] = ecc_bit[0]^ecc_bit[1]^ecc_bit[3]^ecc_bit[165]^tmp[0]^tmp[1]^tmp[3];
			CheckBits[162] = ecc_bit[0]^ecc_bit[2]^ecc_bit[166]^tmp[0]^tmp[2];
			CheckBits[163] = ecc_bit[0]^ecc_bit[1]^ecc_bit[3]^ecc_bit[167]^tmp[0]^tmp[1]^tmp[3];
			CheckBits[164] = ecc_bit[2]^ecc_bit[168]^tmp[2];
			CheckBits[165] = ecc_bit[3]^ecc_bit[169]^tmp[3];
			CheckBits[166] = ecc_bit[1]^ecc_bit[170]^tmp[1];
			CheckBits[167] = ecc_bit[0]^ecc_bit[2]^ecc_bit[171]^tmp[0]^tmp[2];
			CheckBits[168] = ecc_bit[0]^ecc_bit[1]^ecc_bit[3]^ecc_bit[172]^tmp[0]^tmp[1]^tmp[3];
			CheckBits[169] = ecc_bit[2]^ecc_bit[173]^tmp[2];
			CheckBits[170] = ecc_bit[0]^ecc_bit[3]^ecc_bit[174]^tmp[0]^tmp[3];
			CheckBits[171] = ecc_bit[175];
			CheckBits[172] = ecc_bit[0]^ecc_bit[176]^tmp[0];
			CheckBits[173] = ecc_bit[0]^ecc_bit[1]^ecc_bit[177]^tmp[0]^tmp[1];
			CheckBits[174] = ecc_bit[1]^ecc_bit[2]^ecc_bit[178]^tmp[1]^tmp[2];
			CheckBits[175] = ecc_bit[0]^ecc_bit[2]^ecc_bit[3]^ecc_bit[179]^tmp[0]^tmp[2]^tmp[3];
			CheckBits[176] = ecc_bit[0]^ecc_bit[3]^ecc_bit[180]^tmp[0]^tmp[3];
			CheckBits[177] = ecc_bit[181];
			CheckBits[178] = ecc_bit[182];
			CheckBits[179] = ecc_bit[183];
			CheckBits[180] = ecc_bit[0]^ecc_bit[184]^tmp[0];
			CheckBits[181] = ecc_bit[0]^ecc_bit[1]^ecc_bit[185]^tmp[0]^tmp[1];
			CheckBits[182] = ecc_bit[1]^ecc_bit[2]^ecc_bit[186]^tmp[1]^tmp[2];
			CheckBits[183] = ecc_bit[0]^ecc_bit[2]^ecc_bit[3]^ecc_bit[187]^tmp[0]^tmp[2]^tmp[3];
			CheckBits[184] = ecc_bit[3]^ecc_bit[188]^tmp[3];
			CheckBits[185] = ecc_bit[0]^ecc_bit[1]^ecc_bit[189]^tmp[0]^tmp[1];
			CheckBits[186] = ecc_bit[1]^ecc_bit[2]^ecc_bit[190]^tmp[1]^tmp[2];
			CheckBits[187] = ecc_bit[2]^ecc_bit[3]^ecc_bit[191]^tmp[2]^tmp[3];
			CheckBits[188] = ecc_bit[0]^ecc_bit[1]^ecc_bit[3]^ecc_bit[192]^tmp[0]^tmp[1]^tmp[3];
			CheckBits[189] = ecc_bit[2]^ecc_bit[193]^tmp[2];
			CheckBits[190] = ecc_bit[3]^ecc_bit[194]^tmp[3];
			CheckBits[191] = ecc_bit[0]^ecc_bit[1]^ecc_bit[195]^tmp[0]^tmp[1];
			CheckBits[192] = ecc_bit[1]^ecc_bit[2]^ecc_bit[196]^tmp[1]^tmp[2];
			CheckBits[193] = ecc_bit[2]^ecc_bit[3]^ecc_bit[197]^tmp[2]^tmp[3];
			CheckBits[194] = ecc_bit[1]^ecc_bit[3]^ecc_bit[198]^tmp[1]^tmp[3];
			CheckBits[195] = ecc_bit[1]^ecc_bit[2]^ecc_bit[199]^tmp[1]^tmp[2];
			CheckBits[196] = ecc_bit[0]^ecc_bit[2]^ecc_bit[3]^ecc_bit[200]^tmp[0]^tmp[2]^tmp[3];
			CheckBits[197] = ecc_bit[3]^ecc_bit[201]^tmp[3];
			CheckBits[198] = ecc_bit[0]^ecc_bit[1]^ecc_bit[202]^tmp[0]^tmp[1];
			CheckBits[199] = ecc_bit[1]^ecc_bit[2]^ecc_bit[203]^tmp[1]^tmp[2];
			CheckBits[200] = ecc_bit[2]^ecc_bit[3]^ecc_bit[204]^tmp[2]^tmp[3];
			CheckBits[201] = ecc_bit[1]^ecc_bit[3]^ecc_bit[205]^tmp[1]^tmp[3];
			CheckBits[202] = ecc_bit[1]^ecc_bit[2]^ecc_bit[206]^tmp[1]^tmp[2];
			CheckBits[203] = ecc_bit[0]^ecc_bit[2]^ecc_bit[3]^ecc_bit[207]^tmp[0]^tmp[2]^tmp[3];
			CheckBits[204] = ecc_bit[3]^ecc_bit[208]^tmp[3];
			CheckBits[205] = ecc_bit[1]^ecc_bit[209]^tmp[1];
			CheckBits[206] = ecc_bit[0]^ecc_bit[2]^ecc_bit[210]^tmp[0]^tmp[2];
			CheckBits[207] = ecc_bit[0]^ecc_bit[1]^ecc_bit[3]^ecc_bit[211]^tmp[0]^tmp[1]^tmp[3];
			CheckBits[208] = ecc_bit[0]^ecc_bit[2]^ecc_bit[212]^tmp[0]^tmp[2];
			CheckBits[209] = ecc_bit[1]^ecc_bit[3]^ecc_bit[213]^tmp[1]^tmp[3];
			CheckBits[210] = ecc_bit[1]^ecc_bit[2]^ecc_bit[214]^tmp[1]^tmp[2];
			CheckBits[211] = ecc_bit[0]^ecc_bit[2]^ecc_bit[3]^ecc_bit[215]^tmp[0]^tmp[2]^tmp[3];
			CheckBits[212] = ecc_bit[0]^ecc_bit[3]^ecc_bit[216]^tmp[0]^tmp[3];
			CheckBits[213] = ecc_bit[217];
			CheckBits[214] = ecc_bit[218];
			CheckBits[215] = ecc_bit[219];
			CheckBits[216] = ecc_bit[220];
			CheckBits[217] = ecc_bit[0]^ecc_bit[221]^tmp[0];
			CheckBits[218] = ecc_bit[0]^ecc_bit[1]^ecc_bit[222]^tmp[0]^tmp[1];
			CheckBits[219] = ecc_bit[1]^ecc_bit[2]^ecc_bit[223]^tmp[1]^tmp[2];
			CheckBits[220] = ecc_bit[2]^ecc_bit[3]^ecc_bit[224]^tmp[2]^tmp[3];
			CheckBits[221] = ecc_bit[1]^ecc_bit[3]^ecc_bit[225]^tmp[1]^tmp[3];
			CheckBits[222] = ecc_bit[0]^ecc_bit[1]^ecc_bit[2]^ecc_bit[226]^tmp[0]^tmp[1]^tmp[2];
			CheckBits[223] = ecc_bit[0]^ecc_bit[1]^ecc_bit[2]^ecc_bit[3]^ecc_bit[227]^tmp[0]^tmp[1]^tmp[2]^tmp[3];
			CheckBits[224] = ecc_bit[0]^ecc_bit[2]^ecc_bit[3]^ecc_bit[228]^tmp[0]^tmp[2]^tmp[3];
			CheckBits[225] = ecc_bit[0]^ecc_bit[3]^ecc_bit[229]^tmp[0]^tmp[3];
			CheckBits[226] = ecc_bit[0]^ecc_bit[230]^tmp[0];
			CheckBits[227] = ecc_bit[0]^ecc_bit[1]^ecc_bit[231]^tmp[0]^tmp[1];
			CheckBits[228] = ecc_bit[1]^ecc_bit[2]^ecc_bit[232]^tmp[1]^tmp[2];
			CheckBits[229] = ecc_bit[2]^ecc_bit[3]^ecc_bit[233]^tmp[2]^tmp[3];
			CheckBits[230] = ecc_bit[1]^ecc_bit[3]^ecc_bit[234]^tmp[1]^tmp[3];
			CheckBits[231] = ecc_bit[1]^ecc_bit[2]^ecc_bit[235]^tmp[1]^tmp[2];
			CheckBits[232] = ecc_bit[0]^ecc_bit[2]^ecc_bit[3]^ecc_bit[236]^tmp[0]^tmp[2]^tmp[3];
			CheckBits[233] = ecc_bit[3]^ecc_bit[237]^tmp[3];
			CheckBits[234] = ecc_bit[0]^ecc_bit[1]^ecc_bit[238]^tmp[0]^tmp[1];
			CheckBits[235] = ecc_bit[1]^ecc_bit[2]^ecc_bit[239]^tmp[1]^tmp[2];
			CheckBits[236] = ecc_bit[0]^ecc_bit[2]^ecc_bit[3]^ecc_bit[240]^tmp[0]^tmp[2]^tmp[3];
			CheckBits[237] = ecc_bit[0]^ecc_bit[3]^ecc_bit[241]^tmp[0]^tmp[3];
			CheckBits[238] = ecc_bit[0]^ecc_bit[242]^tmp[0];
			CheckBits[239] = ecc_bit[0]^ecc_bit[1]^ecc_bit[243]^tmp[0]^tmp[1];
			CheckBits[240] = ecc_bit[0]^ecc_bit[1]^ecc_bit[2]^ecc_bit[244]^tmp[0]^tmp[1]^tmp[2];
			CheckBits[241] = ecc_bit[0]^ecc_bit[1]^ecc_bit[2]^ecc_bit[3]^ecc_bit[245]^tmp[0]^tmp[1]^tmp[2]^tmp[3];
			CheckBits[242] = ecc_bit[2]^ecc_bit[3]^ecc_bit[246]^tmp[2]^tmp[3];
			CheckBits[243] = ecc_bit[1]^ecc_bit[3]^ecc_bit[247]^tmp[1]^tmp[3];
			CheckBits[244] = ecc_bit[0]^ecc_bit[1]^ecc_bit[2]^ecc_bit[248]^tmp[0]^tmp[1]^tmp[2];
			CheckBits[245] = ecc_bit[1]^ecc_bit[2]^ecc_bit[3]^ecc_bit[249]^tmp[1]^tmp[2]^tmp[3];
			CheckBits[246] = ecc_bit[0]^ecc_bit[1]^ecc_bit[2]^ecc_bit[3]^ecc_bit[250]^tmp[0]^tmp[1]^tmp[2]^tmp[3];
			CheckBits[247] = ecc_bit[0]^ecc_bit[2]^ecc_bit[3]^ecc_bit[251]^tmp[0]^tmp[2]^tmp[3];
			CheckBits[248] = ecc_bit[3]^ecc_bit[252]^tmp[3];
			CheckBits[249] = ecc_bit[0]^ecc_bit[1]^ecc_bit[253]^tmp[0]^tmp[1];
			CheckBits[250] = ecc_bit[1]^ecc_bit[2]^ecc_bit[254]^tmp[1]^tmp[2];
			CheckBits[251] = ecc_bit[0]^ecc_bit[2]^ecc_bit[3]^ecc_bit[255]^tmp[0]^tmp[2]^tmp[3];
			CheckBits[252] = ecc_bit[3]^ecc_bit[256]^tmp[3];
			CheckBits[253] = ecc_bit[0]^ecc_bit[1]^ecc_bit[257]^tmp[0]^tmp[1];
			CheckBits[254] = ecc_bit[0]^ecc_bit[1]^ecc_bit[2]^ecc_bit[258]^tmp[0]^tmp[1]^tmp[2];
			CheckBits[255] = ecc_bit[1]^ecc_bit[2]^ecc_bit[3]^ecc_bit[259]^tmp[1]^tmp[2]^tmp[3];
			CheckBits[256] = ecc_bit[1]^ecc_bit[2]^ecc_bit[3]^ecc_bit[260]^tmp[1]^tmp[2]^tmp[3];
			CheckBits[257] = ecc_bit[0]^ecc_bit[1]^ecc_bit[2]^ecc_bit[3]^ecc_bit[261]^tmp[0]^tmp[1]^tmp[2]^tmp[3];
			CheckBits[258] = ecc_bit[0]^ecc_bit[2]^ecc_bit[3]^ecc_bit[262]^tmp[0]^tmp[2]^tmp[3];
			CheckBits[259] = ecc_bit[0]^ecc_bit[3]^ecc_bit[263]^tmp[0]^tmp[3];
			CheckBits[260] = ecc_bit[0]^ecc_bit[264]^tmp[0];
			CheckBits[261] = ecc_bit[0]^ecc_bit[1]^ecc_bit[265]^tmp[0]^tmp[1];
			CheckBits[262] = ecc_bit[0]^ecc_bit[1]^ecc_bit[2]^ecc_bit[266]^tmp[0]^tmp[1]^tmp[2];
			CheckBits[263] = ecc_bit[0]^ecc_bit[1]^ecc_bit[2]^ecc_bit[3]^ecc_bit[267]^tmp[0]^tmp[1]^tmp[2]^tmp[3];
			CheckBits[264] = ecc_bit[2]^ecc_bit[3]^ecc_bit[268]^tmp[2]^tmp[3];
			CheckBits[265] = ecc_bit[1]^ecc_bit[3]^ecc_bit[269]^tmp[1]^tmp[3];
			CheckBits[266] = ecc_bit[1]^ecc_bit[2]^ecc_bit[270]^tmp[1]^tmp[2];
			CheckBits[267] = ecc_bit[2]^ecc_bit[3]^ecc_bit[271]^tmp[2]^tmp[3];
			CheckBits[268] = ecc_bit[0]^ecc_bit[1]^ecc_bit[3]^ecc_bit[272]^tmp[0]^tmp[1]^tmp[3];
			CheckBits[269] = ecc_bit[0]^ecc_bit[2]^ecc_bit[273]^tmp[0]^tmp[2];
			CheckBits[270] = ecc_bit[0]^ecc_bit[1]^ecc_bit[3]^ecc_bit[274]^tmp[0]^tmp[1]^tmp[3];
			CheckBits[271] = ecc_bit[0]^ecc_bit[2]^ecc_bit[275]^tmp[0]^tmp[2];
			CheckBits[272] = ecc_bit[1]^ecc_bit[3]^ecc_bit[276]^tmp[1]^tmp[3];
			CheckBits[273] = ecc_bit[1]^ecc_bit[2]^ecc_bit[277]^tmp[1]^tmp[2];
			CheckBits[274] = ecc_bit[0]^ecc_bit[2]^ecc_bit[3]^ecc_bit[278]^tmp[0]^tmp[2]^tmp[3];
			CheckBits[275] = ecc_bit[3]^ecc_bit[279]^tmp[3];
			CheckBits[276] = ecc_bit[1]^ecc_bit[280]^tmp[1];
			CheckBits[277] = ecc_bit[2]^ecc_bit[281]^tmp[2];
			CheckBits[278] = ecc_bit[3]^ecc_bit[282]^tmp[3];
			CheckBits[279] = ecc_bit[1]^ecc_bit[283]^tmp[1];
			CheckBits[280] = ecc_bit[2]^ecc_bit[284]^tmp[2];
			CheckBits[281] = ecc_bit[3]^ecc_bit[285]^tmp[3];
			CheckBits[282] = ecc_bit[1]^ecc_bit[286]^tmp[1];
			CheckBits[283] = ecc_bit[2]^ecc_bit[287]^tmp[2];
			CheckBits[284] = ecc_bit[3]^ecc_bit[288]^tmp[3];
			CheckBits[285] = ecc_bit[1]^ecc_bit[289]^tmp[1];
			CheckBits[286] = ecc_bit[0]^ecc_bit[2]^ecc_bit[290]^tmp[0]^tmp[2];
			CheckBits[287] = ecc_bit[0]^ecc_bit[1]^ecc_bit[3]^ecc_bit[291]^tmp[0]^tmp[1]^tmp[3];
			CheckBits[288] = ecc_bit[0]^ecc_bit[2]^ecc_bit[292]^tmp[0]^tmp[2];
			CheckBits[289] = ecc_bit[1]^ecc_bit[3]^ecc_bit[293]^tmp[1]^tmp[3];
			CheckBits[290] = ecc_bit[0]^ecc_bit[1]^ecc_bit[2]^ecc_bit[294]^tmp[0]^tmp[1]^tmp[2];
			CheckBits[291] = ecc_bit[0]^ecc_bit[1]^ecc_bit[2]^ecc_bit[3]^ecc_bit[295]^tmp[0]^tmp[1]^tmp[2]^tmp[3];
			CheckBits[292] = ecc_bit[0]^ecc_bit[2]^ecc_bit[3]^ecc_bit[296]^tmp[0]^tmp[2]^tmp[3];
			CheckBits[293] = ecc_bit[0]^ecc_bit[3]^ecc_bit[297]^tmp[0]^tmp[3];
			CheckBits[294] = ecc_bit[0]^ecc_bit[298]^tmp[0];
			CheckBits[295] = ecc_bit[0]^ecc_bit[1]^ecc_bit[299]^tmp[0]^tmp[1];
			CheckBits[296] = ecc_bit[0]^ecc_bit[1]^ecc_bit[2]^ecc_bit[300]^tmp[0]^tmp[1]^tmp[2];
			CheckBits[297] = ecc_bit[0]^ecc_bit[1]^ecc_bit[2]^ecc_bit[3]^ecc_bit[301]^tmp[0]^tmp[1]^tmp[2]^tmp[3];
			CheckBits[298] = ecc_bit[2]^ecc_bit[3]^ecc_bit[302]^tmp[2]^tmp[3];
			CheckBits[299] = ecc_bit[0]^ecc_bit[1]^ecc_bit[3]^ecc_bit[303]^tmp[0]^tmp[1]^tmp[3];
			CheckBits[300] = ecc_bit[0]^ecc_bit[2]^ecc_bit[304]^tmp[0]^tmp[2];
			CheckBits[301] = ecc_bit[0]^ecc_bit[1]^ecc_bit[3]^ecc_bit[305]^tmp[0]^tmp[1]^tmp[3];
			CheckBits[302] = ecc_bit[2]^ecc_bit[306]^tmp[2];
			CheckBits[303] = ecc_bit[3]^ecc_bit[307]^tmp[3];
			CheckBits[304] = ecc_bit[0]^ecc_bit[1]^ecc_bit[308]^tmp[0]^tmp[1];
			CheckBits[305] = ecc_bit[0]^ecc_bit[1]^ecc_bit[2]^ecc_bit[309]^tmp[0]^tmp[1]^tmp[2];
			CheckBits[306] = ecc_bit[1]^ecc_bit[2]^ecc_bit[3]^ecc_bit[310]^tmp[1]^tmp[2]^tmp[3];
			CheckBits[307] = ecc_bit[1]^ecc_bit[2]^ecc_bit[3]^ecc_bit[311]^tmp[1]^tmp[2]^tmp[3];
			CheckBits[308] = ecc_bit[0]^ecc_bit[1]^ecc_bit[2]^ecc_bit[3]^ecc_bit[312]^tmp[0]^tmp[1]^tmp[2]^tmp[3];
			CheckBits[309] = ecc_bit[0]^ecc_bit[2]^ecc_bit[3]^ecc_bit[313]^tmp[0]^tmp[2]^tmp[3];
			CheckBits[310] = ecc_bit[3]^ecc_bit[314]^tmp[3];
			CheckBits[311] = ecc_bit[1]^ecc_bit[315]^tmp[1];
			CheckBits[312] = ecc_bit[2]^ecc_bit[316]^tmp[2];
			CheckBits[313] = ecc_bit[3]^ecc_bit[317]^tmp[3];
			CheckBits[314] = ecc_bit[0]^ecc_bit[1]^ecc_bit[318]^tmp[0]^tmp[1];
			CheckBits[315] = ecc_bit[1]^ecc_bit[2]^ecc_bit[319]^tmp[1]^tmp[2];
			CheckBits[316] = ecc_bit[2]^ecc_bit[3]^ecc_bit[320]^tmp[2]^tmp[3];
			CheckBits[317] = ecc_bit[1]^ecc_bit[3]^ecc_bit[321]^tmp[1]^tmp[3];
			CheckBits[318] = ecc_bit[1]^ecc_bit[2]^ecc_bit[322]^tmp[1]^tmp[2];
			CheckBits[319] = ecc_bit[2]^ecc_bit[3]^ecc_bit[323]^tmp[2]^tmp[3];
			CheckBits[320] = ecc_bit[1]^ecc_bit[3]^ecc_bit[324]^tmp[1]^tmp[3];
			CheckBits[321] = ecc_bit[1]^ecc_bit[2]^ecc_bit[325]^tmp[1]^tmp[2];
			CheckBits[322] = ecc_bit[2]^ecc_bit[3]^ecc_bit[326]^tmp[2]^tmp[3];
			CheckBits[323] = ecc_bit[1]^ecc_bit[3]^ecc_bit[327]^tmp[1]^tmp[3];
			CheckBits[324] = ecc_bit[1]^ecc_bit[2]^ecc_bit[328]^tmp[1]^tmp[2];
			CheckBits[325] = ecc_bit[2]^ecc_bit[3]^ecc_bit[329]^tmp[2]^tmp[3];
			CheckBits[326] = ecc_bit[1]^ecc_bit[3]^ecc_bit[330]^tmp[1]^tmp[3];
			CheckBits[327] = ecc_bit[0]^ecc_bit[1]^ecc_bit[2]^ecc_bit[331]^tmp[0]^tmp[1]^tmp[2];
			CheckBits[328] = ecc_bit[1]^ecc_bit[2]^ecc_bit[3]^ecc_bit[332]^tmp[1]^tmp[2]^tmp[3];
			CheckBits[329] = ecc_bit[0]^ecc_bit[1]^ecc_bit[2]^ecc_bit[3]^ecc_bit[333]^tmp[0]^tmp[1]^tmp[2]^tmp[3];
			CheckBits[330] = ecc_bit[0]^ecc_bit[2]^ecc_bit[3]^ecc_bit[334]^tmp[0]^tmp[2]^tmp[3];
			CheckBits[331] = ecc_bit[0]^ecc_bit[3]^ecc_bit[335]^tmp[0]^tmp[3];
			CheckBits[332] = ecc_bit[0]^ecc_bit[336]^tmp[0];
			CheckBits[333] = ecc_bit[1]^ecc_bit[337]^tmp[1];
			CheckBits[334] = ecc_bit[2]^ecc_bit[338]^tmp[2];
			CheckBits[335] = ecc_bit[0]^ecc_bit[3]^ecc_bit[339]^tmp[0]^tmp[3];
			CheckBits[336] = ecc_bit[0]^ecc_bit[340]^tmp[0];
			CheckBits[337] = ecc_bit[0]^ecc_bit[1]^ecc_bit[341]^tmp[0]^tmp[1];
			CheckBits[338] = ecc_bit[0]^ecc_bit[1]^ecc_bit[2]^ecc_bit[342]^tmp[0]^tmp[1]^tmp[2];
			CheckBits[339] = ecc_bit[0]^ecc_bit[1]^ecc_bit[2]^ecc_bit[3]^ecc_bit[343]^tmp[0]^tmp[1]^tmp[2]^tmp[3];
			CheckBits[340] = ecc_bit[0]^ecc_bit[2]^ecc_bit[3]^ecc_bit[344]^tmp[0]^tmp[2]^tmp[3];
			CheckBits[341] = ecc_bit[3]^ecc_bit[345]^tmp[3];
			CheckBits[342] = ecc_bit[1]^ecc_bit[346]^tmp[1];
			CheckBits[343] = ecc_bit[0]^ecc_bit[2]^ecc_bit[347]^tmp[0]^tmp[2];
			CheckBits[344] = ecc_bit[1]^ecc_bit[3]^ecc_bit[348]^tmp[1]^tmp[3];
			CheckBits[345] = ecc_bit[0]^ecc_bit[1]^ecc_bit[2]^ecc_bit[349]^tmp[0]^tmp[1]^tmp[2];
			CheckBits[346] = ecc_bit[1]^ecc_bit[2]^ecc_bit[3]^ecc_bit[350]^tmp[1]^tmp[2]^tmp[3];
			CheckBits[347] = ecc_bit[0]^ecc_bit[1]^ecc_bit[2]^ecc_bit[3]^ecc_bit[351]^tmp[0]^tmp[1]^tmp[2]^tmp[3];
			CheckBits[348] = ecc_bit[2]^ecc_bit[3]^ecc_bit[352]^tmp[2]^tmp[3];
			CheckBits[349] = ecc_bit[0]^ecc_bit[1]^ecc_bit[3]^ecc_bit[353]^tmp[0]^tmp[1]^tmp[3];
			CheckBits[350] = ecc_bit[0]^ecc_bit[2]^ecc_bit[354]^tmp[0]^tmp[2];
			CheckBits[351] = ecc_bit[1]^ecc_bit[3]^ecc_bit[355]^tmp[1]^tmp[3];
			CheckBits[352] = ecc_bit[0]^ecc_bit[1]^ecc_bit[2]^ecc_bit[356]^tmp[0]^tmp[1]^tmp[2];
			CheckBits[353] = ecc_bit[1]^ecc_bit[2]^ecc_bit[3]^ecc_bit[357]^tmp[1]^tmp[2]^tmp[3];
			CheckBits[354] = ecc_bit[1]^ecc_bit[2]^ecc_bit[3]^ecc_bit[358]^tmp[1]^tmp[2]^tmp[3];
			CheckBits[355] = ecc_bit[1]^ecc_bit[2]^ecc_bit[3]^ecc_bit[359]^tmp[1]^tmp[2]^tmp[3];
			CheckBits[356] = ecc_bit[1]^ecc_bit[2]^ecc_bit[3]^ecc_bit[360]^tmp[1]^tmp[2]^tmp[3];
			CheckBits[357] = ecc_bit[0]^ecc_bit[1]^ecc_bit[2]^ecc_bit[3]^ecc_bit[361]^tmp[0]^tmp[1]^tmp[2]^tmp[3];
			CheckBits[358] = ecc_bit[0]^ecc_bit[2]^ecc_bit[3]^ecc_bit[362]^tmp[0]^tmp[2]^tmp[3];
			CheckBits[359] = ecc_bit[0]^ecc_bit[3]^ecc_bit[363]^tmp[0]^tmp[3];
			CheckBits[360] = ecc_bit[0]^ecc_bit[364]^tmp[0];
			CheckBits[361] = ecc_bit[1]^ecc_bit[365]^tmp[1];
			CheckBits[362] = ecc_bit[2]^ecc_bit[366]^tmp[2];
			CheckBits[363] = ecc_bit[3]^ecc_bit[367]^tmp[3];
			CheckBits[364] = ecc_bit[1]^ecc_bit[368]^tmp[1];
			CheckBits[365] = ecc_bit[0]^ecc_bit[2]^ecc_bit[369]^tmp[0]^tmp[2];
			CheckBits[366] = ecc_bit[1]^ecc_bit[3]^ecc_bit[370]^tmp[1]^tmp[3];
			CheckBits[367] = ecc_bit[1]^ecc_bit[2]^ecc_bit[371]^tmp[1]^tmp[2];
			CheckBits[368] = ecc_bit[2]^ecc_bit[3]^ecc_bit[372]^tmp[2]^tmp[3];
			CheckBits[369] = ecc_bit[1]^ecc_bit[3]^ecc_bit[373]^tmp[1]^tmp[3];
			CheckBits[370] = ecc_bit[1]^ecc_bit[2]^ecc_bit[374]^tmp[1]^tmp[2];
			CheckBits[371] = ecc_bit[0]^ecc_bit[2]^ecc_bit[3]^ecc_bit[375]^tmp[0]^tmp[2]^tmp[3];
			CheckBits[372] = ecc_bit[0]^ecc_bit[3]^ecc_bit[376]^tmp[0]^tmp[3];
			CheckBits[373] = ecc_bit[0]^ecc_bit[377]^tmp[0];
			CheckBits[374] = ecc_bit[1]^ecc_bit[378]^tmp[1];
			CheckBits[375] = ecc_bit[2]^ecc_bit[379]^tmp[2];
			CheckBits[376] = ecc_bit[3]^ecc_bit[380]^tmp[3];
			CheckBits[377] = ecc_bit[0]^ecc_bit[1]^ecc_bit[381]^tmp[0]^tmp[1];
			CheckBits[378] = ecc_bit[0]^ecc_bit[1]^ecc_bit[2]^ecc_bit[382]^tmp[0]^tmp[1]^tmp[2];
			CheckBits[379] = ecc_bit[0]^ecc_bit[1]^ecc_bit[2]^ecc_bit[3]^ecc_bit[383]^tmp[0]^tmp[1]^tmp[2]^tmp[3];
			CheckBits[380] = ecc_bit[2]^ecc_bit[3]^ecc_bit[384]^tmp[2]^tmp[3];
			CheckBits[381] = ecc_bit[0]^ecc_bit[1]^ecc_bit[3]^ecc_bit[385]^tmp[0]^tmp[1]^tmp[3];
			CheckBits[382] = ecc_bit[0]^ecc_bit[2]^ecc_bit[386]^tmp[0]^tmp[2];
			CheckBits[383] = ecc_bit[1]^ecc_bit[3]^ecc_bit[387]^tmp[1]^tmp[3];
			CheckBits[384] = ecc_bit[0]^ecc_bit[1]^ecc_bit[2]^ecc_bit[388]^tmp[0]^tmp[1]^tmp[2];
			CheckBits[385] = ecc_bit[1]^ecc_bit[2]^ecc_bit[3]^ecc_bit[389]^tmp[1]^tmp[2]^tmp[3];
			CheckBits[386] = ecc_bit[1]^ecc_bit[2]^ecc_bit[3]^ecc_bit[390]^tmp[1]^tmp[2]^tmp[3];
			CheckBits[387] = ecc_bit[1]^ecc_bit[2]^ecc_bit[3]^ecc_bit[391]^tmp[1]^tmp[2]^tmp[3];
			CheckBits[388] = ecc_bit[0]^ecc_bit[1]^ecc_bit[2]^ecc_bit[3]^ecc_bit[392]^tmp[0]^tmp[1]^tmp[2]^tmp[3];
			CheckBits[389] = ecc_bit[0]^ecc_bit[2]^ecc_bit[3]^ecc_bit[393]^tmp[0]^tmp[2]^tmp[3];
			CheckBits[390] = ecc_bit[0]^ecc_bit[3]^ecc_bit[394]^tmp[0]^tmp[3];
			CheckBits[391] = ecc_bit[395];
			CheckBits[392] = ecc_bit[0]^ecc_bit[396]^tmp[0];
			CheckBits[393] = ecc_bit[0]^ecc_bit[1]^ecc_bit[397]^tmp[0]^tmp[1];
			CheckBits[394] = ecc_bit[0]^ecc_bit[1]^ecc_bit[2]^ecc_bit[398]^tmp[0]^tmp[1]^tmp[2];
			CheckBits[395] = ecc_bit[1]^ecc_bit[2]^ecc_bit[3]^ecc_bit[399]^tmp[1]^tmp[2]^tmp[3];
			CheckBits[396] = ecc_bit[1]^ecc_bit[2]^ecc_bit[3]^ecc_bit[400]^tmp[1]^tmp[2]^tmp[3];
			CheckBits[397] = ecc_bit[1]^ecc_bit[2]^ecc_bit[3]^ecc_bit[401]^tmp[1]^tmp[2]^tmp[3];
			CheckBits[398] = ecc_bit[0]^ecc_bit[1]^ecc_bit[2]^ecc_bit[3]^ecc_bit[402]^tmp[0]^tmp[1]^tmp[2]^tmp[3];
			CheckBits[399] = ecc_bit[0]^ecc_bit[2]^ecc_bit[3]^ecc_bit[403]^tmp[0]^tmp[2]^tmp[3];
			CheckBits[400] = ecc_bit[0]^ecc_bit[3]^ecc_bit[404]^tmp[0]^tmp[3];
			CheckBits[401] = ecc_bit[0]^ecc_bit[405]^tmp[0];
			CheckBits[402] = ecc_bit[1]^ecc_bit[406]^tmp[1];
			CheckBits[403] = ecc_bit[0]^ecc_bit[2]^ecc_bit[407]^tmp[0]^tmp[2];
			CheckBits[404] = ecc_bit[1]^ecc_bit[3]^ecc_bit[408]^tmp[1]^tmp[3];
			CheckBits[405] = ecc_bit[0]^ecc_bit[1]^ecc_bit[2]^ecc_bit[409]^tmp[0]^tmp[1]^tmp[2];
			CheckBits[406] = ecc_bit[0]^ecc_bit[1]^ecc_bit[2]^ecc_bit[3]^ecc_bit[410]^tmp[0]^tmp[1]^tmp[2]^tmp[3];
			CheckBits[407] = ecc_bit[2]^ecc_bit[3]^ecc_bit[411]^tmp[2]^tmp[3];
			CheckBits[408] = ecc_bit[0]^ecc_bit[1]^ecc_bit[3]^ecc_bit[412]^tmp[0]^tmp[1]^tmp[3];
			CheckBits[409] = ecc_bit[0]^ecc_bit[2]^ecc_bit[413]^tmp[0]^tmp[2];
			CheckBits[410] = ecc_bit[0]^ecc_bit[1]^ecc_bit[3]^ecc_bit[414]^tmp[0]^tmp[1]^tmp[3];
			CheckBits[411] = ecc_bit[0]^ecc_bit[2]^ecc_bit[415]^tmp[0]^tmp[2];
			CheckBits[412] = ecc_bit[0]^ecc_bit[1]^ecc_bit[3]^ecc_bit[416]^tmp[0]^tmp[1]^tmp[3];
			CheckBits[413] = ecc_bit[2]^ecc_bit[417]^tmp[2];
			CheckBits[414] = ecc_bit[0]^ecc_bit[3]^ecc_bit[418]^tmp[0]^tmp[3];
			CheckBits[415] = ecc_bit[419];
			CheckBits[416] = ecc_bit[0]^ecc_bit[420]^tmp[0];
			CheckBits[417] = ecc_bit[1]^ecc_bit[421]^tmp[1];
			CheckBits[418] = ecc_bit[2]^ecc_bit[422]^tmp[2];
			CheckBits[419] = ecc_bit[3]^ecc_bit[423]^tmp[3];
			CheckBits[420] = ecc_bit[1]^ecc_bit[424]^tmp[1];
			CheckBits[421] = ecc_bit[0]^ecc_bit[2]^ecc_bit[425]^tmp[0]^tmp[2];
			CheckBits[422] = ecc_bit[0]^ecc_bit[1]^ecc_bit[3]^ecc_bit[426]^tmp[0]^tmp[1]^tmp[3];
			CheckBits[423] = ecc_bit[2]^ecc_bit[427]^tmp[2];
			CheckBits[424] = ecc_bit[3]^ecc_bit[428]^tmp[3];
			CheckBits[425] = ecc_bit[0]^ecc_bit[1]^ecc_bit[429]^tmp[0]^tmp[1];
			CheckBits[426] = ecc_bit[0]^ecc_bit[1]^ecc_bit[2]^ecc_bit[430]^tmp[0]^tmp[1]^tmp[2];
			CheckBits[427] = ecc_bit[1]^ecc_bit[2]^ecc_bit[3]^ecc_bit[431]^tmp[1]^tmp[2]^tmp[3];
			CheckBits[428] = ecc_bit[1]^ecc_bit[2]^ecc_bit[3]^ecc_bit[432]^tmp[1]^tmp[2]^tmp[3];
			CheckBits[429] = ecc_bit[0]^ecc_bit[1]^ecc_bit[2]^ecc_bit[3]^ecc_bit[433]^tmp[0]^tmp[1]^tmp[2]^tmp[3];
			CheckBits[430] = ecc_bit[0]^ecc_bit[2]^ecc_bit[3]^ecc_bit[434]^tmp[0]^tmp[2]^tmp[3];
			CheckBits[431] = ecc_bit[0]^ecc_bit[3]^ecc_bit[435]^tmp[0]^tmp[3];
			CheckBits[432] = ecc_bit[0]^ecc_bit[436]^tmp[0];
			CheckBits[433] = ecc_bit[0]^ecc_bit[1]^ecc_bit[437]^tmp[0]^tmp[1];
			CheckBits[434] = ecc_bit[0]^ecc_bit[1]^ecc_bit[2]^ecc_bit[438]^tmp[0]^tmp[1]^tmp[2];
			CheckBits[435] = ecc_bit[1]^ecc_bit[2]^ecc_bit[3]^ecc_bit[439]^tmp[1]^tmp[2]^tmp[3];
			CheckBits[436] = ecc_bit[1]^ecc_bit[2]^ecc_bit[3]^ecc_bit[440]^tmp[1]^tmp[2]^tmp[3];
			CheckBits[437] = ecc_bit[1]^ecc_bit[2]^ecc_bit[3]^ecc_bit[441]^tmp[1]^tmp[2]^tmp[3];
			CheckBits[438] = ecc_bit[0]^ecc_bit[1]^ecc_bit[2]^ecc_bit[3]^ecc_bit[442]^tmp[0]^tmp[1]^tmp[2]^tmp[3];
			CheckBits[439] = ecc_bit[0]^ecc_bit[2]^ecc_bit[3]^ecc_bit[443]^tmp[0]^tmp[2]^tmp[3];
			CheckBits[440] = ecc_bit[3]^ecc_bit[444]^tmp[3];
			CheckBits[441] = ecc_bit[1]^ecc_bit[445]^tmp[1];
			CheckBits[442] = ecc_bit[2]^ecc_bit[446]^tmp[2];
			CheckBits[443] = ecc_bit[0]^ecc_bit[3]^ecc_bit[447]^tmp[0]^tmp[3];
			CheckBits[444] = ecc_bit[0]^tmp[0];
			CheckBits[445] = ecc_bit[1]^tmp[1];
			CheckBits[446] = ecc_bit[2]^tmp[2];
			CheckBits[447] = ecc_bit[0]^ecc_bit[3]^tmp[0]^tmp[3];
		} else if(ecc_size == 53) {
			CheckBits[0] = ecc_bit[3]^ecc_bit[4]^tmp[3];
			CheckBits[1] = ecc_bit[0]^ecc_bit[3]^ecc_bit[5]^tmp[0]^tmp[3];
			CheckBits[2] = ecc_bit[1]^ecc_bit[3]^ecc_bit[6]^tmp[1]^tmp[3];
			CheckBits[3] = ecc_bit[2]^ecc_bit[3]^ecc_bit[7]^tmp[2]^tmp[3];
			CheckBits[4] = ecc_bit[0]^ecc_bit[8]^tmp[0];
			CheckBits[5] = ecc_bit[1]^ecc_bit[9]^tmp[1];
			CheckBits[6] = ecc_bit[2]^ecc_bit[10]^tmp[2];
			CheckBits[7] = ecc_bit[0]^ecc_bit[3]^ecc_bit[11]^tmp[0]^tmp[3];
			CheckBits[8] = ecc_bit[0]^ecc_bit[1]^ecc_bit[3]^ecc_bit[12]^tmp[0]^tmp[1]^tmp[3];
			CheckBits[9] = ecc_bit[1]^ecc_bit[2]^ecc_bit[3]^ecc_bit[13]^tmp[1]^tmp[2]^tmp[3];
			CheckBits[10] = ecc_bit[2]^ecc_bit[14]^tmp[2];
			CheckBits[11] = ecc_bit[3]^ecc_bit[15]^tmp[3];
			CheckBits[12] = ecc_bit[3]^ecc_bit[16]^tmp[3];
			CheckBits[13] = ecc_bit[0]^ecc_bit[3]^ecc_bit[17]^tmp[0]^tmp[3];
			CheckBits[14] = ecc_bit[0]^ecc_bit[1]^ecc_bit[3]^ecc_bit[18]^tmp[0]^tmp[1]^tmp[3];
			CheckBits[15] = ecc_bit[1]^ecc_bit[2]^ecc_bit[3]^ecc_bit[19]^tmp[1]^tmp[2]^tmp[3];
			CheckBits[16] = ecc_bit[0]^ecc_bit[2]^ecc_bit[20]^tmp[0]^tmp[2];
			CheckBits[17] = ecc_bit[0]^ecc_bit[1]^ecc_bit[3]^ecc_bit[21]^tmp[0]^tmp[1]^tmp[3];
			CheckBits[18] = ecc_bit[0]^ecc_bit[1]^ecc_bit[2]^ecc_bit[3]^ecc_bit[22]^tmp[0]^tmp[1]^tmp[2]^tmp[3];
			CheckBits[19] = ecc_bit[1]^ecc_bit[2]^ecc_bit[23]^tmp[1]^tmp[2];
			CheckBits[20] = ecc_bit[0]^ecc_bit[2]^ecc_bit[3]^ecc_bit[24]^tmp[0]^tmp[2]^tmp[3];
			CheckBits[21] = ecc_bit[0]^ecc_bit[1]^ecc_bit[25]^tmp[0]^tmp[1];
			CheckBits[22] = ecc_bit[0]^ecc_bit[1]^ecc_bit[2]^ecc_bit[26]^tmp[0]^tmp[1]^tmp[2];
			CheckBits[23] = ecc_bit[0]^ecc_bit[1]^ecc_bit[2]^ecc_bit[3]^ecc_bit[27]^tmp[0]^tmp[1]^tmp[2]^tmp[3];
			CheckBits[24] = ecc_bit[0]^ecc_bit[1]^ecc_bit[2]^ecc_bit[28]^tmp[0]^tmp[1]^tmp[2];
			CheckBits[25] = ecc_bit[1]^ecc_bit[2]^ecc_bit[3]^ecc_bit[29]^tmp[1]^tmp[2]^tmp[3];
			CheckBits[26] = ecc_bit[0]^ecc_bit[2]^ecc_bit[30]^tmp[0]^tmp[2];
			CheckBits[27] = ecc_bit[1]^ecc_bit[3]^ecc_bit[31]^tmp[1]^tmp[3];
			CheckBits[28] = ecc_bit[0]^ecc_bit[2]^ecc_bit[3]^ecc_bit[32]^tmp[0]^tmp[2]^tmp[3];
			CheckBits[29] = ecc_bit[0]^ecc_bit[1]^ecc_bit[33]^tmp[0]^tmp[1];
			CheckBits[30] = ecc_bit[0]^ecc_bit[1]^ecc_bit[2]^ecc_bit[34]^tmp[0]^tmp[1]^tmp[2];
			CheckBits[31] = ecc_bit[0]^ecc_bit[1]^ecc_bit[2]^ecc_bit[3]^ecc_bit[35]^tmp[0]^tmp[1]^tmp[2]^tmp[3];
			CheckBits[32] = ecc_bit[0]^ecc_bit[1]^ecc_bit[2]^ecc_bit[36]^tmp[0]^tmp[1]^tmp[2];
			CheckBits[33] = ecc_bit[1]^ecc_bit[2]^ecc_bit[3]^ecc_bit[37]^tmp[1]^tmp[2]^tmp[3];
			CheckBits[34] = ecc_bit[2]^ecc_bit[38]^tmp[2];
			CheckBits[35] = ecc_bit[0]^ecc_bit[3]^ecc_bit[39]^tmp[0]^tmp[3];
			CheckBits[36] = ecc_bit[1]^ecc_bit[3]^ecc_bit[40]^tmp[1]^tmp[3];
			CheckBits[37] = ecc_bit[2]^ecc_bit[3]^ecc_bit[41]^tmp[2]^tmp[3];
			CheckBits[38] = ecc_bit[42];
			CheckBits[39] = ecc_bit[43];
			CheckBits[40] = ecc_bit[44];
			CheckBits[41] = ecc_bit[0]^ecc_bit[45]^tmp[0];
			CheckBits[42] = ecc_bit[1]^ecc_bit[46]^tmp[1];
			CheckBits[43] = ecc_bit[2]^ecc_bit[47]^tmp[2];
			CheckBits[44] = ecc_bit[0]^ecc_bit[3]^ecc_bit[48]^tmp[0]^tmp[3];
			CheckBits[45] = ecc_bit[1]^ecc_bit[3]^ecc_bit[49]^tmp[1]^tmp[3];
			CheckBits[46] = ecc_bit[2]^ecc_bit[3]^ecc_bit[50]^tmp[2]^tmp[3];
			CheckBits[47] = ecc_bit[51];
			CheckBits[48] = ecc_bit[0]^ecc_bit[52]^tmp[0];
			CheckBits[49] = ecc_bit[1]^ecc_bit[53]^tmp[1];
			CheckBits[50] = ecc_bit[0]^ecc_bit[2]^ecc_bit[54]^tmp[0]^tmp[2];
			CheckBits[51] = ecc_bit[0]^ecc_bit[1]^ecc_bit[3]^ecc_bit[55]^tmp[0]^tmp[1]^tmp[3];
			CheckBits[52] = ecc_bit[1]^ecc_bit[2]^ecc_bit[3]^ecc_bit[56]^tmp[1]^tmp[2]^tmp[3];
			CheckBits[53] = ecc_bit[2]^ecc_bit[57]^tmp[2];
			CheckBits[54] = ecc_bit[3]^ecc_bit[58]^tmp[3];
			CheckBits[55] = ecc_bit[0]^ecc_bit[3]^ecc_bit[59]^tmp[0]^tmp[3];
			CheckBits[56] = ecc_bit[0]^ecc_bit[1]^ecc_bit[3]^ecc_bit[60]^tmp[0]^tmp[1]^tmp[3];
			CheckBits[57] = ecc_bit[1]^ecc_bit[2]^ecc_bit[3]^ecc_bit[61]^tmp[1]^tmp[2]^tmp[3];
			CheckBits[58] = ecc_bit[2]^ecc_bit[62]^tmp[2];
			CheckBits[59] = ecc_bit[3]^ecc_bit[63]^tmp[3];
			CheckBits[60] = ecc_bit[3]^ecc_bit[64]^tmp[3];
			CheckBits[61] = ecc_bit[3]^ecc_bit[65]^tmp[3];
			CheckBits[62] = ecc_bit[3]^ecc_bit[66]^tmp[3];
			CheckBits[63] = ecc_bit[3]^ecc_bit[67]^tmp[3];
			CheckBits[64] = ecc_bit[0]^ecc_bit[3]^ecc_bit[68]^tmp[0]^tmp[3];
			CheckBits[65] = ecc_bit[1]^ecc_bit[3]^ecc_bit[69]^tmp[1]^tmp[3];
			CheckBits[66] = ecc_bit[2]^ecc_bit[3]^ecc_bit[70]^tmp[2]^tmp[3];
			CheckBits[67] = ecc_bit[0]^ecc_bit[71]^tmp[0];
			CheckBits[68] = ecc_bit[0]^ecc_bit[1]^ecc_bit[72]^tmp[0]^tmp[1];
			CheckBits[69] = ecc_bit[0]^ecc_bit[1]^ecc_bit[2]^ecc_bit[73]^tmp[0]^tmp[1]^tmp[2];
			CheckBits[70] = ecc_bit[0]^ecc_bit[1]^ecc_bit[2]^ecc_bit[3]^ecc_bit[74]^tmp[0]^tmp[1]^tmp[2]^tmp[3];
			CheckBits[71] = ecc_bit[1]^ecc_bit[2]^ecc_bit[75]^tmp[1]^tmp[2];
			CheckBits[72] = ecc_bit[0]^ecc_bit[2]^ecc_bit[3]^ecc_bit[76]^tmp[0]^tmp[2]^tmp[3];
			CheckBits[73] = ecc_bit[0]^ecc_bit[1]^ecc_bit[77]^tmp[0]^tmp[1];
			CheckBits[74] = ecc_bit[1]^ecc_bit[2]^ecc_bit[78]^tmp[1]^tmp[2];
			CheckBits[75] = ecc_bit[2]^ecc_bit[3]^ecc_bit[79]^tmp[2]^tmp[3];
			CheckBits[76] = ecc_bit[80];
			CheckBits[77] = ecc_bit[0]^ecc_bit[81]^tmp[0];
			CheckBits[78] = ecc_bit[0]^ecc_bit[1]^ecc_bit[82]^tmp[0]^tmp[1];
			CheckBits[79] = ecc_bit[1]^ecc_bit[2]^ecc_bit[83]^tmp[1]^tmp[2];
			CheckBits[80] = ecc_bit[2]^ecc_bit[3]^ecc_bit[84]^tmp[2]^tmp[3];
			CheckBits[81] = ecc_bit[85];
			CheckBits[82] = ecc_bit[0]^ecc_bit[86]^tmp[0];
			CheckBits[83] = ecc_bit[0]^ecc_bit[1]^ecc_bit[87]^tmp[0]^tmp[1];
			CheckBits[84] = ecc_bit[1]^ecc_bit[2]^ecc_bit[88]^tmp[1]^tmp[2];
			CheckBits[85] = ecc_bit[0]^ecc_bit[2]^ecc_bit[3]^ecc_bit[89]^tmp[0]^tmp[2]^tmp[3];
			CheckBits[86] = ecc_bit[0]^ecc_bit[1]^ecc_bit[90]^tmp[0]^tmp[1];
			CheckBits[87] = ecc_bit[1]^ecc_bit[2]^ecc_bit[91]^tmp[1]^tmp[2];
			CheckBits[88] = ecc_bit[2]^ecc_bit[3]^ecc_bit[92]^tmp[2]^tmp[3];
			CheckBits[89] = ecc_bit[93];
			CheckBits[90] = ecc_bit[94];
			CheckBits[91] = ecc_bit[0]^ecc_bit[95]^tmp[0];
			CheckBits[92] = ecc_bit[1]^ecc_bit[96]^tmp[1];
			CheckBits[93] = ecc_bit[0]^ecc_bit[2]^ecc_bit[97]^tmp[0]^tmp[2];
			CheckBits[94] = ecc_bit[1]^ecc_bit[3]^ecc_bit[98]^tmp[1]^tmp[3];
			CheckBits[95] = ecc_bit[2]^ecc_bit[3]^ecc_bit[99]^tmp[2]^tmp[3];
			CheckBits[96] = ecc_bit[100];
			CheckBits[97] = ecc_bit[0]^ecc_bit[101]^tmp[0];
			CheckBits[98] = ecc_bit[0]^ecc_bit[1]^ecc_bit[102]^tmp[0]^tmp[1];
			CheckBits[99] = ecc_bit[1]^ecc_bit[2]^ecc_bit[103]^tmp[1]^tmp[2];
			CheckBits[100] = ecc_bit[2]^ecc_bit[3]^ecc_bit[104]^tmp[2]^tmp[3];
			CheckBits[101] = ecc_bit[105];
			CheckBits[102] = ecc_bit[0]^ecc_bit[106]^tmp[0];
			CheckBits[103] = ecc_bit[0]^ecc_bit[1]^ecc_bit[107]^tmp[0]^tmp[1];
			CheckBits[104] = ecc_bit[1]^ecc_bit[2]^ecc_bit[108]^tmp[1]^tmp[2];
			CheckBits[105] = ecc_bit[2]^ecc_bit[3]^ecc_bit[109]^tmp[2]^tmp[3];
			CheckBits[106] = ecc_bit[0]^ecc_bit[110]^tmp[0];
			CheckBits[107] = ecc_bit[0]^ecc_bit[1]^ecc_bit[111]^tmp[0]^tmp[1];
			CheckBits[108] = ecc_bit[0]^ecc_bit[1]^ecc_bit[2]^ecc_bit[112]^tmp[0]^tmp[1]^tmp[2];
			CheckBits[109] = ecc_bit[0]^ecc_bit[1]^ecc_bit[2]^ecc_bit[3]^ecc_bit[113]^tmp[0]^tmp[1]^tmp[2]^tmp[3];
			CheckBits[110] = ecc_bit[0]^ecc_bit[1]^ecc_bit[2]^ecc_bit[114]^tmp[0]^tmp[1]^tmp[2];
			CheckBits[111] = ecc_bit[1]^ecc_bit[2]^ecc_bit[3]^ecc_bit[115]^tmp[1]^tmp[2]^tmp[3];
			CheckBits[112] = ecc_bit[0]^ecc_bit[2]^ecc_bit[116]^tmp[0]^tmp[2];
			CheckBits[113] = ecc_bit[1]^ecc_bit[3]^ecc_bit[117]^tmp[1]^tmp[3];
			CheckBits[114] = ecc_bit[0]^ecc_bit[2]^ecc_bit[3]^ecc_bit[118]^tmp[0]^tmp[2]^tmp[3];
			CheckBits[115] = ecc_bit[0]^ecc_bit[1]^ecc_bit[119]^tmp[0]^tmp[1];
			CheckBits[116] = ecc_bit[1]^ecc_bit[2]^ecc_bit[120]^tmp[1]^tmp[2];
			CheckBits[117] = ecc_bit[0]^ecc_bit[2]^ecc_bit[3]^ecc_bit[121]^tmp[0]^tmp[2]^tmp[3];
			CheckBits[118] = ecc_bit[0]^ecc_bit[1]^ecc_bit[122]^tmp[0]^tmp[1];
			CheckBits[119] = ecc_bit[1]^ecc_bit[2]^ecc_bit[123]^tmp[1]^tmp[2];
			CheckBits[120] = ecc_bit[2]^ecc_bit[3]^ecc_bit[124]^tmp[2]^tmp[3];
			CheckBits[121] = ecc_bit[125];
			CheckBits[122] = ecc_bit[126];
			CheckBits[123] = ecc_bit[127];
			CheckBits[124] = ecc_bit[128];
			CheckBits[125] = ecc_bit[129];
			CheckBits[126] = ecc_bit[0]^ecc_bit[130]^tmp[0];
			CheckBits[127] = ecc_bit[0]^ecc_bit[1]^ecc_bit[131]^tmp[0]^tmp[1];
			CheckBits[128] = ecc_bit[1]^ecc_bit[2]^ecc_bit[132]^tmp[1]^tmp[2];
			CheckBits[129] = ecc_bit[0]^ecc_bit[2]^ecc_bit[3]^ecc_bit[133]^tmp[0]^tmp[2]^tmp[3];
			CheckBits[130] = ecc_bit[0]^ecc_bit[1]^ecc_bit[134]^tmp[0]^tmp[1];
			CheckBits[131] = ecc_bit[0]^ecc_bit[1]^ecc_bit[2]^ecc_bit[135]^tmp[0]^tmp[1]^tmp[2];
			CheckBits[132] = ecc_bit[0]^ecc_bit[1]^ecc_bit[2]^ecc_bit[3]^ecc_bit[136]^tmp[0]^tmp[1]^tmp[2]^tmp[3];
			CheckBits[133] = ecc_bit[1]^ecc_bit[2]^ecc_bit[137]^tmp[1]^tmp[2];
			CheckBits[134] = ecc_bit[0]^ecc_bit[2]^ecc_bit[3]^ecc_bit[138]^tmp[0]^tmp[2]^tmp[3];
			CheckBits[135] = ecc_bit[0]^ecc_bit[1]^ecc_bit[139]^tmp[0]^tmp[1];
			CheckBits[136] = ecc_bit[1]^ecc_bit[2]^ecc_bit[140]^tmp[1]^tmp[2];
			CheckBits[137] = ecc_bit[0]^ecc_bit[2]^ecc_bit[3]^ecc_bit[141]^tmp[0]^tmp[2]^tmp[3];
			CheckBits[138] = ecc_bit[0]^ecc_bit[1]^ecc_bit[142]^tmp[0]^tmp[1];
			CheckBits[139] = ecc_bit[1]^ecc_bit[2]^ecc_bit[143]^tmp[1]^tmp[2];
			CheckBits[140] = ecc_bit[0]^ecc_bit[2]^ecc_bit[3]^ecc_bit[144]^tmp[0]^tmp[2]^tmp[3];
			CheckBits[141] = ecc_bit[0]^ecc_bit[1]^ecc_bit[145]^tmp[0]^tmp[1];
			CheckBits[142] = ecc_bit[1]^ecc_bit[2]^ecc_bit[146]^tmp[1]^tmp[2];
			CheckBits[143] = ecc_bit[0]^ecc_bit[2]^ecc_bit[3]^ecc_bit[147]^tmp[0]^tmp[2]^tmp[3];
			CheckBits[144] = ecc_bit[1]^ecc_bit[148]^tmp[1];
			CheckBits[145] = ecc_bit[0]^ecc_bit[2]^ecc_bit[149]^tmp[0]^tmp[2];
			CheckBits[146] = ecc_bit[0]^ecc_bit[1]^ecc_bit[3]^ecc_bit[150]^tmp[0]^tmp[1]^tmp[3];
			CheckBits[147] = ecc_bit[1]^ecc_bit[2]^ecc_bit[3]^ecc_bit[151]^tmp[1]^tmp[2]^tmp[3];
			CheckBits[148] = ecc_bit[2]^ecc_bit[152]^tmp[2];
			CheckBits[149] = ecc_bit[3]^ecc_bit[153]^tmp[3];
			CheckBits[150] = ecc_bit[0]^ecc_bit[3]^ecc_bit[154]^tmp[0]^tmp[3];
			CheckBits[151] = ecc_bit[1]^ecc_bit[3]^ecc_bit[155]^tmp[1]^tmp[3];
			CheckBits[152] = ecc_bit[2]^ecc_bit[3]^ecc_bit[156]^tmp[2]^tmp[3];
			CheckBits[153] = ecc_bit[0]^ecc_bit[157]^tmp[0];
			CheckBits[154] = ecc_bit[1]^ecc_bit[158]^tmp[1];
			CheckBits[155] = ecc_bit[2]^ecc_bit[159]^tmp[2];
			CheckBits[156] = ecc_bit[0]^ecc_bit[3]^ecc_bit[160]^tmp[0]^tmp[3];
			CheckBits[157] = ecc_bit[1]^ecc_bit[3]^ecc_bit[161]^tmp[1]^tmp[3];
			CheckBits[158] = ecc_bit[2]^ecc_bit[3]^ecc_bit[162]^tmp[2]^tmp[3];
			CheckBits[159] = ecc_bit[163];
			CheckBits[160] = ecc_bit[164];
			CheckBits[161] = ecc_bit[0]^ecc_bit[165]^tmp[0];
			CheckBits[162] = ecc_bit[0]^ecc_bit[1]^ecc_bit[166]^tmp[0]^tmp[1];
			CheckBits[163] = ecc_bit[0]^ecc_bit[1]^ecc_bit[2]^ecc_bit[167]^tmp[0]^tmp[1]^tmp[2];
			CheckBits[164] = ecc_bit[0]^ecc_bit[1]^ecc_bit[2]^ecc_bit[3]^ecc_bit[168]^tmp[0]^tmp[1]^tmp[2]^tmp[3];
			CheckBits[165] = ecc_bit[1]^ecc_bit[2]^ecc_bit[169]^tmp[1]^tmp[2];
			CheckBits[166] = ecc_bit[0]^ecc_bit[2]^ecc_bit[3]^ecc_bit[170]^tmp[0]^tmp[2]^tmp[3];
			CheckBits[167] = ecc_bit[0]^ecc_bit[1]^ecc_bit[171]^tmp[0]^tmp[1];
			CheckBits[168] = ecc_bit[0]^ecc_bit[1]^ecc_bit[2]^ecc_bit[172]^tmp[0]^tmp[1]^tmp[2];
			CheckBits[169] = ecc_bit[0]^ecc_bit[1]^ecc_bit[2]^ecc_bit[3]^ecc_bit[173]^tmp[0]^tmp[1]^tmp[2]^tmp[3];
			CheckBits[170] = ecc_bit[1]^ecc_bit[2]^ecc_bit[174]^tmp[1]^tmp[2];
			CheckBits[171] = ecc_bit[2]^ecc_bit[3]^ecc_bit[175]^tmp[2]^tmp[3];
			CheckBits[172] = ecc_bit[176];
			CheckBits[173] = ecc_bit[0]^ecc_bit[177]^tmp[0];
			CheckBits[174] = ecc_bit[0]^ecc_bit[1]^ecc_bit[178]^tmp[0]^tmp[1];
			CheckBits[175] = ecc_bit[0]^ecc_bit[1]^ecc_bit[2]^ecc_bit[179]^tmp[0]^tmp[1]^tmp[2];
			CheckBits[176] = ecc_bit[0]^ecc_bit[1]^ecc_bit[2]^ecc_bit[3]^ecc_bit[180]^tmp[0]^tmp[1]^tmp[2]^tmp[3];
			CheckBits[177] = ecc_bit[0]^ecc_bit[1]^ecc_bit[2]^ecc_bit[181]^tmp[0]^tmp[1]^tmp[2];
			CheckBits[178] = ecc_bit[1]^ecc_bit[2]^ecc_bit[3]^ecc_bit[182]^tmp[1]^tmp[2]^tmp[3];
			CheckBits[179] = ecc_bit[2]^ecc_bit[183]^tmp[2];
			CheckBits[180] = ecc_bit[3]^ecc_bit[184]^tmp[3];
			CheckBits[181] = ecc_bit[0]^ecc_bit[3]^ecc_bit[185]^tmp[0]^tmp[3];
			CheckBits[182] = ecc_bit[1]^ecc_bit[3]^ecc_bit[186]^tmp[1]^tmp[3];
			CheckBits[183] = ecc_bit[0]^ecc_bit[2]^ecc_bit[3]^ecc_bit[187]^tmp[0]^tmp[2]^tmp[3];
			CheckBits[184] = ecc_bit[1]^ecc_bit[188]^tmp[1];
			CheckBits[185] = ecc_bit[0]^ecc_bit[2]^ecc_bit[189]^tmp[0]^tmp[2];
			CheckBits[186] = ecc_bit[0]^ecc_bit[1]^ecc_bit[3]^ecc_bit[190]^tmp[0]^tmp[1]^tmp[3];
			CheckBits[187] = ecc_bit[1]^ecc_bit[2]^ecc_bit[3]^ecc_bit[191]^tmp[1]^tmp[2]^tmp[3];
			CheckBits[188] = ecc_bit[0]^ecc_bit[2]^ecc_bit[192]^tmp[0]^tmp[2];
			CheckBits[189] = ecc_bit[0]^ecc_bit[1]^ecc_bit[3]^ecc_bit[193]^tmp[0]^tmp[1]^tmp[3];
			CheckBits[190] = ecc_bit[0]^ecc_bit[1]^ecc_bit[2]^ecc_bit[3]^ecc_bit[194]^tmp[0]^tmp[1]^tmp[2]^tmp[3];
			CheckBits[191] = ecc_bit[1]^ecc_bit[2]^ecc_bit[195]^tmp[1]^tmp[2];
			CheckBits[192] = ecc_bit[0]^ecc_bit[2]^ecc_bit[3]^ecc_bit[196]^tmp[0]^tmp[2]^tmp[3];
			CheckBits[193] = ecc_bit[1]^ecc_bit[197]^tmp[1];
			CheckBits[194] = ecc_bit[2]^ecc_bit[198]^tmp[2];
			CheckBits[195] = ecc_bit[3]^ecc_bit[199]^tmp[3];
			CheckBits[196] = ecc_bit[3]^ecc_bit[200]^tmp[3];
			CheckBits[197] = ecc_bit[0]^ecc_bit[3]^ecc_bit[201]^tmp[0]^tmp[3];
			CheckBits[198] = ecc_bit[1]^ecc_bit[3]^ecc_bit[202]^tmp[1]^tmp[3];
			CheckBits[199] = ecc_bit[0]^ecc_bit[2]^ecc_bit[3]^ecc_bit[203]^tmp[0]^tmp[2]^tmp[3];
			CheckBits[200] = ecc_bit[0]^ecc_bit[1]^ecc_bit[204]^tmp[0]^tmp[1];
			CheckBits[201] = ecc_bit[1]^ecc_bit[2]^ecc_bit[205]^tmp[1]^tmp[2];
			CheckBits[202] = ecc_bit[0]^ecc_bit[2]^ecc_bit[3]^ecc_bit[206]^tmp[0]^tmp[2]^tmp[3];
			CheckBits[203] = ecc_bit[0]^ecc_bit[1]^ecc_bit[207]^tmp[0]^tmp[1];
			CheckBits[204] = ecc_bit[1]^ecc_bit[2]^ecc_bit[208]^tmp[1]^tmp[2];
			CheckBits[205] = ecc_bit[0]^ecc_bit[2]^ecc_bit[3]^ecc_bit[209]^tmp[0]^tmp[2]^tmp[3];
			CheckBits[206] = ecc_bit[1]^ecc_bit[210]^tmp[1];
			CheckBits[207] = ecc_bit[2]^ecc_bit[211]^tmp[2];
			CheckBits[208] = ecc_bit[3]^ecc_bit[212]^tmp[3];
			CheckBits[209] = ecc_bit[3]^ecc_bit[213]^tmp[3];
			CheckBits[210] = ecc_bit[0]^ecc_bit[3]^ecc_bit[214]^tmp[0]^tmp[3];
			CheckBits[211] = ecc_bit[0]^ecc_bit[1]^ecc_bit[3]^ecc_bit[215]^tmp[0]^tmp[1]^tmp[3];
			CheckBits[212] = ecc_bit[0]^ecc_bit[1]^ecc_bit[2]^ecc_bit[3]^ecc_bit[216]^tmp[0]^tmp[1]^tmp[2]^tmp[3];
			CheckBits[213] = ecc_bit[0]^ecc_bit[1]^ecc_bit[2]^ecc_bit[217]^tmp[0]^tmp[1]^tmp[2];
			CheckBits[214] = ecc_bit[0]^ecc_bit[1]^ecc_bit[2]^ecc_bit[3]^ecc_bit[218]^tmp[0]^tmp[1]^tmp[2]^tmp[3];
			CheckBits[215] = ecc_bit[1]^ecc_bit[2]^ecc_bit[219]^tmp[1]^tmp[2];
			CheckBits[216] = ecc_bit[2]^ecc_bit[3]^ecc_bit[220]^tmp[2]^tmp[3];
			CheckBits[217] = ecc_bit[221];
			CheckBits[218] = ecc_bit[222];
			CheckBits[219] = ecc_bit[223];
			CheckBits[220] = ecc_bit[0]^ecc_bit[224]^tmp[0];
			CheckBits[221] = ecc_bit[1]^ecc_bit[225]^tmp[1];
			CheckBits[222] = ecc_bit[2]^ecc_bit[226]^tmp[2];
			CheckBits[223] = ecc_bit[0]^ecc_bit[3]^ecc_bit[227]^tmp[0]^tmp[3];
			CheckBits[224] = ecc_bit[1]^ecc_bit[3]^ecc_bit[228]^tmp[1]^tmp[3];
			CheckBits[225] = ecc_bit[2]^ecc_bit[3]^ecc_bit[229]^tmp[2]^tmp[3];
			CheckBits[226] = ecc_bit[230];
			CheckBits[227] = ecc_bit[0]^ecc_bit[231]^tmp[0];
			CheckBits[228] = ecc_bit[1]^ecc_bit[232]^tmp[1];
			CheckBits[229] = ecc_bit[0]^ecc_bit[2]^ecc_bit[233]^tmp[0]^tmp[2];
			CheckBits[230] = ecc_bit[0]^ecc_bit[1]^ecc_bit[3]^ecc_bit[234]^tmp[0]^tmp[1]^tmp[3];
			CheckBits[231] = ecc_bit[0]^ecc_bit[1]^ecc_bit[2]^ecc_bit[3]^ecc_bit[235]^tmp[0]^tmp[1]^tmp[2]^tmp[3];
			CheckBits[232] = ecc_bit[1]^ecc_bit[2]^ecc_bit[236]^tmp[1]^tmp[2];
			CheckBits[233] = ecc_bit[2]^ecc_bit[3]^ecc_bit[237]^tmp[2]^tmp[3];
			CheckBits[234] = ecc_bit[238];
			CheckBits[235] = ecc_bit[0]^ecc_bit[239]^tmp[0];
			CheckBits[236] = ecc_bit[0]^ecc_bit[1]^ecc_bit[240]^tmp[0]^tmp[1];
			CheckBits[237] = ecc_bit[0]^ecc_bit[1]^ecc_bit[2]^ecc_bit[241]^tmp[0]^tmp[1]^tmp[2];
			CheckBits[238] = ecc_bit[1]^ecc_bit[2]^ecc_bit[3]^ecc_bit[242]^tmp[1]^tmp[2]^tmp[3];
			CheckBits[239] = ecc_bit[0]^ecc_bit[2]^ecc_bit[243]^tmp[0]^tmp[2];
			CheckBits[240] = ecc_bit[1]^ecc_bit[3]^ecc_bit[244]^tmp[1]^tmp[3];
			CheckBits[241] = ecc_bit[0]^ecc_bit[2]^ecc_bit[3]^ecc_bit[245]^tmp[0]^tmp[2]^tmp[3];
			CheckBits[242] = ecc_bit[0]^ecc_bit[1]^ecc_bit[246]^tmp[0]^tmp[1];
			CheckBits[243] = ecc_bit[0]^ecc_bit[1]^ecc_bit[2]^ecc_bit[247]^tmp[0]^tmp[1]^tmp[2];
			CheckBits[244] = ecc_bit[0]^ecc_bit[1]^ecc_bit[2]^ecc_bit[3]^ecc_bit[248]^tmp[0]^tmp[1]^tmp[2]^tmp[3];
			CheckBits[245] = ecc_bit[1]^ecc_bit[2]^ecc_bit[249]^tmp[1]^tmp[2];
			CheckBits[246] = ecc_bit[2]^ecc_bit[3]^ecc_bit[250]^tmp[2]^tmp[3];
			CheckBits[247] = ecc_bit[0]^ecc_bit[251]^tmp[0];
			CheckBits[248] = ecc_bit[0]^ecc_bit[1]^ecc_bit[252]^tmp[0]^tmp[1];
			CheckBits[249] = ecc_bit[0]^ecc_bit[1]^ecc_bit[2]^ecc_bit[253]^tmp[0]^tmp[1]^tmp[2];
			CheckBits[250] = ecc_bit[1]^ecc_bit[2]^ecc_bit[3]^ecc_bit[254]^tmp[1]^tmp[2]^tmp[3];
			CheckBits[251] = ecc_bit[0]^ecc_bit[2]^ecc_bit[255]^tmp[0]^tmp[2];
			CheckBits[252] = ecc_bit[1]^ecc_bit[3]^ecc_bit[256]^tmp[1]^tmp[3];
			CheckBits[253] = ecc_bit[2]^ecc_bit[3]^ecc_bit[257]^tmp[2]^tmp[3];
			CheckBits[254] = ecc_bit[258];
			CheckBits[255] = ecc_bit[259];
			CheckBits[256] = ecc_bit[0]^ecc_bit[260]^tmp[0];
			CheckBits[257] = ecc_bit[1]^ecc_bit[261]^tmp[1];
			CheckBits[258] = ecc_bit[2]^ecc_bit[262]^tmp[2];
			CheckBits[259] = ecc_bit[0]^ecc_bit[3]^ecc_bit[263]^tmp[0]^tmp[3];
			CheckBits[260] = ecc_bit[0]^ecc_bit[1]^ecc_bit[3]^ecc_bit[264]^tmp[0]^tmp[1]^tmp[3];
			CheckBits[261] = ecc_bit[0]^ecc_bit[1]^ecc_bit[2]^ecc_bit[3]^ecc_bit[265]^tmp[0]^tmp[1]^tmp[2]^tmp[3];
			CheckBits[262] = ecc_bit[1]^ecc_bit[2]^ecc_bit[266]^tmp[1]^tmp[2];
			CheckBits[263] = ecc_bit[2]^ecc_bit[3]^ecc_bit[267]^tmp[2]^tmp[3];
			CheckBits[264] = ecc_bit[0]^ecc_bit[268]^tmp[0];
			CheckBits[265] = ecc_bit[0]^ecc_bit[1]^ecc_bit[269]^tmp[0]^tmp[1];
			CheckBits[266] = ecc_bit[0]^ecc_bit[1]^ecc_bit[2]^ecc_bit[270]^tmp[0]^tmp[1]^tmp[2];
			CheckBits[267] = ecc_bit[0]^ecc_bit[1]^ecc_bit[2]^ecc_bit[3]^ecc_bit[271]^tmp[0]^tmp[1]^tmp[2]^tmp[3];
			CheckBits[268] = ecc_bit[1]^ecc_bit[2]^ecc_bit[272]^tmp[1]^tmp[2];
			CheckBits[269] = ecc_bit[2]^ecc_bit[3]^ecc_bit[273]^tmp[2]^tmp[3];
			CheckBits[270] = ecc_bit[0]^ecc_bit[274]^tmp[0];
			CheckBits[271] = ecc_bit[1]^ecc_bit[275]^tmp[1];
			CheckBits[272] = ecc_bit[2]^ecc_bit[276]^tmp[2];
			CheckBits[273] = ecc_bit[0]^ecc_bit[3]^ecc_bit[277]^tmp[0]^tmp[3];
			CheckBits[274] = ecc_bit[1]^ecc_bit[3]^ecc_bit[278]^tmp[1]^tmp[3];
			CheckBits[275] = ecc_bit[2]^ecc_bit[3]^ecc_bit[279]^tmp[2]^tmp[3];
			CheckBits[276] = ecc_bit[0]^ecc_bit[280]^tmp[0];
			CheckBits[277] = ecc_bit[1]^ecc_bit[281]^tmp[1];
			CheckBits[278] = ecc_bit[0]^ecc_bit[2]^ecc_bit[282]^tmp[0]^tmp[2];
			CheckBits[279] = ecc_bit[0]^ecc_bit[1]^ecc_bit[3]^ecc_bit[283]^tmp[0]^tmp[1]^tmp[3];
			CheckBits[280] = ecc_bit[1]^ecc_bit[2]^ecc_bit[3]^ecc_bit[284]^tmp[1]^tmp[2]^tmp[3];
			CheckBits[281] = ecc_bit[2]^ecc_bit[285]^tmp[2];
			CheckBits[282] = ecc_bit[3]^ecc_bit[286]^tmp[3];
			CheckBits[283] = ecc_bit[0]^ecc_bit[3]^ecc_bit[287]^tmp[0]^tmp[3];
			CheckBits[284] = ecc_bit[0]^ecc_bit[1]^ecc_bit[3]^ecc_bit[288]^tmp[0]^tmp[1]^tmp[3];
			CheckBits[285] = ecc_bit[0]^ecc_bit[1]^ecc_bit[2]^ecc_bit[3]^ecc_bit[289]^tmp[0]^tmp[1]^tmp[2]^tmp[3];
			CheckBits[286] = ecc_bit[0]^ecc_bit[1]^ecc_bit[2]^ecc_bit[290]^tmp[0]^tmp[1]^tmp[2];
			CheckBits[287] = ecc_bit[0]^ecc_bit[1]^ecc_bit[2]^ecc_bit[3]^ecc_bit[291]^tmp[0]^tmp[1]^tmp[2]^tmp[3];
			CheckBits[288] = ecc_bit[0]^ecc_bit[1]^ecc_bit[2]^ecc_bit[292]^tmp[0]^tmp[1]^tmp[2];
			CheckBits[289] = ecc_bit[1]^ecc_bit[2]^ecc_bit[3]^ecc_bit[293]^tmp[1]^tmp[2]^tmp[3];
			CheckBits[290] = ecc_bit[2]^ecc_bit[294]^tmp[2];
			CheckBits[291] = ecc_bit[0]^ecc_bit[3]^ecc_bit[295]^tmp[0]^tmp[3];
			CheckBits[292] = ecc_bit[1]^ecc_bit[3]^ecc_bit[296]^tmp[1]^tmp[3];
			CheckBits[293] = ecc_bit[2]^ecc_bit[3]^ecc_bit[297]^tmp[2]^tmp[3];
			CheckBits[294] = ecc_bit[0]^ecc_bit[298]^tmp[0];
			CheckBits[295] = ecc_bit[0]^ecc_bit[1]^ecc_bit[299]^tmp[0]^tmp[1];
			CheckBits[296] = ecc_bit[1]^ecc_bit[2]^ecc_bit[300]^tmp[1]^tmp[2];
			CheckBits[297] = ecc_bit[2]^ecc_bit[3]^ecc_bit[301]^tmp[2]^tmp[3];
			CheckBits[298] = ecc_bit[302];
			CheckBits[299] = ecc_bit[0]^ecc_bit[303]^tmp[0];
			CheckBits[300] = ecc_bit[1]^ecc_bit[304]^tmp[1];
			CheckBits[301] = ecc_bit[0]^ecc_bit[2]^ecc_bit[305]^tmp[0]^tmp[2];
			CheckBits[302] = ecc_bit[0]^ecc_bit[1]^ecc_bit[3]^ecc_bit[306]^tmp[0]^tmp[1]^tmp[3];
			CheckBits[303] = ecc_bit[1]^ecc_bit[2]^ecc_bit[3]^ecc_bit[307]^tmp[1]^tmp[2]^tmp[3];
			CheckBits[304] = ecc_bit[2]^ecc_bit[308]^tmp[2];
			CheckBits[305] = ecc_bit[0]^ecc_bit[3]^ecc_bit[309]^tmp[0]^tmp[3];
			CheckBits[306] = ecc_bit[1]^ecc_bit[3]^ecc_bit[310]^tmp[1]^tmp[3];
			CheckBits[307] = ecc_bit[0]^ecc_bit[2]^ecc_bit[3]^ecc_bit[311]^tmp[0]^tmp[2]^tmp[3];
			CheckBits[308] = ecc_bit[1]^ecc_bit[312]^tmp[1];
			CheckBits[309] = ecc_bit[2]^ecc_bit[313]^tmp[2];
			CheckBits[310] = ecc_bit[0]^ecc_bit[3]^ecc_bit[314]^tmp[0]^tmp[3];
			CheckBits[311] = ecc_bit[1]^ecc_bit[3]^ecc_bit[315]^tmp[1]^tmp[3];
			CheckBits[312] = ecc_bit[2]^ecc_bit[3]^ecc_bit[316]^tmp[2]^tmp[3];
			CheckBits[313] = ecc_bit[0]^ecc_bit[317]^tmp[0];
			CheckBits[314] = ecc_bit[1]^ecc_bit[318]^tmp[1];
			CheckBits[315] = ecc_bit[0]^ecc_bit[2]^ecc_bit[319]^tmp[0]^tmp[2];
			CheckBits[316] = ecc_bit[0]^ecc_bit[1]^ecc_bit[3]^ecc_bit[320]^tmp[0]^tmp[1]^tmp[3];
			CheckBits[317] = ecc_bit[0]^ecc_bit[1]^ecc_bit[2]^ecc_bit[3]^ecc_bit[321]^tmp[0]^tmp[1]^tmp[2]^tmp[3];
			CheckBits[318] = ecc_bit[0]^ecc_bit[1]^ecc_bit[2]^ecc_bit[322]^tmp[0]^tmp[1]^tmp[2];
			CheckBits[319] = ecc_bit[1]^ecc_bit[2]^ecc_bit[3]^ecc_bit[323]^tmp[1]^tmp[2]^tmp[3];
			CheckBits[320] = ecc_bit[2]^ecc_bit[324]^tmp[2];
			CheckBits[321] = ecc_bit[0]^ecc_bit[3]^ecc_bit[325]^tmp[0]^tmp[3];
			CheckBits[322] = ecc_bit[1]^ecc_bit[3]^ecc_bit[326]^tmp[1]^tmp[3];
			CheckBits[323] = ecc_bit[0]^ecc_bit[2]^ecc_bit[3]^ecc_bit[327]^tmp[0]^tmp[2]^tmp[3];
			CheckBits[324] = ecc_bit[1]^ecc_bit[328]^tmp[1];
			CheckBits[325] = ecc_bit[0]^ecc_bit[2]^ecc_bit[329]^tmp[0]^tmp[2];
			CheckBits[326] = ecc_bit[1]^ecc_bit[3]^ecc_bit[330]^tmp[1]^tmp[3];
			CheckBits[327] = ecc_bit[0]^ecc_bit[2]^ecc_bit[3]^ecc_bit[331]^tmp[0]^tmp[2]^tmp[3];
			CheckBits[328] = ecc_bit[0]^ecc_bit[1]^ecc_bit[332]^tmp[0]^tmp[1];
			CheckBits[329] = ecc_bit[0]^ecc_bit[1]^ecc_bit[2]^ecc_bit[333]^tmp[0]^tmp[1]^tmp[2];
			CheckBits[330] = ecc_bit[1]^ecc_bit[2]^ecc_bit[3]^ecc_bit[334]^tmp[1]^tmp[2]^tmp[3];
			CheckBits[331] = ecc_bit[2]^ecc_bit[335]^tmp[2];
			CheckBits[332] = ecc_bit[0]^ecc_bit[3]^ecc_bit[336]^tmp[0]^tmp[3];
			CheckBits[333] = ecc_bit[1]^ecc_bit[3]^ecc_bit[337]^tmp[1]^tmp[3];
			CheckBits[334] = ecc_bit[0]^ecc_bit[2]^ecc_bit[3]^ecc_bit[338]^tmp[0]^tmp[2]^tmp[3];
			CheckBits[335] = ecc_bit[1]^ecc_bit[339]^tmp[1];
			CheckBits[336] = ecc_bit[0]^ecc_bit[2]^ecc_bit[340]^tmp[0]^tmp[2];
			CheckBits[337] = ecc_bit[0]^ecc_bit[1]^ecc_bit[3]^ecc_bit[341]^tmp[0]^tmp[1]^tmp[3];
			CheckBits[338] = ecc_bit[0]^ecc_bit[1]^ecc_bit[2]^ecc_bit[3]^ecc_bit[342]^tmp[0]^tmp[1]^tmp[2]^tmp[3];
			CheckBits[339] = ecc_bit[0]^ecc_bit[1]^ecc_bit[2]^ecc_bit[343]^tmp[0]^tmp[1]^tmp[2];
			CheckBits[340] = ecc_bit[0]^ecc_bit[1]^ecc_bit[2]^ecc_bit[3]^ecc_bit[344]^tmp[0]^tmp[1]^tmp[2]^tmp[3];
			CheckBits[341] = ecc_bit[0]^ecc_bit[1]^ecc_bit[2]^ecc_bit[345]^tmp[0]^tmp[1]^tmp[2];
			CheckBits[342] = ecc_bit[1]^ecc_bit[2]^ecc_bit[3]^ecc_bit[346]^tmp[1]^tmp[2]^tmp[3];
			CheckBits[343] = ecc_bit[2]^ecc_bit[347]^tmp[2];
			CheckBits[344] = ecc_bit[0]^ecc_bit[3]^ecc_bit[348]^tmp[0]^tmp[3];
			CheckBits[345] = ecc_bit[0]^ecc_bit[1]^ecc_bit[3]^ecc_bit[349]^tmp[0]^tmp[1]^tmp[3];
			CheckBits[346] = ecc_bit[1]^ecc_bit[2]^ecc_bit[3]^ecc_bit[350]^tmp[1]^tmp[2]^tmp[3];
			CheckBits[347] = ecc_bit[0]^ecc_bit[2]^ecc_bit[351]^tmp[0]^tmp[2];
			CheckBits[348] = ecc_bit[0]^ecc_bit[1]^ecc_bit[3]^ecc_bit[352]^tmp[0]^tmp[1]^tmp[3];
			CheckBits[349] = ecc_bit[1]^ecc_bit[2]^ecc_bit[3]^ecc_bit[353]^tmp[1]^tmp[2]^tmp[3];
			CheckBits[350] = ecc_bit[0]^ecc_bit[2]^ecc_bit[354]^tmp[0]^tmp[2];
			CheckBits[351] = ecc_bit[0]^ecc_bit[1]^ecc_bit[3]^ecc_bit[355]^tmp[0]^tmp[1]^tmp[3];
			CheckBits[352] = ecc_bit[1]^ecc_bit[2]^ecc_bit[3]^ecc_bit[356]^tmp[1]^tmp[2]^tmp[3];
			CheckBits[353] = ecc_bit[0]^ecc_bit[2]^ecc_bit[357]^tmp[0]^tmp[2];
			CheckBits[354] = ecc_bit[0]^ecc_bit[1]^ecc_bit[3]^ecc_bit[358]^tmp[0]^tmp[1]^tmp[3];
			CheckBits[355] = ecc_bit[1]^ecc_bit[2]^ecc_bit[3]^ecc_bit[359]^tmp[1]^tmp[2]^tmp[3];
			CheckBits[356] = ecc_bit[2]^ecc_bit[360]^tmp[2];
			CheckBits[357] = ecc_bit[0]^ecc_bit[3]^ecc_bit[361]^tmp[0]^tmp[3];
			CheckBits[358] = ecc_bit[1]^ecc_bit[3]^ecc_bit[362]^tmp[1]^tmp[3];
			CheckBits[359] = ecc_bit[0]^ecc_bit[2]^ecc_bit[3]^ecc_bit[363]^tmp[0]^tmp[2]^tmp[3];
			CheckBits[360] = ecc_bit[0]^ecc_bit[1]^ecc_bit[364]^tmp[0]^tmp[1];
			CheckBits[361] = ecc_bit[0]^ecc_bit[1]^ecc_bit[2]^ecc_bit[365]^tmp[0]^tmp[1]^tmp[2];
			CheckBits[362] = ecc_bit[0]^ecc_bit[1]^ecc_bit[2]^ecc_bit[3]^ecc_bit[366]^tmp[0]^tmp[1]^tmp[2]^tmp[3];
			CheckBits[363] = ecc_bit[0]^ecc_bit[1]^ecc_bit[2]^ecc_bit[367]^tmp[0]^tmp[1]^tmp[2];
			CheckBits[364] = ecc_bit[1]^ecc_bit[2]^ecc_bit[3]^ecc_bit[368]^tmp[1]^tmp[2]^tmp[3];
			CheckBits[365] = ecc_bit[2]^ecc_bit[369]^tmp[2];
			CheckBits[366] = ecc_bit[3]^ecc_bit[370]^tmp[3];
			CheckBits[367] = ecc_bit[3]^ecc_bit[371]^tmp[3];
			CheckBits[368] = ecc_bit[0]^ecc_bit[3]^ecc_bit[372]^tmp[0]^tmp[3];
			CheckBits[369] = ecc_bit[0]^ecc_bit[1]^ecc_bit[3]^ecc_bit[373]^tmp[0]^tmp[1]^tmp[3];
			CheckBits[370] = ecc_bit[0]^ecc_bit[1]^ecc_bit[2]^ecc_bit[3]^ecc_bit[374]^tmp[0]^tmp[1]^tmp[2]^tmp[3];
			CheckBits[371] = ecc_bit[1]^ecc_bit[2]^ecc_bit[375]^tmp[1]^tmp[2];
			CheckBits[372] = ecc_bit[0]^ecc_bit[2]^ecc_bit[3]^ecc_bit[376]^tmp[0]^tmp[2]^tmp[3];
			CheckBits[373] = ecc_bit[0]^ecc_bit[1]^ecc_bit[377]^tmp[0]^tmp[1];
			CheckBits[374] = ecc_bit[0]^ecc_bit[1]^ecc_bit[2]^ecc_bit[378]^tmp[0]^tmp[1]^tmp[2];
			CheckBits[375] = ecc_bit[0]^ecc_bit[1]^ecc_bit[2]^ecc_bit[3]^ecc_bit[379]^tmp[0]^tmp[1]^tmp[2]^tmp[3];
			CheckBits[376] = ecc_bit[0]^ecc_bit[1]^ecc_bit[2]^ecc_bit[380]^tmp[0]^tmp[1]^tmp[2];
			CheckBits[377] = ecc_bit[1]^ecc_bit[2]^ecc_bit[3]^ecc_bit[381]^tmp[1]^tmp[2]^tmp[3];
			CheckBits[378] = ecc_bit[2]^ecc_bit[382]^tmp[2];
			CheckBits[379] = ecc_bit[3]^ecc_bit[383]^tmp[3];
			CheckBits[380] = ecc_bit[0]^ecc_bit[3]^ecc_bit[384]^tmp[0]^tmp[3];
			CheckBits[381] = ecc_bit[1]^ecc_bit[3]^ecc_bit[385]^tmp[1]^tmp[3];
			CheckBits[382] = ecc_bit[0]^ecc_bit[2]^ecc_bit[3]^ecc_bit[386]^tmp[0]^tmp[2]^tmp[3];
			CheckBits[383] = ecc_bit[0]^ecc_bit[1]^ecc_bit[387]^tmp[0]^tmp[1];
			CheckBits[384] = ecc_bit[1]^ecc_bit[2]^ecc_bit[388]^tmp[1]^tmp[2];
			CheckBits[385] = ecc_bit[0]^ecc_bit[2]^ecc_bit[3]^ecc_bit[389]^tmp[0]^tmp[2]^tmp[3];
			CheckBits[386] = ecc_bit[0]^ecc_bit[1]^ecc_bit[390]^tmp[0]^tmp[1];
			CheckBits[387] = ecc_bit[1]^ecc_bit[2]^ecc_bit[391]^tmp[1]^tmp[2];
			CheckBits[388] = ecc_bit[2]^ecc_bit[3]^ecc_bit[392]^tmp[2]^tmp[3];
			CheckBits[389] = ecc_bit[0]^ecc_bit[393]^tmp[0];
			CheckBits[390] = ecc_bit[0]^ecc_bit[1]^ecc_bit[394]^tmp[0]^tmp[1];
			CheckBits[391] = ecc_bit[0]^ecc_bit[1]^ecc_bit[2]^ecc_bit[395]^tmp[0]^tmp[1]^tmp[2];
			CheckBits[392] = ecc_bit[0]^ecc_bit[1]^ecc_bit[2]^ecc_bit[3]^ecc_bit[396]^tmp[0]^tmp[1]^tmp[2]^tmp[3];
			CheckBits[393] = ecc_bit[1]^ecc_bit[2]^ecc_bit[397]^tmp[1]^tmp[2];
			CheckBits[394] = ecc_bit[2]^ecc_bit[3]^ecc_bit[398]^tmp[2]^tmp[3];
			CheckBits[395] = ecc_bit[0]^ecc_bit[399]^tmp[0];
			CheckBits[396] = ecc_bit[0]^ecc_bit[1]^ecc_bit[400]^tmp[0]^tmp[1];
			CheckBits[397] = ecc_bit[0]^ecc_bit[1]^ecc_bit[2]^ecc_bit[401]^tmp[0]^tmp[1]^tmp[2];
			CheckBits[398] = ecc_bit[0]^ecc_bit[1]^ecc_bit[2]^ecc_bit[3]^ecc_bit[402]^tmp[0]^tmp[1]^tmp[2]^tmp[3];
			CheckBits[399] = ecc_bit[0]^ecc_bit[1]^ecc_bit[2]^ecc_bit[403]^tmp[0]^tmp[1]^tmp[2];
			CheckBits[400] = ecc_bit[1]^ecc_bit[2]^ecc_bit[3]^ecc_bit[404]^tmp[1]^tmp[2]^tmp[3];
			CheckBits[401] = ecc_bit[2]^ecc_bit[405]^tmp[2];
			CheckBits[402] = ecc_bit[0]^ecc_bit[3]^ecc_bit[406]^tmp[0]^tmp[3];
			CheckBits[403] = ecc_bit[0]^ecc_bit[1]^ecc_bit[3]^ecc_bit[407]^tmp[0]^tmp[1]^tmp[3];
			CheckBits[404] = ecc_bit[1]^ecc_bit[2]^ecc_bit[3]^ecc_bit[408]^tmp[1]^tmp[2]^tmp[3];
			CheckBits[405] = ecc_bit[0]^ecc_bit[2]^ecc_bit[409]^tmp[0]^tmp[2];
			CheckBits[406] = ecc_bit[1]^ecc_bit[3]^ecc_bit[410]^tmp[1]^tmp[3];
			CheckBits[407] = ecc_bit[2]^ecc_bit[3]^ecc_bit[411]^tmp[2]^tmp[3];
			CheckBits[408] = ecc_bit[0]^ecc_bit[412]^tmp[0];
			CheckBits[409] = ecc_bit[1]^ecc_bit[413]^tmp[1];
			CheckBits[410] = ecc_bit[2]^ecc_bit[414]^tmp[2];
			CheckBits[411] = ecc_bit[0]^ecc_bit[3]^ecc_bit[415]^tmp[0]^tmp[3];
			CheckBits[412] = ecc_bit[0]^ecc_bit[1]^ecc_bit[3]^ecc_bit[416]^tmp[0]^tmp[1]^tmp[3];
			CheckBits[413] = ecc_bit[1]^ecc_bit[2]^ecc_bit[3]^ecc_bit[417]^tmp[1]^tmp[2]^tmp[3];
			CheckBits[414] = ecc_bit[0]^ecc_bit[2]^ecc_bit[418]^tmp[0]^tmp[2];
			CheckBits[415] = ecc_bit[0]^ecc_bit[1]^ecc_bit[3]^ecc_bit[419]^tmp[0]^tmp[1]^tmp[3];
			CheckBits[416] = ecc_bit[1]^ecc_bit[2]^ecc_bit[3]^tmp[1]^tmp[2]^tmp[3];
			CheckBits[417] = ecc_bit[0]^ecc_bit[2]^tmp[0]^tmp[2];
			CheckBits[418] = ecc_bit[1]^ecc_bit[3]^tmp[1]^tmp[3];
			CheckBits[419] = ecc_bit[2]^ecc_bit[3]^tmp[2]^tmp[3];
		} else if(ecc_size == 46) {
			CheckBits[0] = ecc_bit[2]^ecc_bit[4]^tmp[2];
			CheckBits[1] = ecc_bit[0]^ecc_bit[3]^ecc_bit[5]^tmp[0]^tmp[3];
			CheckBits[2] = ecc_bit[0]^ecc_bit[1]^ecc_bit[2]^ecc_bit[6]^tmp[0]^tmp[1]^tmp[2];
			CheckBits[3] = ecc_bit[0]^ecc_bit[1]^ecc_bit[2]^ecc_bit[3]^ecc_bit[7]^tmp[0]^tmp[1]^tmp[2]^tmp[3];
			CheckBits[4] = ecc_bit[1]^ecc_bit[3]^ecc_bit[8]^tmp[1]^tmp[3];
			CheckBits[5] = ecc_bit[9];
			CheckBits[6] = ecc_bit[0]^ecc_bit[10]^tmp[0];
			CheckBits[7] = ecc_bit[1]^ecc_bit[11]^tmp[1];
			CheckBits[8] = ecc_bit[0]^ecc_bit[2]^ecc_bit[12]^tmp[0]^tmp[2];
			CheckBits[9] = ecc_bit[1]^ecc_bit[3]^ecc_bit[13]^tmp[1]^tmp[3];
			CheckBits[10] = ecc_bit[14];
			CheckBits[11] = ecc_bit[0]^ecc_bit[15]^tmp[0];
			CheckBits[12] = ecc_bit[0]^ecc_bit[1]^ecc_bit[16]^tmp[0]^tmp[1];
			CheckBits[13] = ecc_bit[1]^ecc_bit[2]^ecc_bit[17]^tmp[1]^tmp[2];
			CheckBits[14] = ecc_bit[2]^ecc_bit[3]^ecc_bit[18]^tmp[2]^tmp[3];
			CheckBits[15] = ecc_bit[0]^ecc_bit[2]^ecc_bit[3]^ecc_bit[19]^tmp[0]^tmp[2]^tmp[3];
			CheckBits[16] = ecc_bit[1]^ecc_bit[2]^ecc_bit[3]^ecc_bit[20]^tmp[1]^tmp[2]^tmp[3];
			CheckBits[17] = ecc_bit[0]^ecc_bit[3]^ecc_bit[21]^tmp[0]^tmp[3];
			CheckBits[18] = ecc_bit[1]^ecc_bit[2]^ecc_bit[22]^tmp[1]^tmp[2];
			CheckBits[19] = ecc_bit[2]^ecc_bit[3]^ecc_bit[23]^tmp[2]^tmp[3];
			CheckBits[20] = ecc_bit[2]^ecc_bit[3]^ecc_bit[24]^tmp[2]^tmp[3];
			CheckBits[21] = ecc_bit[2]^ecc_bit[3]^ecc_bit[25]^tmp[2]^tmp[3];
			CheckBits[22] = ecc_bit[2]^ecc_bit[3]^ecc_bit[26]^tmp[2]^tmp[3];
			CheckBits[23] = ecc_bit[0]^ecc_bit[2]^ecc_bit[3]^ecc_bit[27]^tmp[0]^tmp[2]^tmp[3];
			CheckBits[24] = ecc_bit[0]^ecc_bit[1]^ecc_bit[2]^ecc_bit[3]^ecc_bit[28]^tmp[0]^tmp[1]^tmp[2]^tmp[3];
			CheckBits[25] = ecc_bit[1]^ecc_bit[3]^ecc_bit[29]^tmp[1]^tmp[3];
			CheckBits[26] = ecc_bit[0]^ecc_bit[30]^tmp[0];
			CheckBits[27] = ecc_bit[0]^ecc_bit[1]^ecc_bit[31]^tmp[0]^tmp[1];
			CheckBits[28] = ecc_bit[0]^ecc_bit[1]^ecc_bit[2]^ecc_bit[32]^tmp[0]^tmp[1]^tmp[2];
			CheckBits[29] = ecc_bit[0]^ecc_bit[1]^ecc_bit[2]^ecc_bit[3]^ecc_bit[33]^tmp[0]^tmp[1]^tmp[2]^tmp[3];
			CheckBits[30] = ecc_bit[1]^ecc_bit[3]^ecc_bit[34]^tmp[1]^tmp[3];
			CheckBits[31] = ecc_bit[35];
			CheckBits[32] = ecc_bit[0]^ecc_bit[36]^tmp[0];
			CheckBits[33] = ecc_bit[1]^ecc_bit[37]^tmp[1];
			CheckBits[34] = ecc_bit[2]^ecc_bit[38]^tmp[2];
			CheckBits[35] = ecc_bit[0]^ecc_bit[3]^ecc_bit[39]^tmp[0]^tmp[3];
			CheckBits[36] = ecc_bit[1]^ecc_bit[2]^ecc_bit[40]^tmp[1]^tmp[2];
			CheckBits[37] = ecc_bit[0]^ecc_bit[2]^ecc_bit[3]^ecc_bit[41]^tmp[0]^tmp[2]^tmp[3];
			CheckBits[38] = ecc_bit[0]^ecc_bit[1]^ecc_bit[2]^ecc_bit[3]^ecc_bit[42]^tmp[0]^tmp[1]^tmp[2]^tmp[3];
			CheckBits[39] = ecc_bit[0]^ecc_bit[1]^ecc_bit[3]^ecc_bit[43]^tmp[0]^tmp[1]^tmp[3];
			CheckBits[40] = ecc_bit[1]^ecc_bit[44]^tmp[1];
			CheckBits[41] = ecc_bit[0]^ecc_bit[2]^ecc_bit[45]^tmp[0]^tmp[2];
			CheckBits[42] = ecc_bit[0]^ecc_bit[1]^ecc_bit[3]^ecc_bit[46]^tmp[0]^tmp[1]^tmp[3];
			CheckBits[43] = ecc_bit[0]^ecc_bit[1]^ecc_bit[47]^tmp[0]^tmp[1];
			CheckBits[44] = ecc_bit[0]^ecc_bit[1]^ecc_bit[2]^ecc_bit[48]^tmp[0]^tmp[1]^tmp[2];
			CheckBits[45] = ecc_bit[1]^ecc_bit[2]^ecc_bit[3]^ecc_bit[49]^tmp[1]^tmp[2]^tmp[3];
			CheckBits[46] = ecc_bit[3]^ecc_bit[50]^tmp[3];
			CheckBits[47] = ecc_bit[2]^ecc_bit[51]^tmp[2];
			CheckBits[48] = ecc_bit[3]^ecc_bit[52]^tmp[3];
			CheckBits[49] = ecc_bit[0]^ecc_bit[2]^ecc_bit[53]^tmp[0]^tmp[2];
			CheckBits[50] = ecc_bit[0]^ecc_bit[1]^ecc_bit[3]^ecc_bit[54]^tmp[0]^tmp[1]^tmp[3];
			CheckBits[51] = ecc_bit[0]^ecc_bit[1]^ecc_bit[55]^tmp[0]^tmp[1];
			CheckBits[52] = ecc_bit[1]^ecc_bit[2]^ecc_bit[56]^tmp[1]^tmp[2];
			CheckBits[53] = ecc_bit[2]^ecc_bit[3]^ecc_bit[57]^tmp[2]^tmp[3];
			CheckBits[54] = ecc_bit[0]^ecc_bit[2]^ecc_bit[3]^ecc_bit[58]^tmp[0]^tmp[2]^tmp[3];
			CheckBits[55] = ecc_bit[0]^ecc_bit[1]^ecc_bit[2]^ecc_bit[3]^ecc_bit[59]^tmp[0]^tmp[1]^tmp[2]^tmp[3];
			CheckBits[56] = ecc_bit[1]^ecc_bit[3]^ecc_bit[60]^tmp[1]^tmp[3];
			CheckBits[57] = ecc_bit[0]^ecc_bit[61]^tmp[0];
			CheckBits[58] = ecc_bit[0]^ecc_bit[1]^ecc_bit[62]^tmp[0]^tmp[1];
			CheckBits[59] = ecc_bit[1]^ecc_bit[2]^ecc_bit[63]^tmp[1]^tmp[2];
			CheckBits[60] = ecc_bit[2]^ecc_bit[3]^ecc_bit[64]^tmp[2]^tmp[3];
			CheckBits[61] = ecc_bit[0]^ecc_bit[2]^ecc_bit[3]^ecc_bit[65]^tmp[0]^tmp[2]^tmp[3];
			CheckBits[62] = ecc_bit[1]^ecc_bit[2]^ecc_bit[3]^ecc_bit[66]^tmp[1]^tmp[2]^tmp[3];
			CheckBits[63] = ecc_bit[0]^ecc_bit[3]^ecc_bit[67]^tmp[0]^tmp[3];
			CheckBits[64] = ecc_bit[1]^ecc_bit[2]^ecc_bit[68]^tmp[1]^tmp[2];
			CheckBits[65] = ecc_bit[0]^ecc_bit[2]^ecc_bit[3]^ecc_bit[69]^tmp[0]^tmp[2]^tmp[3];
			CheckBits[66] = ecc_bit[1]^ecc_bit[2]^ecc_bit[3]^ecc_bit[70]^tmp[1]^tmp[2]^tmp[3];
			CheckBits[67] = ecc_bit[0]^ecc_bit[3]^ecc_bit[71]^tmp[0]^tmp[3];
			CheckBits[68] = ecc_bit[0]^ecc_bit[1]^ecc_bit[2]^ecc_bit[72]^tmp[0]^tmp[1]^tmp[2];
			CheckBits[69] = ecc_bit[1]^ecc_bit[2]^ecc_bit[3]^ecc_bit[73]^tmp[1]^tmp[2]^tmp[3];
			CheckBits[70] = ecc_bit[3]^ecc_bit[74]^tmp[3];
			CheckBits[71] = ecc_bit[0]^ecc_bit[2]^ecc_bit[75]^tmp[0]^tmp[2];
			CheckBits[72] = ecc_bit[1]^ecc_bit[3]^ecc_bit[76]^tmp[1]^tmp[3];
			CheckBits[73] = ecc_bit[77];
			CheckBits[74] = ecc_bit[0]^ecc_bit[78]^tmp[0];
			CheckBits[75] = ecc_bit[0]^ecc_bit[1]^ecc_bit[79]^tmp[0]^tmp[1];
			CheckBits[76] = ecc_bit[1]^ecc_bit[2]^ecc_bit[80]^tmp[1]^tmp[2];
			CheckBits[77] = ecc_bit[2]^ecc_bit[3]^ecc_bit[81]^tmp[2]^tmp[3];
			CheckBits[78] = ecc_bit[2]^ecc_bit[3]^ecc_bit[82]^tmp[2]^tmp[3];
			CheckBits[79] = ecc_bit[2]^ecc_bit[3]^ecc_bit[83]^tmp[2]^tmp[3];
			CheckBits[80] = ecc_bit[0]^ecc_bit[2]^ecc_bit[3]^ecc_bit[84]^tmp[0]^tmp[2]^tmp[3];
			CheckBits[81] = ecc_bit[0]^ecc_bit[1]^ecc_bit[2]^ecc_bit[3]^ecc_bit[85]^tmp[0]^tmp[1]^tmp[2]^tmp[3];
			CheckBits[82] = ecc_bit[1]^ecc_bit[3]^ecc_bit[86]^tmp[1]^tmp[3];
			CheckBits[83] = ecc_bit[87];
			CheckBits[84] = ecc_bit[88];
			CheckBits[85] = ecc_bit[0]^ecc_bit[89]^tmp[0];
			CheckBits[86] = ecc_bit[0]^ecc_bit[1]^ecc_bit[90]^tmp[0]^tmp[1];
			CheckBits[87] = ecc_bit[1]^ecc_bit[2]^ecc_bit[91]^tmp[1]^tmp[2];
			CheckBits[88] = ecc_bit[2]^ecc_bit[3]^ecc_bit[92]^tmp[2]^tmp[3];
			CheckBits[89] = ecc_bit[0]^ecc_bit[2]^ecc_bit[3]^ecc_bit[93]^tmp[0]^tmp[2]^tmp[3];
			CheckBits[90] = ecc_bit[1]^ecc_bit[2]^ecc_bit[3]^ecc_bit[94]^tmp[1]^tmp[2]^tmp[3];
			CheckBits[91] = ecc_bit[3]^ecc_bit[95]^tmp[3];
			CheckBits[92] = ecc_bit[0]^ecc_bit[2]^ecc_bit[96]^tmp[0]^tmp[2];
			CheckBits[93] = ecc_bit[1]^ecc_bit[3]^ecc_bit[97]^tmp[1]^tmp[3];
			CheckBits[94] = ecc_bit[98];
			CheckBits[95] = ecc_bit[99];
			CheckBits[96] = ecc_bit[100];
			CheckBits[97] = ecc_bit[101];
			CheckBits[98] = ecc_bit[102];
			CheckBits[99] = ecc_bit[0]^ecc_bit[103]^tmp[0];
			CheckBits[100] = ecc_bit[0]^ecc_bit[1]^ecc_bit[104]^tmp[0]^tmp[1];
			CheckBits[101] = ecc_bit[0]^ecc_bit[1]^ecc_bit[2]^ecc_bit[105]^tmp[0]^tmp[1]^tmp[2];
			CheckBits[102] = ecc_bit[0]^ecc_bit[1]^ecc_bit[2]^ecc_bit[3]^ecc_bit[106]^tmp[0]^tmp[1]^tmp[2]^tmp[3];
			CheckBits[103] = ecc_bit[0]^ecc_bit[1]^ecc_bit[3]^ecc_bit[107]^tmp[0]^tmp[1]^tmp[3];
			CheckBits[104] = ecc_bit[0]^ecc_bit[1]^ecc_bit[108]^tmp[0]^tmp[1];
			CheckBits[105] = ecc_bit[1]^ecc_bit[2]^ecc_bit[109]^tmp[1]^tmp[2];
			CheckBits[106] = ecc_bit[2]^ecc_bit[3]^ecc_bit[110]^tmp[2]^tmp[3];
			CheckBits[107] = ecc_bit[2]^ecc_bit[3]^ecc_bit[111]^tmp[2]^tmp[3];
			CheckBits[108] = ecc_bit[2]^ecc_bit[3]^ecc_bit[112]^tmp[2]^tmp[3];
			CheckBits[109] = ecc_bit[0]^ecc_bit[2]^ecc_bit[3]^ecc_bit[113]^tmp[0]^tmp[2]^tmp[3];
			CheckBits[110] = ecc_bit[0]^ecc_bit[1]^ecc_bit[2]^ecc_bit[3]^ecc_bit[114]^tmp[0]^tmp[1]^tmp[2]^tmp[3];
			CheckBits[111] = ecc_bit[1]^ecc_bit[3]^ecc_bit[115]^tmp[1]^tmp[3];
			CheckBits[112] = ecc_bit[0]^ecc_bit[116]^tmp[0];
			CheckBits[113] = ecc_bit[0]^ecc_bit[1]^ecc_bit[117]^tmp[0]^tmp[1];
			CheckBits[114] = ecc_bit[0]^ecc_bit[1]^ecc_bit[2]^ecc_bit[118]^tmp[0]^tmp[1]^tmp[2];
			CheckBits[115] = ecc_bit[0]^ecc_bit[1]^ecc_bit[2]^ecc_bit[3]^ecc_bit[119]^tmp[0]^tmp[1]^tmp[2]^tmp[3];
			CheckBits[116] = ecc_bit[1]^ecc_bit[3]^ecc_bit[120]^tmp[1]^tmp[3];
			CheckBits[117] = ecc_bit[0]^ecc_bit[121]^tmp[0];
			CheckBits[118] = ecc_bit[0]^ecc_bit[1]^ecc_bit[122]^tmp[0]^tmp[1];
			CheckBits[119] = ecc_bit[0]^ecc_bit[1]^ecc_bit[2]^ecc_bit[123]^tmp[0]^tmp[1]^tmp[2];
			CheckBits[120] = ecc_bit[1]^ecc_bit[2]^ecc_bit[3]^ecc_bit[124]^tmp[1]^tmp[2]^tmp[3];
			CheckBits[121] = ecc_bit[0]^ecc_bit[3]^ecc_bit[125]^tmp[0]^tmp[3];
			CheckBits[122] = ecc_bit[1]^ecc_bit[2]^ecc_bit[126]^tmp[1]^tmp[2];
			CheckBits[123] = ecc_bit[0]^ecc_bit[2]^ecc_bit[3]^ecc_bit[127]^tmp[0]^tmp[2]^tmp[3];
			CheckBits[124] = ecc_bit[0]^ecc_bit[1]^ecc_bit[2]^ecc_bit[3]^ecc_bit[128]^tmp[0]^tmp[1]^tmp[2]^tmp[3];
			CheckBits[125] = ecc_bit[0]^ecc_bit[1]^ecc_bit[3]^ecc_bit[129]^tmp[0]^tmp[1]^tmp[3];
			CheckBits[126] = ecc_bit[0]^ecc_bit[1]^ecc_bit[130]^tmp[0]^tmp[1];
			CheckBits[127] = ecc_bit[0]^ecc_bit[1]^ecc_bit[2]^ecc_bit[131]^tmp[0]^tmp[1]^tmp[2];
			CheckBits[128] = ecc_bit[1]^ecc_bit[2]^ecc_bit[3]^ecc_bit[132]^tmp[1]^tmp[2]^tmp[3];
			CheckBits[129] = ecc_bit[0]^ecc_bit[3]^ecc_bit[133]^tmp[0]^tmp[3];
			CheckBits[130] = ecc_bit[1]^ecc_bit[2]^ecc_bit[134]^tmp[1]^tmp[2];
			CheckBits[131] = ecc_bit[2]^ecc_bit[3]^ecc_bit[135]^tmp[2]^tmp[3];
			CheckBits[132] = ecc_bit[2]^ecc_bit[3]^ecc_bit[136]^tmp[2]^tmp[3];
			CheckBits[133] = ecc_bit[0]^ecc_bit[2]^ecc_bit[3]^ecc_bit[137]^tmp[0]^tmp[2]^tmp[3];
			CheckBits[134] = ecc_bit[1]^ecc_bit[2]^ecc_bit[3]^ecc_bit[138]^tmp[1]^tmp[2]^tmp[3];
			CheckBits[135] = ecc_bit[3]^ecc_bit[139]^tmp[3];
			CheckBits[136] = ecc_bit[0]^ecc_bit[2]^ecc_bit[140]^tmp[0]^tmp[2];
			CheckBits[137] = ecc_bit[0]^ecc_bit[1]^ecc_bit[3]^ecc_bit[141]^tmp[0]^tmp[1]^tmp[3];
			CheckBits[138] = ecc_bit[0]^ecc_bit[1]^ecc_bit[142]^tmp[0]^tmp[1];
			CheckBits[139] = ecc_bit[0]^ecc_bit[1]^ecc_bit[2]^ecc_bit[143]^tmp[0]^tmp[1]^tmp[2];
			CheckBits[140] = ecc_bit[0]^ecc_bit[1]^ecc_bit[2]^ecc_bit[3]^ecc_bit[144]^tmp[0]^tmp[1]^tmp[2]^tmp[3];
			CheckBits[141] = ecc_bit[0]^ecc_bit[1]^ecc_bit[3]^ecc_bit[145]^tmp[0]^tmp[1]^tmp[3];
			CheckBits[142] = ecc_bit[0]^ecc_bit[1]^ecc_bit[146]^tmp[0]^tmp[1];
			CheckBits[143] = ecc_bit[1]^ecc_bit[2]^ecc_bit[147]^tmp[1]^tmp[2];
			CheckBits[144] = ecc_bit[2]^ecc_bit[3]^ecc_bit[148]^tmp[2]^tmp[3];
			CheckBits[145] = ecc_bit[2]^ecc_bit[3]^ecc_bit[149]^tmp[2]^tmp[3];
			CheckBits[146] = ecc_bit[2]^ecc_bit[3]^ecc_bit[150]^tmp[2]^tmp[3];
			CheckBits[147] = ecc_bit[0]^ecc_bit[2]^ecc_bit[3]^ecc_bit[151]^tmp[0]^tmp[2]^tmp[3];
			CheckBits[148] = ecc_bit[0]^ecc_bit[1]^ecc_bit[2]^ecc_bit[3]^ecc_bit[152]^tmp[0]^tmp[1]^tmp[2]^tmp[3];
			CheckBits[149] = ecc_bit[0]^ecc_bit[1]^ecc_bit[3]^ecc_bit[153]^tmp[0]^tmp[1]^tmp[3];
			CheckBits[150] = ecc_bit[1]^ecc_bit[154]^tmp[1];
			CheckBits[151] = ecc_bit[0]^ecc_bit[2]^ecc_bit[155]^tmp[0]^tmp[2];
			CheckBits[152] = ecc_bit[0]^ecc_bit[1]^ecc_bit[3]^ecc_bit[156]^tmp[0]^tmp[1]^tmp[3];
			CheckBits[153] = ecc_bit[0]^ecc_bit[1]^ecc_bit[157]^tmp[0]^tmp[1];
			CheckBits[154] = ecc_bit[1]^ecc_bit[2]^ecc_bit[158]^tmp[1]^tmp[2];
			CheckBits[155] = ecc_bit[2]^ecc_bit[3]^ecc_bit[159]^tmp[2]^tmp[3];
			CheckBits[156] = ecc_bit[0]^ecc_bit[2]^ecc_bit[3]^ecc_bit[160]^tmp[0]^tmp[2]^tmp[3];
			CheckBits[157] = ecc_bit[0]^ecc_bit[1]^ecc_bit[2]^ecc_bit[3]^ecc_bit[161]^tmp[0]^tmp[1]^tmp[2]^tmp[3];
			CheckBits[158] = ecc_bit[0]^ecc_bit[1]^ecc_bit[3]^ecc_bit[162]^tmp[0]^tmp[1]^tmp[3];
			CheckBits[159] = ecc_bit[1]^ecc_bit[163]^tmp[1];
			CheckBits[160] = ecc_bit[2]^ecc_bit[164]^tmp[2];
			CheckBits[161] = ecc_bit[3]^ecc_bit[165]^tmp[3];
			CheckBits[162] = ecc_bit[2]^ecc_bit[166]^tmp[2];
			CheckBits[163] = ecc_bit[3]^ecc_bit[167]^tmp[3];
			CheckBits[164] = ecc_bit[2]^ecc_bit[168]^tmp[2];
			CheckBits[165] = ecc_bit[3]^ecc_bit[169]^tmp[3];
			CheckBits[166] = ecc_bit[0]^ecc_bit[2]^ecc_bit[170]^tmp[0]^tmp[2];
			CheckBits[167] = ecc_bit[1]^ecc_bit[3]^ecc_bit[171]^tmp[1]^tmp[3];
			CheckBits[168] = ecc_bit[172];
			CheckBits[169] = ecc_bit[0]^ecc_bit[173]^tmp[0];
			CheckBits[170] = ecc_bit[0]^ecc_bit[1]^ecc_bit[174]^tmp[0]^tmp[1];
			CheckBits[171] = ecc_bit[0]^ecc_bit[1]^ecc_bit[2]^ecc_bit[175]^tmp[0]^tmp[1]^tmp[2];
			CheckBits[172] = ecc_bit[1]^ecc_bit[2]^ecc_bit[3]^ecc_bit[176]^tmp[1]^tmp[2]^tmp[3];
			CheckBits[173] = ecc_bit[0]^ecc_bit[3]^ecc_bit[177]^tmp[0]^tmp[3];
			CheckBits[174] = ecc_bit[1]^ecc_bit[2]^ecc_bit[178]^tmp[1]^tmp[2];
			CheckBits[175] = ecc_bit[2]^ecc_bit[3]^ecc_bit[179]^tmp[2]^tmp[3];
			CheckBits[176] = ecc_bit[0]^ecc_bit[2]^ecc_bit[3]^ecc_bit[180]^tmp[0]^tmp[2]^tmp[3];
			CheckBits[177] = ecc_bit[1]^ecc_bit[2]^ecc_bit[3]^ecc_bit[181]^tmp[1]^tmp[2]^tmp[3];
			CheckBits[178] = ecc_bit[0]^ecc_bit[3]^ecc_bit[182]^tmp[0]^tmp[3];
			CheckBits[179] = ecc_bit[0]^ecc_bit[1]^ecc_bit[2]^ecc_bit[183]^tmp[0]^tmp[1]^tmp[2];
			CheckBits[180] = ecc_bit[0]^ecc_bit[1]^ecc_bit[2]^ecc_bit[3]^ecc_bit[184]^tmp[0]^tmp[1]^tmp[2]^tmp[3];
			CheckBits[181] = ecc_bit[0]^ecc_bit[1]^ecc_bit[3]^ecc_bit[185]^tmp[0]^tmp[1]^tmp[3];
			CheckBits[182] = ecc_bit[1]^ecc_bit[186]^tmp[1];
			CheckBits[183] = ecc_bit[2]^ecc_bit[187]^tmp[2];
			CheckBits[184] = ecc_bit[3]^ecc_bit[188]^tmp[3];
			CheckBits[185] = ecc_bit[2]^ecc_bit[189]^tmp[2];
			CheckBits[186] = ecc_bit[0]^ecc_bit[3]^ecc_bit[190]^tmp[0]^tmp[3];
			CheckBits[187] = ecc_bit[0]^ecc_bit[1]^ecc_bit[2]^ecc_bit[191]^tmp[0]^tmp[1]^tmp[2];
			CheckBits[188] = ecc_bit[0]^ecc_bit[1]^ecc_bit[2]^ecc_bit[3]^ecc_bit[192]^tmp[0]^tmp[1]^tmp[2]^tmp[3];
			CheckBits[189] = ecc_bit[0]^ecc_bit[1]^ecc_bit[3]^ecc_bit[193]^tmp[0]^tmp[1]^tmp[3];
			CheckBits[190] = ecc_bit[0]^ecc_bit[1]^ecc_bit[194]^tmp[0]^tmp[1];
			CheckBits[191] = ecc_bit[0]^ecc_bit[1]^ecc_bit[2]^ecc_bit[195]^tmp[0]^tmp[1]^tmp[2];
			CheckBits[192] = ecc_bit[1]^ecc_bit[2]^ecc_bit[3]^ecc_bit[196]^tmp[1]^tmp[2]^tmp[3];
			CheckBits[193] = ecc_bit[0]^ecc_bit[3]^ecc_bit[197]^tmp[0]^tmp[3];
			CheckBits[194] = ecc_bit[1]^ecc_bit[2]^ecc_bit[198]^tmp[1]^tmp[2];
			CheckBits[195] = ecc_bit[2]^ecc_bit[3]^ecc_bit[199]^tmp[2]^tmp[3];
			CheckBits[196] = ecc_bit[0]^ecc_bit[2]^ecc_bit[3]^ecc_bit[200]^tmp[0]^tmp[2]^tmp[3];
			CheckBits[197] = ecc_bit[1]^ecc_bit[2]^ecc_bit[3]^ecc_bit[201]^tmp[1]^tmp[2]^tmp[3];
			CheckBits[198] = ecc_bit[3]^ecc_bit[202]^tmp[3];
			CheckBits[199] = ecc_bit[2]^ecc_bit[203]^tmp[2];
			CheckBits[200] = ecc_bit[0]^ecc_bit[3]^ecc_bit[204]^tmp[0]^tmp[3];
			CheckBits[201] = ecc_bit[1]^ecc_bit[2]^ecc_bit[205]^tmp[1]^tmp[2];
			CheckBits[202] = ecc_bit[0]^ecc_bit[2]^ecc_bit[3]^ecc_bit[206]^tmp[0]^tmp[2]^tmp[3];
			CheckBits[203] = ecc_bit[0]^ecc_bit[1]^ecc_bit[2]^ecc_bit[3]^ecc_bit[207]^tmp[0]^tmp[1]^tmp[2]^tmp[3];
			CheckBits[204] = ecc_bit[1]^ecc_bit[3]^ecc_bit[208]^tmp[1]^tmp[3];
			CheckBits[205] = ecc_bit[0]^ecc_bit[209]^tmp[0];
			CheckBits[206] = ecc_bit[1]^ecc_bit[210]^tmp[1];
			CheckBits[207] = ecc_bit[0]^ecc_bit[2]^ecc_bit[211]^tmp[0]^tmp[2];
			CheckBits[208] = ecc_bit[1]^ecc_bit[3]^ecc_bit[212]^tmp[1]^tmp[3];
			CheckBits[209] = ecc_bit[213];
			CheckBits[210] = ecc_bit[214];
			CheckBits[211] = ecc_bit[0]^ecc_bit[215]^tmp[0];
			CheckBits[212] = ecc_bit[0]^ecc_bit[1]^ecc_bit[216]^tmp[0]^tmp[1];
			CheckBits[213] = ecc_bit[0]^ecc_bit[1]^ecc_bit[2]^ecc_bit[217]^tmp[0]^tmp[1]^tmp[2];
			CheckBits[214] = ecc_bit[1]^ecc_bit[2]^ecc_bit[3]^ecc_bit[218]^tmp[1]^tmp[2]^tmp[3];
			CheckBits[215] = ecc_bit[3]^ecc_bit[219]^tmp[3];
			CheckBits[216] = ecc_bit[2]^ecc_bit[220]^tmp[2];
			CheckBits[217] = ecc_bit[3]^ecc_bit[221]^tmp[3];
			CheckBits[218] = ecc_bit[0]^ecc_bit[2]^ecc_bit[222]^tmp[0]^tmp[2];
			CheckBits[219] = ecc_bit[1]^ecc_bit[3]^ecc_bit[223]^tmp[1]^tmp[3];
			CheckBits[220] = ecc_bit[0]^ecc_bit[224]^tmp[0];
			CheckBits[221] = ecc_bit[0]^ecc_bit[1]^ecc_bit[225]^tmp[0]^tmp[1];
			CheckBits[222] = ecc_bit[0]^ecc_bit[1]^ecc_bit[2]^ecc_bit[226]^tmp[0]^tmp[1]^tmp[2];
			CheckBits[223] = ecc_bit[1]^ecc_bit[2]^ecc_bit[3]^ecc_bit[227]^tmp[1]^tmp[2]^tmp[3];
			CheckBits[224] = ecc_bit[0]^ecc_bit[3]^ecc_bit[228]^tmp[0]^tmp[3];
			CheckBits[225] = ecc_bit[0]^ecc_bit[1]^ecc_bit[2]^ecc_bit[229]^tmp[0]^tmp[1]^tmp[2];
			CheckBits[226] = ecc_bit[0]^ecc_bit[1]^ecc_bit[2]^ecc_bit[3]^ecc_bit[230]^tmp[0]^tmp[1]^tmp[2]^tmp[3];
			CheckBits[227] = ecc_bit[1]^ecc_bit[3]^ecc_bit[231]^tmp[1]^tmp[3];
			CheckBits[228] = ecc_bit[0]^ecc_bit[232]^tmp[0];
			CheckBits[229] = ecc_bit[1]^ecc_bit[233]^tmp[1];
			CheckBits[230] = ecc_bit[0]^ecc_bit[2]^ecc_bit[234]^tmp[0]^tmp[2];
			CheckBits[231] = ecc_bit[0]^ecc_bit[1]^ecc_bit[3]^ecc_bit[235]^tmp[0]^tmp[1]^tmp[3];
			CheckBits[232] = ecc_bit[0]^ecc_bit[1]^ecc_bit[236]^tmp[0]^tmp[1];
			CheckBits[233] = ecc_bit[1]^ecc_bit[2]^ecc_bit[237]^tmp[1]^tmp[2];
			CheckBits[234] = ecc_bit[2]^ecc_bit[3]^ecc_bit[238]^tmp[2]^tmp[3];
			CheckBits[235] = ecc_bit[0]^ecc_bit[2]^ecc_bit[3]^ecc_bit[239]^tmp[0]^tmp[2]^tmp[3];
			CheckBits[236] = ecc_bit[1]^ecc_bit[2]^ecc_bit[3]^ecc_bit[240]^tmp[1]^tmp[2]^tmp[3];
			CheckBits[237] = ecc_bit[3]^ecc_bit[241]^tmp[3];
			CheckBits[238] = ecc_bit[2]^ecc_bit[242]^tmp[2];
			CheckBits[239] = ecc_bit[3]^ecc_bit[243]^tmp[3];
			CheckBits[240] = ecc_bit[0]^ecc_bit[2]^ecc_bit[244]^tmp[0]^tmp[2];
			CheckBits[241] = ecc_bit[1]^ecc_bit[3]^ecc_bit[245]^tmp[1]^tmp[3];
			CheckBits[242] = ecc_bit[0]^ecc_bit[246]^tmp[0];
			CheckBits[243] = ecc_bit[1]^ecc_bit[247]^tmp[1];
			CheckBits[244] = ecc_bit[2]^ecc_bit[248]^tmp[2];
			CheckBits[245] = ecc_bit[3]^ecc_bit[249]^tmp[3];
			CheckBits[246] = ecc_bit[2]^ecc_bit[250]^tmp[2];
			CheckBits[247] = ecc_bit[0]^ecc_bit[3]^ecc_bit[251]^tmp[0]^tmp[3];
			CheckBits[248] = ecc_bit[1]^ecc_bit[2]^ecc_bit[252]^tmp[1]^tmp[2];
			CheckBits[249] = ecc_bit[0]^ecc_bit[2]^ecc_bit[3]^ecc_bit[253]^tmp[0]^tmp[2]^tmp[3];
			CheckBits[250] = ecc_bit[0]^ecc_bit[1]^ecc_bit[2]^ecc_bit[3]^ecc_bit[254]^tmp[0]^tmp[1]^tmp[2]^tmp[3];
			CheckBits[251] = ecc_bit[1]^ecc_bit[3]^ecc_bit[255]^tmp[1]^tmp[3];
			CheckBits[252] = ecc_bit[0]^ecc_bit[256]^tmp[0];
			CheckBits[253] = ecc_bit[0]^ecc_bit[1]^ecc_bit[257]^tmp[0]^tmp[1];
			CheckBits[254] = ecc_bit[0]^ecc_bit[1]^ecc_bit[2]^ecc_bit[258]^tmp[0]^tmp[1]^tmp[2];
			CheckBits[255] = ecc_bit[1]^ecc_bit[2]^ecc_bit[3]^ecc_bit[259]^tmp[1]^tmp[2]^tmp[3];
			CheckBits[256] = ecc_bit[0]^ecc_bit[3]^ecc_bit[260]^tmp[0]^tmp[3];
			CheckBits[257] = ecc_bit[0]^ecc_bit[1]^ecc_bit[2]^ecc_bit[261]^tmp[0]^tmp[1]^tmp[2];
			CheckBits[258] = ecc_bit[1]^ecc_bit[2]^ecc_bit[3]^ecc_bit[262]^tmp[1]^tmp[2]^tmp[3];
			CheckBits[259] = ecc_bit[0]^ecc_bit[3]^ecc_bit[263]^tmp[0]^tmp[3];
			CheckBits[260] = ecc_bit[1]^ecc_bit[2]^ecc_bit[264]^tmp[1]^tmp[2];
			CheckBits[261] = ecc_bit[2]^ecc_bit[3]^ecc_bit[265]^tmp[2]^tmp[3];
			CheckBits[262] = ecc_bit[2]^ecc_bit[3]^ecc_bit[266]^tmp[2]^tmp[3];
			CheckBits[263] = ecc_bit[0]^ecc_bit[2]^ecc_bit[3]^ecc_bit[267]^tmp[0]^tmp[2]^tmp[3];
			CheckBits[264] = ecc_bit[0]^ecc_bit[1]^ecc_bit[2]^ecc_bit[3]^ecc_bit[268]^tmp[0]^tmp[1]^tmp[2]^tmp[3];
			CheckBits[265] = ecc_bit[0]^ecc_bit[1]^ecc_bit[3]^ecc_bit[269]^tmp[0]^tmp[1]^tmp[3];
			CheckBits[266] = ecc_bit[0]^ecc_bit[1]^ecc_bit[270]^tmp[0]^tmp[1];
			CheckBits[267] = ecc_bit[1]^ecc_bit[2]^ecc_bit[271]^tmp[1]^tmp[2];
			CheckBits[268] = ecc_bit[0]^ecc_bit[2]^ecc_bit[3]^ecc_bit[272]^tmp[0]^tmp[2]^tmp[3];
			CheckBits[269] = ecc_bit[1]^ecc_bit[2]^ecc_bit[3]^ecc_bit[273]^tmp[1]^tmp[2]^tmp[3];
			CheckBits[270] = ecc_bit[0]^ecc_bit[3]^ecc_bit[274]^tmp[0]^tmp[3];
			CheckBits[271] = ecc_bit[1]^ecc_bit[2]^ecc_bit[275]^tmp[1]^tmp[2];
			CheckBits[272] = ecc_bit[2]^ecc_bit[3]^ecc_bit[276]^tmp[2]^tmp[3];
			CheckBits[273] = ecc_bit[2]^ecc_bit[3]^ecc_bit[277]^tmp[2]^tmp[3];
			CheckBits[274] = ecc_bit[0]^ecc_bit[2]^ecc_bit[3]^ecc_bit[278]^tmp[0]^tmp[2]^tmp[3];
			CheckBits[275] = ecc_bit[1]^ecc_bit[2]^ecc_bit[3]^ecc_bit[279]^tmp[1]^tmp[2]^tmp[3];
			CheckBits[276] = ecc_bit[0]^ecc_bit[3]^ecc_bit[280]^tmp[0]^tmp[3];
			CheckBits[277] = ecc_bit[1]^ecc_bit[2]^ecc_bit[281]^tmp[1]^tmp[2];
			CheckBits[278] = ecc_bit[2]^ecc_bit[3]^ecc_bit[282]^tmp[2]^tmp[3];
			CheckBits[279] = ecc_bit[0]^ecc_bit[2]^ecc_bit[3]^ecc_bit[283]^tmp[0]^tmp[2]^tmp[3];
			CheckBits[280] = ecc_bit[0]^ecc_bit[1]^ecc_bit[2]^ecc_bit[3]^ecc_bit[284]^tmp[0]^tmp[1]^tmp[2]^tmp[3];
			CheckBits[281] = ecc_bit[0]^ecc_bit[1]^ecc_bit[3]^ecc_bit[285]^tmp[0]^tmp[1]^tmp[3];
			CheckBits[282] = ecc_bit[0]^ecc_bit[1]^ecc_bit[286]^tmp[0]^tmp[1];
			CheckBits[283] = ecc_bit[0]^ecc_bit[1]^ecc_bit[2]^ecc_bit[287]^tmp[0]^tmp[1]^tmp[2];
			CheckBits[284] = ecc_bit[0]^ecc_bit[1]^ecc_bit[2]^ecc_bit[3]^ecc_bit[288]^tmp[0]^tmp[1]^tmp[2]^tmp[3];
			CheckBits[285] = ecc_bit[1]^ecc_bit[3]^ecc_bit[289]^tmp[1]^tmp[3];
			CheckBits[286] = ecc_bit[0]^ecc_bit[290]^tmp[0];
			CheckBits[287] = ecc_bit[1]^ecc_bit[291]^tmp[1];
			CheckBits[288] = ecc_bit[2]^ecc_bit[292]^tmp[2];
			CheckBits[289] = ecc_bit[0]^ecc_bit[3]^ecc_bit[293]^tmp[0]^tmp[3];
			CheckBits[290] = ecc_bit[0]^ecc_bit[1]^ecc_bit[2]^ecc_bit[294]^tmp[0]^tmp[1]^tmp[2];
			CheckBits[291] = ecc_bit[1]^ecc_bit[2]^ecc_bit[3]^ecc_bit[295]^tmp[1]^tmp[2]^tmp[3];
			CheckBits[292] = ecc_bit[3]^ecc_bit[296]^tmp[3];
			CheckBits[293] = ecc_bit[0]^ecc_bit[2]^ecc_bit[297]^tmp[0]^tmp[2];
			CheckBits[294] = ecc_bit[1]^ecc_bit[3]^ecc_bit[298]^tmp[1]^tmp[3];
			CheckBits[295] = ecc_bit[0]^ecc_bit[299]^tmp[0];
			CheckBits[296] = ecc_bit[0]^ecc_bit[1]^ecc_bit[300]^tmp[0]^tmp[1];
			CheckBits[297] = ecc_bit[1]^ecc_bit[2]^ecc_bit[301]^tmp[1]^tmp[2];
			CheckBits[298] = ecc_bit[2]^ecc_bit[3]^ecc_bit[302]^tmp[2]^tmp[3];
			CheckBits[299] = ecc_bit[2]^ecc_bit[3]^ecc_bit[303]^tmp[2]^tmp[3];
			CheckBits[300] = ecc_bit[0]^ecc_bit[2]^ecc_bit[3]^ecc_bit[304]^tmp[0]^tmp[2]^tmp[3];
			CheckBits[301] = ecc_bit[1]^ecc_bit[2]^ecc_bit[3]^ecc_bit[305]^tmp[1]^tmp[2]^tmp[3];
			CheckBits[302] = ecc_bit[0]^ecc_bit[3]^ecc_bit[306]^tmp[0]^tmp[3];
			CheckBits[303] = ecc_bit[1]^ecc_bit[2]^ecc_bit[307]^tmp[1]^tmp[2];
			CheckBits[304] = ecc_bit[2]^ecc_bit[3]^ecc_bit[308]^tmp[2]^tmp[3];
			CheckBits[305] = ecc_bit[2]^ecc_bit[3]^ecc_bit[309]^tmp[2]^tmp[3];
			CheckBits[306] = ecc_bit[0]^ecc_bit[2]^ecc_bit[3]^ecc_bit[310]^tmp[0]^tmp[2]^tmp[3];
			CheckBits[307] = ecc_bit[1]^ecc_bit[2]^ecc_bit[3]^ecc_bit[311]^tmp[1]^tmp[2]^tmp[3];
			CheckBits[308] = ecc_bit[0]^ecc_bit[3]^ecc_bit[312]^tmp[0]^tmp[3];
			CheckBits[309] = ecc_bit[0]^ecc_bit[1]^ecc_bit[2]^ecc_bit[313]^tmp[0]^tmp[1]^tmp[2];
			CheckBits[310] = ecc_bit[0]^ecc_bit[1]^ecc_bit[2]^ecc_bit[3]^ecc_bit[314]^tmp[0]^tmp[1]^tmp[2]^tmp[3];
			CheckBits[311] = ecc_bit[0]^ecc_bit[1]^ecc_bit[3]^ecc_bit[315]^tmp[0]^tmp[1]^tmp[3];
			CheckBits[312] = ecc_bit[1]^ecc_bit[316]^tmp[1];
			CheckBits[313] = ecc_bit[0]^ecc_bit[2]^ecc_bit[317]^tmp[0]^tmp[2];
			CheckBits[314] = ecc_bit[0]^ecc_bit[1]^ecc_bit[3]^ecc_bit[318]^tmp[0]^tmp[1]^tmp[3];
			CheckBits[315] = ecc_bit[1]^ecc_bit[319]^tmp[1];
			CheckBits[316] = ecc_bit[2]^ecc_bit[320]^tmp[2];
			CheckBits[317] = ecc_bit[3]^ecc_bit[321]^tmp[3];
			CheckBits[318] = ecc_bit[0]^ecc_bit[2]^ecc_bit[322]^tmp[0]^tmp[2];
			CheckBits[319] = ecc_bit[0]^ecc_bit[1]^ecc_bit[3]^ecc_bit[323]^tmp[0]^tmp[1]^tmp[3];
			CheckBits[320] = ecc_bit[0]^ecc_bit[1]^ecc_bit[324]^tmp[0]^tmp[1];
			CheckBits[321] = ecc_bit[1]^ecc_bit[2]^ecc_bit[325]^tmp[1]^tmp[2];
			CheckBits[322] = ecc_bit[0]^ecc_bit[2]^ecc_bit[3]^ecc_bit[326]^tmp[0]^tmp[2]^tmp[3];
			CheckBits[323] = ecc_bit[0]^ecc_bit[1]^ecc_bit[2]^ecc_bit[3]^ecc_bit[327]^tmp[0]^tmp[1]^tmp[2]^tmp[3];
			CheckBits[324] = ecc_bit[0]^ecc_bit[1]^ecc_bit[3]^ecc_bit[328]^tmp[0]^tmp[1]^tmp[3];
			CheckBits[325] = ecc_bit[0]^ecc_bit[1]^ecc_bit[329]^tmp[0]^tmp[1];
			CheckBits[326] = ecc_bit[0]^ecc_bit[1]^ecc_bit[2]^ecc_bit[330]^tmp[0]^tmp[1]^tmp[2];
			CheckBits[327] = ecc_bit[1]^ecc_bit[2]^ecc_bit[3]^ecc_bit[331]^tmp[1]^tmp[2]^tmp[3];
			CheckBits[328] = ecc_bit[3]^ecc_bit[332]^tmp[3];
			CheckBits[329] = ecc_bit[0]^ecc_bit[2]^ecc_bit[333]^tmp[0]^tmp[2];
			CheckBits[330] = ecc_bit[0]^ecc_bit[1]^ecc_bit[3]^ecc_bit[334]^tmp[0]^tmp[1]^tmp[3];
			CheckBits[331] = ecc_bit[0]^ecc_bit[1]^ecc_bit[335]^tmp[0]^tmp[1];
			CheckBits[332] = ecc_bit[1]^ecc_bit[2]^ecc_bit[336]^tmp[1]^tmp[2];
			CheckBits[333] = ecc_bit[2]^ecc_bit[3]^ecc_bit[337]^tmp[2]^tmp[3];
			CheckBits[334] = ecc_bit[2]^ecc_bit[3]^ecc_bit[338]^tmp[2]^tmp[3];
			CheckBits[335] = ecc_bit[2]^ecc_bit[3]^ecc_bit[339]^tmp[2]^tmp[3];
			CheckBits[336] = ecc_bit[0]^ecc_bit[2]^ecc_bit[3]^ecc_bit[340]^tmp[0]^tmp[2]^tmp[3];
			CheckBits[337] = ecc_bit[0]^ecc_bit[1]^ecc_bit[2]^ecc_bit[3]^ecc_bit[341]^tmp[0]^tmp[1]^tmp[2]^tmp[3];
			CheckBits[338] = ecc_bit[0]^ecc_bit[1]^ecc_bit[3]^ecc_bit[342]^tmp[0]^tmp[1]^tmp[3];
			CheckBits[339] = ecc_bit[0]^ecc_bit[1]^ecc_bit[343]^tmp[0]^tmp[1];
			CheckBits[340] = ecc_bit[1]^ecc_bit[2]^ecc_bit[344]^tmp[1]^tmp[2];
			CheckBits[341] = ecc_bit[0]^ecc_bit[2]^ecc_bit[3]^ecc_bit[345]^tmp[0]^tmp[2]^tmp[3];
			CheckBits[342] = ecc_bit[0]^ecc_bit[1]^ecc_bit[2]^ecc_bit[3]^ecc_bit[346]^tmp[0]^tmp[1]^tmp[2]^tmp[3];
			CheckBits[343] = ecc_bit[1]^ecc_bit[3]^ecc_bit[347]^tmp[1]^tmp[3];
			CheckBits[344] = ecc_bit[0]^ecc_bit[348]^tmp[0];
			CheckBits[345] = ecc_bit[1]^ecc_bit[349]^tmp[1];
			CheckBits[346] = ecc_bit[2]^ecc_bit[350]^tmp[2];
			CheckBits[347] = ecc_bit[0]^ecc_bit[3]^ecc_bit[351]^tmp[0]^tmp[3];
			CheckBits[348] = ecc_bit[0]^ecc_bit[1]^ecc_bit[2]^ecc_bit[352]^tmp[0]^tmp[1]^tmp[2];
			CheckBits[349] = ecc_bit[0]^ecc_bit[1]^ecc_bit[2]^ecc_bit[3]^ecc_bit[353]^tmp[0]^tmp[1]^tmp[2]^tmp[3];
			CheckBits[350] = ecc_bit[1]^ecc_bit[3]^ecc_bit[354]^tmp[1]^tmp[3];
			CheckBits[351] = ecc_bit[0]^ecc_bit[355]^tmp[0];
			CheckBits[352] = ecc_bit[0]^ecc_bit[1]^ecc_bit[356]^tmp[0]^tmp[1];
			CheckBits[353] = ecc_bit[0]^ecc_bit[1]^ecc_bit[2]^ecc_bit[357]^tmp[0]^tmp[1]^tmp[2];
			CheckBits[354] = ecc_bit[1]^ecc_bit[2]^ecc_bit[3]^ecc_bit[358]^tmp[1]^tmp[2]^tmp[3];
			CheckBits[355] = ecc_bit[3]^ecc_bit[359]^tmp[3];
			CheckBits[356] = ecc_bit[2]^ecc_bit[360]^tmp[2];
			CheckBits[357] = ecc_bit[3]^ecc_bit[361]^tmp[3];
			CheckBits[358] = ecc_bit[0]^ecc_bit[2]^ecc_bit[362]^tmp[0]^tmp[2];
			CheckBits[359] = ecc_bit[0]^ecc_bit[1]^ecc_bit[3]^ecc_bit[363]^tmp[0]^tmp[1]^tmp[3];
			CheckBits[360] = ecc_bit[0]^ecc_bit[1]^tmp[0]^tmp[1];
			CheckBits[361] = ecc_bit[0]^ecc_bit[1]^ecc_bit[2]^tmp[0]^tmp[1]^tmp[2];
			CheckBits[362] = ecc_bit[0]^ecc_bit[1]^ecc_bit[2]^ecc_bit[3]^tmp[0]^tmp[1]^tmp[2]^tmp[3];
			CheckBits[363] = ecc_bit[1]^ecc_bit[3]^tmp[1]^tmp[3];
		} else if(ecc_size == 28) {
			CheckBits[0] = ecc_bit[0]^ecc_bit[1]^ecc_bit[3]^ecc_bit[4]^tmp[0]^tmp[1]^tmp[3];
			CheckBits[1] = ecc_bit[2]^ecc_bit[3]^ecc_bit[5]^tmp[2]^tmp[3];
			CheckBits[2] = ecc_bit[1]^ecc_bit[6]^tmp[1];
			CheckBits[3] = ecc_bit[0]^ecc_bit[2]^ecc_bit[7]^tmp[0]^tmp[2];
			CheckBits[4] = ecc_bit[0]^ecc_bit[1]^ecc_bit[3]^ecc_bit[8]^tmp[0]^tmp[1]^tmp[3];
			CheckBits[5] = ecc_bit[0]^ecc_bit[2]^ecc_bit[3]^ecc_bit[9]^tmp[0]^tmp[2]^tmp[3];
			CheckBits[6] = ecc_bit[10];
			CheckBits[7] = ecc_bit[0]^ecc_bit[11]^tmp[0];
			CheckBits[8] = ecc_bit[0]^ecc_bit[1]^ecc_bit[12]^tmp[0]^tmp[1];
			CheckBits[9] = ecc_bit[1]^ecc_bit[2]^ecc_bit[13]^tmp[1]^tmp[2];
			CheckBits[10] = ecc_bit[2]^ecc_bit[3]^ecc_bit[14]^tmp[2]^tmp[3];
			CheckBits[11] = ecc_bit[0]^ecc_bit[1]^ecc_bit[15]^tmp[0]^tmp[1];
			CheckBits[12] = ecc_bit[1]^ecc_bit[2]^ecc_bit[16]^tmp[1]^tmp[2];
			CheckBits[13] = ecc_bit[0]^ecc_bit[2]^ecc_bit[3]^ecc_bit[17]^tmp[0]^tmp[2]^tmp[3];
			CheckBits[14] = ecc_bit[18];
			CheckBits[15] = ecc_bit[19];
			CheckBits[16] = ecc_bit[20];
			CheckBits[17] = ecc_bit[0]^ecc_bit[21]^tmp[0];
			CheckBits[18] = ecc_bit[1]^ecc_bit[22]^tmp[1];
			CheckBits[19] = ecc_bit[0]^ecc_bit[2]^ecc_bit[23]^tmp[0]^tmp[2];
			CheckBits[20] = ecc_bit[0]^ecc_bit[1]^ecc_bit[3]^ecc_bit[24]^tmp[0]^tmp[1]^tmp[3];
			CheckBits[21] = ecc_bit[0]^ecc_bit[2]^ecc_bit[3]^ecc_bit[25]^tmp[0]^tmp[2]^tmp[3];
			CheckBits[22] = ecc_bit[0]^ecc_bit[26]^tmp[0];
			CheckBits[23] = ecc_bit[1]^ecc_bit[27]^tmp[1];
			CheckBits[24] = ecc_bit[2]^ecc_bit[28]^tmp[2];
			CheckBits[25] = ecc_bit[3]^ecc_bit[29]^tmp[3];
			CheckBits[26] = ecc_bit[0]^ecc_bit[1]^ecc_bit[3]^ecc_bit[30]^tmp[0]^tmp[1]^tmp[3];
			CheckBits[27] = ecc_bit[0]^ecc_bit[2]^ecc_bit[3]^ecc_bit[31]^tmp[0]^tmp[2]^tmp[3];
			CheckBits[28] = ecc_bit[32];
			CheckBits[29] = ecc_bit[33];
			CheckBits[30] = ecc_bit[0]^ecc_bit[34]^tmp[0];
			CheckBits[31] = ecc_bit[0]^ecc_bit[1]^ecc_bit[35]^tmp[0]^tmp[1];
			CheckBits[32] = ecc_bit[1]^ecc_bit[2]^ecc_bit[36]^tmp[1]^tmp[2];
			CheckBits[33] = ecc_bit[2]^ecc_bit[3]^ecc_bit[37]^tmp[2]^tmp[3];
			CheckBits[34] = ecc_bit[0]^ecc_bit[1]^ecc_bit[38]^tmp[0]^tmp[1];
			CheckBits[35] = ecc_bit[0]^ecc_bit[1]^ecc_bit[2]^ecc_bit[39]^tmp[0]^tmp[1]^tmp[2];
			CheckBits[36] = ecc_bit[1]^ecc_bit[2]^ecc_bit[3]^ecc_bit[40]^tmp[1]^tmp[2]^tmp[3];
			CheckBits[37] = ecc_bit[0]^ecc_bit[1]^ecc_bit[2]^ecc_bit[41]^tmp[0]^tmp[1]^tmp[2];
			CheckBits[38] = ecc_bit[1]^ecc_bit[2]^ecc_bit[3]^ecc_bit[42]^tmp[1]^tmp[2]^tmp[3];
			CheckBits[39] = ecc_bit[0]^ecc_bit[1]^ecc_bit[2]^ecc_bit[43]^tmp[0]^tmp[1]^tmp[2];
			CheckBits[40] = ecc_bit[1]^ecc_bit[2]^ecc_bit[3]^ecc_bit[44]^tmp[1]^tmp[2]^tmp[3];
			CheckBits[41] = ecc_bit[1]^ecc_bit[2]^ecc_bit[45]^tmp[1]^tmp[2];
			CheckBits[42] = ecc_bit[0]^ecc_bit[2]^ecc_bit[3]^ecc_bit[46]^tmp[0]^tmp[2]^tmp[3];
			CheckBits[43] = ecc_bit[0]^ecc_bit[47]^tmp[0];
			CheckBits[44] = ecc_bit[1]^ecc_bit[48]^tmp[1];
			CheckBits[45] = ecc_bit[0]^ecc_bit[2]^ecc_bit[49]^tmp[0]^tmp[2];
			CheckBits[46] = ecc_bit[1]^ecc_bit[3]^ecc_bit[50]^tmp[1]^tmp[3];
			CheckBits[47] = ecc_bit[1]^ecc_bit[2]^ecc_bit[3]^ecc_bit[51]^tmp[1]^tmp[2]^tmp[3];
			CheckBits[48] = ecc_bit[1]^ecc_bit[2]^ecc_bit[52]^tmp[1]^tmp[2];
			CheckBits[49] = ecc_bit[2]^ecc_bit[3]^ecc_bit[53]^tmp[2]^tmp[3];
			CheckBits[50] = ecc_bit[0]^ecc_bit[1]^ecc_bit[54]^tmp[0]^tmp[1];
			CheckBits[51] = ecc_bit[1]^ecc_bit[2]^ecc_bit[55]^tmp[1]^tmp[2];
			CheckBits[52] = ecc_bit[2]^ecc_bit[3]^ecc_bit[56]^tmp[2]^tmp[3];
			CheckBits[53] = ecc_bit[0]^ecc_bit[1]^ecc_bit[57]^tmp[0]^tmp[1];
			CheckBits[54] = ecc_bit[1]^ecc_bit[2]^ecc_bit[58]^tmp[1]^tmp[2];
			CheckBits[55] = ecc_bit[0]^ecc_bit[2]^ecc_bit[3]^ecc_bit[59]^tmp[0]^tmp[2]^tmp[3];
			CheckBits[56] = ecc_bit[0]^ecc_bit[60]^tmp[0];
			CheckBits[57] = ecc_bit[0]^ecc_bit[1]^ecc_bit[61]^tmp[0]^tmp[1];
			CheckBits[58] = ecc_bit[0]^ecc_bit[1]^ecc_bit[2]^ecc_bit[62]^tmp[0]^tmp[1]^tmp[2];
			CheckBits[59] = ecc_bit[1]^ecc_bit[2]^ecc_bit[3]^ecc_bit[63]^tmp[1]^tmp[2]^tmp[3];
			CheckBits[60] = ecc_bit[1]^ecc_bit[2]^ecc_bit[64]^tmp[1]^tmp[2];
			CheckBits[61] = ecc_bit[2]^ecc_bit[3]^ecc_bit[65]^tmp[2]^tmp[3];
			CheckBits[62] = ecc_bit[1]^ecc_bit[66]^tmp[1];
			CheckBits[63] = ecc_bit[0]^ecc_bit[2]^ecc_bit[67]^tmp[0]^tmp[2];
			CheckBits[64] = ecc_bit[0]^ecc_bit[1]^ecc_bit[3]^ecc_bit[68]^tmp[0]^tmp[1]^tmp[3];
			CheckBits[65] = ecc_bit[0]^ecc_bit[2]^ecc_bit[3]^ecc_bit[69]^tmp[0]^tmp[2]^tmp[3];
			CheckBits[66] = ecc_bit[70];
			CheckBits[67] = ecc_bit[0]^ecc_bit[71]^tmp[0];
			CheckBits[68] = ecc_bit[0]^ecc_bit[1]^ecc_bit[72]^tmp[0]^tmp[1];
			CheckBits[69] = ecc_bit[0]^ecc_bit[1]^ecc_bit[2]^ecc_bit[73]^tmp[0]^tmp[1]^tmp[2];
			CheckBits[70] = ecc_bit[1]^ecc_bit[2]^ecc_bit[3]^ecc_bit[74]^tmp[1]^tmp[2]^tmp[3];
			CheckBits[71] = ecc_bit[0]^ecc_bit[1]^ecc_bit[2]^ecc_bit[75]^tmp[0]^tmp[1]^tmp[2];
			CheckBits[72] = ecc_bit[1]^ecc_bit[2]^ecc_bit[3]^ecc_bit[76]^tmp[1]^tmp[2]^tmp[3];
			CheckBits[73] = ecc_bit[1]^ecc_bit[2]^ecc_bit[77]^tmp[1]^tmp[2];
			CheckBits[74] = ecc_bit[0]^ecc_bit[2]^ecc_bit[3]^ecc_bit[78]^tmp[0]^tmp[2]^tmp[3];
			CheckBits[75] = ecc_bit[0]^ecc_bit[79]^tmp[0];
			CheckBits[76] = ecc_bit[0]^ecc_bit[1]^ecc_bit[80]^tmp[0]^tmp[1];
			CheckBits[77] = ecc_bit[0]^ecc_bit[1]^ecc_bit[2]^ecc_bit[81]^tmp[0]^tmp[1]^tmp[2];
			CheckBits[78] = ecc_bit[1]^ecc_bit[2]^ecc_bit[3]^ecc_bit[82]^tmp[1]^tmp[2]^tmp[3];
			CheckBits[79] = ecc_bit[1]^ecc_bit[2]^ecc_bit[83]^tmp[1]^tmp[2];
			CheckBits[80] = ecc_bit[2]^ecc_bit[3]^ecc_bit[84]^tmp[2]^tmp[3];
			CheckBits[81] = ecc_bit[0]^ecc_bit[1]^ecc_bit[85]^tmp[0]^tmp[1];
			CheckBits[82] = ecc_bit[1]^ecc_bit[2]^ecc_bit[86]^tmp[1]^tmp[2];
			CheckBits[83] = ecc_bit[0]^ecc_bit[2]^ecc_bit[3]^ecc_bit[87]^tmp[0]^tmp[2]^tmp[3];
			CheckBits[84] = ecc_bit[0]^ecc_bit[88]^tmp[0];
			CheckBits[85] = ecc_bit[0]^ecc_bit[1]^ecc_bit[89]^tmp[0]^tmp[1];
			CheckBits[86] = ecc_bit[1]^ecc_bit[2]^ecc_bit[90]^tmp[1]^tmp[2];
			CheckBits[87] = ecc_bit[2]^ecc_bit[3]^ecc_bit[91]^tmp[2]^tmp[3];
			CheckBits[88] = ecc_bit[0]^ecc_bit[1]^ecc_bit[92]^tmp[0]^tmp[1];
			CheckBits[89] = ecc_bit[1]^ecc_bit[2]^ecc_bit[93]^tmp[1]^tmp[2];
			CheckBits[90] = ecc_bit[0]^ecc_bit[2]^ecc_bit[3]^ecc_bit[94]^tmp[0]^tmp[2]^tmp[3];
			CheckBits[91] = ecc_bit[95];
			CheckBits[92] = ecc_bit[96];
			CheckBits[93] = ecc_bit[97];
			CheckBits[94] = ecc_bit[0]^ecc_bit[98]^tmp[0];
			CheckBits[95] = ecc_bit[0]^ecc_bit[1]^ecc_bit[99]^tmp[0]^tmp[1];
			CheckBits[96] = ecc_bit[0]^ecc_bit[1]^ecc_bit[2]^ecc_bit[100]^tmp[0]^tmp[1]^tmp[2];
			CheckBits[97] = ecc_bit[0]^ecc_bit[1]^ecc_bit[2]^ecc_bit[3]^ecc_bit[101]^tmp[0]^tmp[1]^tmp[2]^tmp[3];
			CheckBits[98] = ecc_bit[0]^ecc_bit[2]^ecc_bit[102]^tmp[0]^tmp[2];
			CheckBits[99] = ecc_bit[0]^ecc_bit[1]^ecc_bit[3]^ecc_bit[103]^tmp[0]^tmp[1]^tmp[3];
			CheckBits[100] = ecc_bit[2]^ecc_bit[3]^ecc_bit[104]^tmp[2]^tmp[3];
			CheckBits[101] = ecc_bit[0]^ecc_bit[1]^ecc_bit[105]^tmp[0]^tmp[1];
			CheckBits[102] = ecc_bit[0]^ecc_bit[1]^ecc_bit[2]^ecc_bit[106]^tmp[0]^tmp[1]^tmp[2];
			CheckBits[103] = ecc_bit[1]^ecc_bit[2]^ecc_bit[3]^ecc_bit[107]^tmp[1]^tmp[2]^tmp[3];
			CheckBits[104] = ecc_bit[1]^ecc_bit[2]^ecc_bit[108]^tmp[1]^tmp[2];
			CheckBits[105] = ecc_bit[2]^ecc_bit[3]^ecc_bit[109]^tmp[2]^tmp[3];
			CheckBits[106] = ecc_bit[1]^ecc_bit[110]^tmp[1];
			CheckBits[107] = ecc_bit[2]^ecc_bit[111]^tmp[2];
			CheckBits[108] = ecc_bit[0]^ecc_bit[3]^ecc_bit[112]^tmp[0]^tmp[3];
			CheckBits[109] = ecc_bit[3]^ecc_bit[113]^tmp[3];
			CheckBits[110] = ecc_bit[1]^ecc_bit[3]^ecc_bit[114]^tmp[1]^tmp[3];
			CheckBits[111] = ecc_bit[0]^ecc_bit[1]^ecc_bit[2]^ecc_bit[3]^ecc_bit[115]^tmp[0]^tmp[1]^tmp[2]^tmp[3];
			CheckBits[112] = ecc_bit[2]^ecc_bit[116]^tmp[2];
			CheckBits[113] = ecc_bit[0]^ecc_bit[3]^ecc_bit[117]^tmp[0]^tmp[3];
			CheckBits[114] = ecc_bit[0]^ecc_bit[3]^ecc_bit[118]^tmp[0]^tmp[3];
			CheckBits[115] = ecc_bit[0]^ecc_bit[3]^ecc_bit[119]^tmp[0]^tmp[3];
			CheckBits[116] = ecc_bit[0]^ecc_bit[3]^ecc_bit[120]^tmp[0]^tmp[3];
			CheckBits[117] = ecc_bit[0]^ecc_bit[3]^ecc_bit[121]^tmp[0]^tmp[3];
			CheckBits[118] = ecc_bit[0]^ecc_bit[3]^ecc_bit[122]^tmp[0]^tmp[3];
			CheckBits[119] = ecc_bit[3]^ecc_bit[123]^tmp[3];
			CheckBits[120] = ecc_bit[1]^ecc_bit[3]^ecc_bit[124]^tmp[1]^tmp[3];
			CheckBits[121] = ecc_bit[1]^ecc_bit[2]^ecc_bit[3]^ecc_bit[125]^tmp[1]^tmp[2]^tmp[3];
			CheckBits[122] = ecc_bit[0]^ecc_bit[1]^ecc_bit[2]^ecc_bit[126]^tmp[0]^tmp[1]^tmp[2];
			CheckBits[123] = ecc_bit[0]^ecc_bit[1]^ecc_bit[2]^ecc_bit[3]^ecc_bit[127]^tmp[0]^tmp[1]^tmp[2]^tmp[3];
			CheckBits[124] = ecc_bit[2]^ecc_bit[128]^tmp[2];
			CheckBits[125] = ecc_bit[0]^ecc_bit[3]^ecc_bit[129]^tmp[0]^tmp[3];
			CheckBits[126] = ecc_bit[0]^ecc_bit[3]^ecc_bit[130]^tmp[0]^tmp[3];
			CheckBits[127] = ecc_bit[0]^ecc_bit[3]^ecc_bit[131]^tmp[0]^tmp[3];
			CheckBits[128] = ecc_bit[3]^ecc_bit[132]^tmp[3];
			CheckBits[129] = ecc_bit[0]^ecc_bit[1]^ecc_bit[3]^ecc_bit[133]^tmp[0]^tmp[1]^tmp[3];
			CheckBits[130] = ecc_bit[0]^ecc_bit[2]^ecc_bit[3]^ecc_bit[134]^tmp[0]^tmp[2]^tmp[3];
			CheckBits[131] = ecc_bit[0]^ecc_bit[135]^tmp[0];
			CheckBits[132] = ecc_bit[1]^ecc_bit[136]^tmp[1];
			CheckBits[133] = ecc_bit[2]^ecc_bit[137]^tmp[2];
			CheckBits[134] = ecc_bit[0]^ecc_bit[3]^ecc_bit[138]^tmp[0]^tmp[3];
			CheckBits[135] = ecc_bit[3]^ecc_bit[139]^tmp[3];
			CheckBits[136] = ecc_bit[0]^ecc_bit[1]^ecc_bit[3]^ecc_bit[140]^tmp[0]^tmp[1]^tmp[3];
			CheckBits[137] = ecc_bit[0]^ecc_bit[2]^ecc_bit[3]^ecc_bit[141]^tmp[0]^tmp[2]^tmp[3];
			CheckBits[138] = ecc_bit[142];
			CheckBits[139] = ecc_bit[0]^ecc_bit[143]^tmp[0];
			CheckBits[140] = ecc_bit[1]^ecc_bit[144]^tmp[1];
			CheckBits[141] = ecc_bit[2]^ecc_bit[145]^tmp[2];
			CheckBits[142] = ecc_bit[0]^ecc_bit[3]^ecc_bit[146]^tmp[0]^tmp[3];
			CheckBits[143] = ecc_bit[3]^ecc_bit[147]^tmp[3];
			CheckBits[144] = ecc_bit[1]^ecc_bit[3]^ecc_bit[148]^tmp[1]^tmp[3];
			CheckBits[145] = ecc_bit[1]^ecc_bit[2]^ecc_bit[3]^ecc_bit[149]^tmp[1]^tmp[2]^tmp[3];
			CheckBits[146] = ecc_bit[0]^ecc_bit[1]^ecc_bit[2]^ecc_bit[150]^tmp[0]^tmp[1]^tmp[2];
			CheckBits[147] = ecc_bit[1]^ecc_bit[2]^ecc_bit[3]^ecc_bit[151]^tmp[1]^tmp[2]^tmp[3];
			CheckBits[148] = ecc_bit[0]^ecc_bit[1]^ecc_bit[2]^ecc_bit[152]^tmp[0]^tmp[1]^tmp[2];
			CheckBits[149] = ecc_bit[0]^ecc_bit[1]^ecc_bit[2]^ecc_bit[3]^ecc_bit[153]^tmp[0]^tmp[1]^tmp[2]^tmp[3];
			CheckBits[150] = ecc_bit[0]^ecc_bit[2]^ecc_bit[154]^tmp[0]^tmp[2];
			CheckBits[151] = ecc_bit[0]^ecc_bit[1]^ecc_bit[3]^ecc_bit[155]^tmp[0]^tmp[1]^tmp[3];
			CheckBits[152] = ecc_bit[2]^ecc_bit[3]^ecc_bit[156]^tmp[2]^tmp[3];
			CheckBits[153] = ecc_bit[1]^ecc_bit[157]^tmp[1];
			CheckBits[154] = ecc_bit[0]^ecc_bit[2]^ecc_bit[158]^tmp[0]^tmp[2];
			CheckBits[155] = ecc_bit[0]^ecc_bit[1]^ecc_bit[3]^ecc_bit[159]^tmp[0]^tmp[1]^tmp[3];
			CheckBits[156] = ecc_bit[0]^ecc_bit[2]^ecc_bit[3]^ecc_bit[160]^tmp[0]^tmp[2]^tmp[3];
			CheckBits[157] = ecc_bit[161];
			CheckBits[158] = ecc_bit[0]^ecc_bit[162]^tmp[0];
			CheckBits[159] = ecc_bit[0]^ecc_bit[1]^ecc_bit[163]^tmp[0]^tmp[1];
			CheckBits[160] = ecc_bit[1]^ecc_bit[2]^ecc_bit[164]^tmp[1]^tmp[2];
			CheckBits[161] = ecc_bit[0]^ecc_bit[2]^ecc_bit[3]^ecc_bit[165]^tmp[0]^tmp[2]^tmp[3];
			CheckBits[162] = ecc_bit[0]^ecc_bit[166]^tmp[0];
			CheckBits[163] = ecc_bit[0]^ecc_bit[1]^ecc_bit[167]^tmp[0]^tmp[1];
			CheckBits[164] = ecc_bit[1]^ecc_bit[2]^ecc_bit[168]^tmp[1]^tmp[2];
			CheckBits[165] = ecc_bit[2]^ecc_bit[3]^ecc_bit[169]^tmp[2]^tmp[3];
			CheckBits[166] = ecc_bit[1]^ecc_bit[170]^tmp[1];
			CheckBits[167] = ecc_bit[2]^ecc_bit[171]^tmp[2];
			CheckBits[168] = ecc_bit[3]^ecc_bit[172]^tmp[3];
			CheckBits[169] = ecc_bit[1]^ecc_bit[3]^ecc_bit[173]^tmp[1]^tmp[3];
			CheckBits[170] = ecc_bit[1]^ecc_bit[2]^ecc_bit[3]^ecc_bit[174]^tmp[1]^tmp[2]^tmp[3];
			CheckBits[171] = ecc_bit[1]^ecc_bit[2]^ecc_bit[175]^tmp[1]^tmp[2];
			CheckBits[172] = ecc_bit[2]^ecc_bit[3]^ecc_bit[176]^tmp[2]^tmp[3];
			CheckBits[173] = ecc_bit[1]^ecc_bit[177]^tmp[1];
			CheckBits[174] = ecc_bit[2]^ecc_bit[178]^tmp[2];
			CheckBits[175] = ecc_bit[0]^ecc_bit[3]^ecc_bit[179]^tmp[0]^tmp[3];
			CheckBits[176] = ecc_bit[3]^ecc_bit[180]^tmp[3];
			CheckBits[177] = ecc_bit[1]^ecc_bit[3]^ecc_bit[181]^tmp[1]^tmp[3];
			CheckBits[178] = ecc_bit[0]^ecc_bit[1]^ecc_bit[2]^ecc_bit[3]^ecc_bit[182]^tmp[0]^tmp[1]^tmp[2]^tmp[3];
			CheckBits[179] = ecc_bit[0]^ecc_bit[2]^ecc_bit[183]^tmp[0]^tmp[2];
			CheckBits[180] = ecc_bit[1]^ecc_bit[3]^ecc_bit[184]^tmp[1]^tmp[3];
			CheckBits[181] = ecc_bit[0]^ecc_bit[1]^ecc_bit[2]^ecc_bit[3]^ecc_bit[185]^tmp[0]^tmp[1]^tmp[2]^tmp[3];
			CheckBits[182] = ecc_bit[0]^ecc_bit[2]^ecc_bit[186]^tmp[0]^tmp[2];
			CheckBits[183] = ecc_bit[0]^ecc_bit[1]^ecc_bit[3]^ecc_bit[187]^tmp[0]^tmp[1]^tmp[3];
			CheckBits[184] = ecc_bit[0]^ecc_bit[2]^ecc_bit[3]^ecc_bit[188]^tmp[0]^tmp[2]^tmp[3];
			CheckBits[185] = ecc_bit[0]^ecc_bit[189]^tmp[0];
			CheckBits[186] = ecc_bit[0]^ecc_bit[1]^ecc_bit[190]^tmp[0]^tmp[1];
			CheckBits[187] = ecc_bit[1]^ecc_bit[2]^ecc_bit[191]^tmp[1]^tmp[2];
			CheckBits[188] = ecc_bit[2]^ecc_bit[3]^ecc_bit[192]^tmp[2]^tmp[3];
			CheckBits[189] = ecc_bit[1]^ecc_bit[193]^tmp[1];
			CheckBits[190] = ecc_bit[0]^ecc_bit[2]^ecc_bit[194]^tmp[0]^tmp[2];
			CheckBits[191] = ecc_bit[1]^ecc_bit[3]^ecc_bit[195]^tmp[1]^tmp[3];
			CheckBits[192] = ecc_bit[1]^ecc_bit[2]^ecc_bit[3]^ecc_bit[196]^tmp[1]^tmp[2]^tmp[3];
			CheckBits[193] = ecc_bit[0]^ecc_bit[1]^ecc_bit[2]^ecc_bit[197]^tmp[0]^tmp[1]^tmp[2];
			CheckBits[194] = ecc_bit[1]^ecc_bit[2]^ecc_bit[3]^ecc_bit[198]^tmp[1]^tmp[2]^tmp[3];
			CheckBits[195] = ecc_bit[0]^ecc_bit[1]^ecc_bit[2]^ecc_bit[199]^tmp[0]^tmp[1]^tmp[2];
			CheckBits[196] = ecc_bit[1]^ecc_bit[2]^ecc_bit[3]^ecc_bit[200]^tmp[1]^tmp[2]^tmp[3];
			CheckBits[197] = ecc_bit[0]^ecc_bit[1]^ecc_bit[2]^ecc_bit[201]^tmp[0]^tmp[1]^tmp[2];
			CheckBits[198] = ecc_bit[1]^ecc_bit[2]^ecc_bit[3]^ecc_bit[202]^tmp[1]^tmp[2]^tmp[3];
			CheckBits[199] = ecc_bit[1]^ecc_bit[2]^ecc_bit[203]^tmp[1]^tmp[2];
			CheckBits[200] = ecc_bit[0]^ecc_bit[2]^ecc_bit[3]^ecc_bit[204]^tmp[0]^tmp[2]^tmp[3];
			CheckBits[201] = ecc_bit[205];
			CheckBits[202] = ecc_bit[206];
			CheckBits[203] = ecc_bit[0]^ecc_bit[207]^tmp[0];
			CheckBits[204] = ecc_bit[1]^ecc_bit[208]^tmp[1];
			CheckBits[205] = ecc_bit[0]^ecc_bit[2]^ecc_bit[209]^tmp[0]^tmp[2];
			CheckBits[206] = ecc_bit[1]^ecc_bit[3]^ecc_bit[210]^tmp[1]^tmp[3];
			CheckBits[207] = ecc_bit[0]^ecc_bit[1]^ecc_bit[2]^ecc_bit[3]^ecc_bit[211]^tmp[0]^tmp[1]^tmp[2]^tmp[3];
			CheckBits[208] = ecc_bit[2]^ecc_bit[212]^tmp[2];
			CheckBits[209] = ecc_bit[3]^ecc_bit[213]^tmp[3];
			CheckBits[210] = ecc_bit[1]^ecc_bit[3]^ecc_bit[214]^tmp[1]^tmp[3];
			CheckBits[211] = ecc_bit[0]^ecc_bit[1]^ecc_bit[2]^ecc_bit[3]^ecc_bit[215]^tmp[0]^tmp[1]^tmp[2]^tmp[3];
			CheckBits[212] = ecc_bit[0]^ecc_bit[2]^ecc_bit[216]^tmp[0]^tmp[2];
			CheckBits[213] = ecc_bit[1]^ecc_bit[3]^ecc_bit[217]^tmp[1]^tmp[3];
			CheckBits[214] = ecc_bit[0]^ecc_bit[1]^ecc_bit[2]^ecc_bit[3]^ecc_bit[218]^tmp[0]^tmp[1]^tmp[2]^tmp[3];
			CheckBits[215] = ecc_bit[0]^ecc_bit[2]^ecc_bit[219]^tmp[0]^tmp[2];
			CheckBits[216] = ecc_bit[0]^ecc_bit[1]^ecc_bit[3]^ecc_bit[220]^tmp[0]^tmp[1]^tmp[3];
			CheckBits[217] = ecc_bit[2]^ecc_bit[3]^ecc_bit[221]^tmp[2]^tmp[3];
			CheckBits[218] = ecc_bit[1]^ecc_bit[222]^tmp[1];
			CheckBits[219] = ecc_bit[2]^ecc_bit[223]^tmp[2];
			CheckBits[220] = ecc_bit[0]^ecc_bit[3]^tmp[0]^tmp[3];
			CheckBits[221] = ecc_bit[3]^tmp[3];
			CheckBits[222] = ecc_bit[0]^ecc_bit[1]^ecc_bit[3]^tmp[0]^tmp[1]^tmp[3];
			CheckBits[223] = ecc_bit[0]^ecc_bit[2]^ecc_bit[3]^tmp[0]^tmp[2]^tmp[3];
		} else if(ecc_size == 21) {
			CheckBits[0] = ecc_bit[1]^ecc_bit[4]^tmp[1];
			CheckBits[1] = ecc_bit[2]^ecc_bit[5]^tmp[2];
			CheckBits[2] = ecc_bit[0]^ecc_bit[3]^ecc_bit[6]^tmp[0]^tmp[3];
			CheckBits[3] = ecc_bit[7];
			CheckBits[4] = ecc_bit[8];
			CheckBits[5] = ecc_bit[0]^ecc_bit[9]^tmp[0];
			CheckBits[6] = ecc_bit[1]^ecc_bit[10]^tmp[1];
			CheckBits[7] = ecc_bit[0]^ecc_bit[2]^ecc_bit[11]^tmp[0]^tmp[2];
			CheckBits[8] = ecc_bit[1]^ecc_bit[3]^ecc_bit[12]^tmp[1]^tmp[3];
			CheckBits[9] = ecc_bit[0]^ecc_bit[1]^ecc_bit[2]^ecc_bit[13]^tmp[0]^tmp[1]^tmp[2];
			CheckBits[10] = ecc_bit[0]^ecc_bit[1]^ecc_bit[2]^ecc_bit[3]^ecc_bit[14]^tmp[0]^tmp[1]^tmp[2]^tmp[3];
			CheckBits[11] = ecc_bit[2]^ecc_bit[3]^ecc_bit[15]^tmp[2]^tmp[3];
			CheckBits[12] = ecc_bit[1]^ecc_bit[3]^ecc_bit[16]^tmp[1]^tmp[3];
			CheckBits[13] = ecc_bit[0]^ecc_bit[1]^ecc_bit[2]^ecc_bit[17]^tmp[0]^tmp[1]^tmp[2];
			CheckBits[14] = ecc_bit[0]^ecc_bit[1]^ecc_bit[2]^ecc_bit[3]^ecc_bit[18]^tmp[0]^tmp[1]^tmp[2]^tmp[3];
			CheckBits[15] = ecc_bit[2]^ecc_bit[3]^ecc_bit[19]^tmp[2]^tmp[3];
			CheckBits[16] = ecc_bit[0]^ecc_bit[1]^ecc_bit[3]^ecc_bit[20]^tmp[0]^tmp[1]^tmp[3];
			CheckBits[17] = ecc_bit[2]^ecc_bit[21]^tmp[2];
			CheckBits[18] = ecc_bit[3]^ecc_bit[22]^tmp[3];
			CheckBits[19] = ecc_bit[1]^ecc_bit[23]^tmp[1];
			CheckBits[20] = ecc_bit[0]^ecc_bit[2]^ecc_bit[24]^tmp[0]^tmp[2];
			CheckBits[21] = ecc_bit[0]^ecc_bit[1]^ecc_bit[3]^ecc_bit[25]^tmp[0]^tmp[1]^tmp[3];
			CheckBits[22] = ecc_bit[0]^ecc_bit[2]^ecc_bit[26]^tmp[0]^tmp[2];
			CheckBits[23] = ecc_bit[0]^ecc_bit[1]^ecc_bit[3]^ecc_bit[27]^tmp[0]^tmp[1]^tmp[3];
			CheckBits[24] = ecc_bit[2]^ecc_bit[28]^tmp[2];
			CheckBits[25] = ecc_bit[3]^ecc_bit[29]^tmp[3];
			CheckBits[26] = ecc_bit[1]^ecc_bit[30]^tmp[1];
			CheckBits[27] = ecc_bit[2]^ecc_bit[31]^tmp[2];
			CheckBits[28] = ecc_bit[3]^ecc_bit[32]^tmp[3];
			CheckBits[29] = ecc_bit[1]^ecc_bit[33]^tmp[1];
			CheckBits[30] = ecc_bit[0]^ecc_bit[2]^ecc_bit[34]^tmp[0]^tmp[2];
			CheckBits[31] = ecc_bit[0]^ecc_bit[1]^ecc_bit[3]^ecc_bit[35]^tmp[0]^tmp[1]^tmp[3];
			CheckBits[32] = ecc_bit[0]^ecc_bit[2]^ecc_bit[36]^tmp[0]^tmp[2];
			CheckBits[33] = ecc_bit[0]^ecc_bit[1]^ecc_bit[3]^ecc_bit[37]^tmp[0]^tmp[1]^tmp[3];
			CheckBits[34] = ecc_bit[0]^ecc_bit[2]^ecc_bit[38]^tmp[0]^tmp[2];
			CheckBits[35] = ecc_bit[1]^ecc_bit[3]^ecc_bit[39]^tmp[1]^tmp[3];
			CheckBits[36] = ecc_bit[1]^ecc_bit[2]^ecc_bit[40]^tmp[1]^tmp[2];
			CheckBits[37] = ecc_bit[0]^ecc_bit[2]^ecc_bit[3]^ecc_bit[41]^tmp[0]^tmp[2]^tmp[3];
			CheckBits[38] = ecc_bit[3]^ecc_bit[42]^tmp[3];
			CheckBits[39] = ecc_bit[0]^ecc_bit[1]^ecc_bit[43]^tmp[0]^tmp[1];
			CheckBits[40] = ecc_bit[1]^ecc_bit[2]^ecc_bit[44]^tmp[1]^tmp[2];
			CheckBits[41] = ecc_bit[2]^ecc_bit[3]^ecc_bit[45]^tmp[2]^tmp[3];
			CheckBits[42] = ecc_bit[0]^ecc_bit[1]^ecc_bit[3]^ecc_bit[46]^tmp[0]^tmp[1]^tmp[3];
			CheckBits[43] = ecc_bit[0]^ecc_bit[2]^ecc_bit[47]^tmp[0]^tmp[2];
			CheckBits[44] = ecc_bit[1]^ecc_bit[3]^ecc_bit[48]^tmp[1]^tmp[3];
			CheckBits[45] = ecc_bit[0]^ecc_bit[1]^ecc_bit[2]^ecc_bit[49]^tmp[0]^tmp[1]^tmp[2];
			CheckBits[46] = ecc_bit[0]^ecc_bit[1]^ecc_bit[2]^ecc_bit[3]^ecc_bit[50]^tmp[0]^tmp[1]^tmp[2]^tmp[3];
			CheckBits[47] = ecc_bit[2]^ecc_bit[3]^ecc_bit[51]^tmp[2]^tmp[3];
			CheckBits[48] = ecc_bit[0]^ecc_bit[1]^ecc_bit[3]^ecc_bit[52]^tmp[0]^tmp[1]^tmp[3];
			CheckBits[49] = ecc_bit[2]^ecc_bit[53]^tmp[2];
			CheckBits[50] = ecc_bit[0]^ecc_bit[3]^ecc_bit[54]^tmp[0]^tmp[3];
			CheckBits[51] = ecc_bit[0]^ecc_bit[55]^tmp[0];
			CheckBits[52] = ecc_bit[1]^ecc_bit[56]^tmp[1];
			CheckBits[53] = ecc_bit[2]^ecc_bit[57]^tmp[2];
			CheckBits[54] = ecc_bit[0]^ecc_bit[3]^ecc_bit[58]^tmp[0]^tmp[3];
			CheckBits[55] = ecc_bit[0]^ecc_bit[59]^tmp[0];
			CheckBits[56] = ecc_bit[0]^ecc_bit[1]^ecc_bit[60]^tmp[0]^tmp[1];
			CheckBits[57] = ecc_bit[1]^ecc_bit[2]^ecc_bit[61]^tmp[1]^tmp[2];
			CheckBits[58] = ecc_bit[0]^ecc_bit[2]^ecc_bit[3]^ecc_bit[62]^tmp[0]^tmp[2]^tmp[3];
			CheckBits[59] = ecc_bit[0]^ecc_bit[3]^ecc_bit[63]^tmp[0]^tmp[3];
			CheckBits[60] = ecc_bit[64];
			CheckBits[61] = ecc_bit[65];
			CheckBits[62] = ecc_bit[66];
			CheckBits[63] = ecc_bit[67];
			CheckBits[64] = ecc_bit[68];
			CheckBits[65] = ecc_bit[69];
			CheckBits[66] = ecc_bit[0]^ecc_bit[70]^tmp[0];
			CheckBits[67] = ecc_bit[1]^ecc_bit[71]^tmp[1];
			CheckBits[68] = ecc_bit[2]^ecc_bit[72]^tmp[2];
			CheckBits[69] = ecc_bit[3]^ecc_bit[73]^tmp[3];
			CheckBits[70] = ecc_bit[1]^ecc_bit[74]^tmp[1];
			CheckBits[71] = ecc_bit[0]^ecc_bit[2]^ecc_bit[75]^tmp[0]^tmp[2];
			CheckBits[72] = ecc_bit[1]^ecc_bit[3]^ecc_bit[76]^tmp[1]^tmp[3];
			CheckBits[73] = ecc_bit[0]^ecc_bit[1]^ecc_bit[2]^ecc_bit[77]^tmp[0]^tmp[1]^tmp[2];
			CheckBits[74] = ecc_bit[1]^ecc_bit[2]^ecc_bit[3]^ecc_bit[78]^tmp[1]^tmp[2]^tmp[3];
			CheckBits[75] = ecc_bit[1]^ecc_bit[2]^ecc_bit[3]^ecc_bit[79]^tmp[1]^tmp[2]^tmp[3];
			CheckBits[76] = ecc_bit[1]^ecc_bit[2]^ecc_bit[3]^ecc_bit[80]^tmp[1]^tmp[2]^tmp[3];
			CheckBits[77] = ecc_bit[0]^ecc_bit[1]^ecc_bit[2]^ecc_bit[3]^ecc_bit[81]^tmp[0]^tmp[1]^tmp[2]^tmp[3];
			CheckBits[78] = ecc_bit[0]^ecc_bit[2]^ecc_bit[3]^ecc_bit[82]^tmp[0]^tmp[2]^tmp[3];
			CheckBits[79] = ecc_bit[3]^ecc_bit[83]^tmp[3];
			CheckBits[80] = ecc_bit[0]^ecc_bit[1]^ecc_bit[84]^tmp[0]^tmp[1];
			CheckBits[81] = ecc_bit[1]^ecc_bit[2]^ecc_bit[85]^tmp[1]^tmp[2];
			CheckBits[82] = ecc_bit[2]^ecc_bit[3]^ecc_bit[86]^tmp[2]^tmp[3];
			CheckBits[83] = ecc_bit[0]^ecc_bit[1]^ecc_bit[3]^ecc_bit[87]^tmp[0]^tmp[1]^tmp[3];
			CheckBits[84] = ecc_bit[2]^ecc_bit[88]^tmp[2];
			CheckBits[85] = ecc_bit[0]^ecc_bit[3]^ecc_bit[89]^tmp[0]^tmp[3];
			CheckBits[86] = ecc_bit[0]^ecc_bit[90]^tmp[0];
			CheckBits[87] = ecc_bit[1]^ecc_bit[91]^tmp[1];
			CheckBits[88] = ecc_bit[0]^ecc_bit[2]^ecc_bit[92]^tmp[0]^tmp[2];
			CheckBits[89] = ecc_bit[1]^ecc_bit[3]^ecc_bit[93]^tmp[1]^tmp[3];
			CheckBits[90] = ecc_bit[0]^ecc_bit[1]^ecc_bit[2]^ecc_bit[94]^tmp[0]^tmp[1]^tmp[2];
			CheckBits[91] = ecc_bit[0]^ecc_bit[1]^ecc_bit[2]^ecc_bit[3]^ecc_bit[95]^tmp[0]^tmp[1]^tmp[2]^tmp[3];
			CheckBits[92] = ecc_bit[2]^ecc_bit[3]^ecc_bit[96]^tmp[2]^tmp[3];
			CheckBits[93] = ecc_bit[0]^ecc_bit[1]^ecc_bit[3]^ecc_bit[97]^tmp[0]^tmp[1]^tmp[3];
			CheckBits[94] = ecc_bit[0]^ecc_bit[2]^ecc_bit[98]^tmp[0]^tmp[2];
			CheckBits[95] = ecc_bit[1]^ecc_bit[3]^ecc_bit[99]^tmp[1]^tmp[3];
			CheckBits[96] = ecc_bit[0]^ecc_bit[1]^ecc_bit[2]^ecc_bit[100]^tmp[0]^tmp[1]^tmp[2];
			CheckBits[97] = ecc_bit[1]^ecc_bit[2]^ecc_bit[3]^ecc_bit[101]^tmp[1]^tmp[2]^tmp[3];
			CheckBits[98] = ecc_bit[0]^ecc_bit[1]^ecc_bit[2]^ecc_bit[3]^ecc_bit[102]^tmp[0]^tmp[1]^tmp[2]^tmp[3];
			CheckBits[99] = ecc_bit[2]^ecc_bit[3]^ecc_bit[103]^tmp[2]^tmp[3];
			CheckBits[100] = ecc_bit[0]^ecc_bit[1]^ecc_bit[3]^ecc_bit[104]^tmp[0]^tmp[1]^tmp[3];
			CheckBits[101] = ecc_bit[0]^ecc_bit[2]^ecc_bit[105]^tmp[0]^tmp[2];
			CheckBits[102] = ecc_bit[1]^ecc_bit[3]^ecc_bit[106]^tmp[1]^tmp[3];
			CheckBits[103] = ecc_bit[1]^ecc_bit[2]^ecc_bit[107]^tmp[1]^tmp[2];
			CheckBits[104] = ecc_bit[2]^ecc_bit[3]^ecc_bit[108]^tmp[2]^tmp[3];
			CheckBits[105] = ecc_bit[1]^ecc_bit[3]^ecc_bit[109]^tmp[1]^tmp[3];
			CheckBits[106] = ecc_bit[0]^ecc_bit[1]^ecc_bit[2]^ecc_bit[110]^tmp[0]^tmp[1]^tmp[2];
			CheckBits[107] = ecc_bit[0]^ecc_bit[1]^ecc_bit[2]^ecc_bit[3]^ecc_bit[111]^tmp[0]^tmp[1]^tmp[2]^tmp[3];
			CheckBits[108] = ecc_bit[0]^ecc_bit[2]^ecc_bit[3]^ecc_bit[112]^tmp[0]^tmp[2]^tmp[3];
			CheckBits[109] = ecc_bit[0]^ecc_bit[3]^ecc_bit[113]^tmp[0]^tmp[3];
			CheckBits[110] = ecc_bit[0]^ecc_bit[114]^tmp[0];
			CheckBits[111] = ecc_bit[0]^ecc_bit[1]^ecc_bit[115]^tmp[0]^tmp[1];
			CheckBits[112] = ecc_bit[0]^ecc_bit[1]^ecc_bit[2]^ecc_bit[116]^tmp[0]^tmp[1]^tmp[2];
			CheckBits[113] = ecc_bit[1]^ecc_bit[2]^ecc_bit[3]^ecc_bit[117]^tmp[1]^tmp[2]^tmp[3];
			CheckBits[114] = ecc_bit[0]^ecc_bit[1]^ecc_bit[2]^ecc_bit[3]^ecc_bit[118]^tmp[0]^tmp[1]^tmp[2]^tmp[3];
			CheckBits[115] = ecc_bit[0]^ecc_bit[2]^ecc_bit[3]^ecc_bit[119]^tmp[0]^tmp[2]^tmp[3];
			CheckBits[116] = ecc_bit[3]^ecc_bit[120]^tmp[3];
			CheckBits[117] = ecc_bit[0]^ecc_bit[1]^ecc_bit[121]^tmp[0]^tmp[1];
			CheckBits[118] = ecc_bit[0]^ecc_bit[1]^ecc_bit[2]^ecc_bit[122]^tmp[0]^tmp[1]^tmp[2];
			CheckBits[119] = ecc_bit[1]^ecc_bit[2]^ecc_bit[3]^ecc_bit[123]^tmp[1]^tmp[2]^tmp[3];
			CheckBits[120] = ecc_bit[0]^ecc_bit[1]^ecc_bit[2]^ecc_bit[3]^ecc_bit[124]^tmp[0]^tmp[1]^tmp[2]^tmp[3];
			CheckBits[121] = ecc_bit[2]^ecc_bit[3]^ecc_bit[125]^tmp[2]^tmp[3];
			CheckBits[122] = ecc_bit[0]^ecc_bit[1]^ecc_bit[3]^ecc_bit[126]^tmp[0]^tmp[1]^tmp[3];
			CheckBits[123] = ecc_bit[0]^ecc_bit[2]^ecc_bit[127]^tmp[0]^tmp[2];
			CheckBits[124] = ecc_bit[0]^ecc_bit[1]^ecc_bit[3]^ecc_bit[128]^tmp[0]^tmp[1]^tmp[3];
			CheckBits[125] = ecc_bit[0]^ecc_bit[2]^ecc_bit[129]^tmp[0]^tmp[2];
			CheckBits[126] = ecc_bit[1]^ecc_bit[3]^ecc_bit[130]^tmp[1]^tmp[3];
			CheckBits[127] = ecc_bit[0]^ecc_bit[1]^ecc_bit[2]^ecc_bit[131]^tmp[0]^tmp[1]^tmp[2];
			CheckBits[128] = ecc_bit[0]^ecc_bit[1]^ecc_bit[2]^ecc_bit[3]^ecc_bit[132]^tmp[0]^tmp[1]^tmp[2]^tmp[3];
			CheckBits[129] = ecc_bit[0]^ecc_bit[2]^ecc_bit[3]^ecc_bit[133]^tmp[0]^tmp[2]^tmp[3];
			CheckBits[130] = ecc_bit[0]^ecc_bit[3]^ecc_bit[134]^tmp[0]^tmp[3];
			CheckBits[131] = ecc_bit[0]^ecc_bit[135]^tmp[0];
			CheckBits[132] = ecc_bit[0]^ecc_bit[1]^ecc_bit[136]^tmp[0]^tmp[1];
			CheckBits[133] = ecc_bit[1]^ecc_bit[2]^ecc_bit[137]^tmp[1]^tmp[2];
			CheckBits[134] = ecc_bit[0]^ecc_bit[2]^ecc_bit[3]^ecc_bit[138]^tmp[0]^tmp[2]^tmp[3];
			CheckBits[135] = ecc_bit[3]^ecc_bit[139]^tmp[3];
			CheckBits[136] = ecc_bit[0]^ecc_bit[1]^ecc_bit[140]^tmp[0]^tmp[1];
			CheckBits[137] = ecc_bit[1]^ecc_bit[2]^ecc_bit[141]^tmp[1]^tmp[2];
			CheckBits[138] = ecc_bit[0]^ecc_bit[2]^ecc_bit[3]^ecc_bit[142]^tmp[0]^tmp[2]^tmp[3];
			CheckBits[139] = ecc_bit[3]^ecc_bit[143]^tmp[3];
			CheckBits[140] = ecc_bit[0]^ecc_bit[1]^ecc_bit[144]^tmp[0]^tmp[1];
			CheckBits[141] = ecc_bit[1]^ecc_bit[2]^ecc_bit[145]^tmp[1]^tmp[2];
			CheckBits[142] = ecc_bit[0]^ecc_bit[2]^ecc_bit[3]^ecc_bit[146]^tmp[0]^tmp[2]^tmp[3];
			CheckBits[143] = ecc_bit[0]^ecc_bit[3]^ecc_bit[147]^tmp[0]^tmp[3];
			CheckBits[144] = ecc_bit[148];
			CheckBits[145] = ecc_bit[0]^ecc_bit[149]^tmp[0];
			CheckBits[146] = ecc_bit[0]^ecc_bit[1]^ecc_bit[150]^tmp[0]^tmp[1];
			CheckBits[147] = ecc_bit[1]^ecc_bit[2]^ecc_bit[151]^tmp[1]^tmp[2];
			CheckBits[148] = ecc_bit[0]^ecc_bit[2]^ecc_bit[3]^ecc_bit[152]^tmp[0]^tmp[2]^tmp[3];
			CheckBits[149] = ecc_bit[0]^ecc_bit[3]^ecc_bit[153]^tmp[0]^tmp[3];
			CheckBits[150] = ecc_bit[0]^ecc_bit[154]^tmp[0];
			CheckBits[151] = ecc_bit[1]^ecc_bit[155]^tmp[1];
			CheckBits[152] = ecc_bit[0]^ecc_bit[2]^ecc_bit[156]^tmp[0]^tmp[2];
			CheckBits[153] = ecc_bit[0]^ecc_bit[1]^ecc_bit[3]^ecc_bit[157]^tmp[0]^tmp[1]^tmp[3];
			CheckBits[154] = ecc_bit[0]^ecc_bit[2]^ecc_bit[158]^tmp[0]^tmp[2];
			CheckBits[155] = ecc_bit[1]^ecc_bit[3]^ecc_bit[159]^tmp[1]^tmp[3];
			CheckBits[156] = ecc_bit[1]^ecc_bit[2]^ecc_bit[160]^tmp[1]^tmp[2];
			CheckBits[157] = ecc_bit[0]^ecc_bit[2]^ecc_bit[3]^ecc_bit[161]^tmp[0]^tmp[2]^tmp[3];
			CheckBits[158] = ecc_bit[0]^ecc_bit[3]^ecc_bit[162]^tmp[0]^tmp[3];
			CheckBits[159] = ecc_bit[163];
			CheckBits[160] = ecc_bit[164];
			CheckBits[161] = ecc_bit[0]^ecc_bit[165]^tmp[0];
			CheckBits[162] = ecc_bit[0]^ecc_bit[1]^ecc_bit[166]^tmp[0]^tmp[1];
			CheckBits[163] = ecc_bit[1]^ecc_bit[2]^ecc_bit[167]^tmp[1]^tmp[2];
			CheckBits[164] = ecc_bit[2]^ecc_bit[3]^tmp[2]^tmp[3];
			CheckBits[165] = ecc_bit[0]^ecc_bit[1]^ecc_bit[3]^tmp[0]^tmp[1]^tmp[3];
			CheckBits[166] = ecc_bit[2]^tmp[2];
			CheckBits[167] = ecc_bit[0]^ecc_bit[3]^tmp[0]^tmp[3];
		} else if(ecc_size == 14) {
			CheckBits[0] = ecc_bit[3]^ecc_bit[4]^tmp[3];
			CheckBits[1] = ecc_bit[3]^ecc_bit[5]^tmp[3];
			CheckBits[2] = ecc_bit[3]^ecc_bit[6]^tmp[3];
			CheckBits[3] = ecc_bit[3]^ecc_bit[7]^tmp[3];
			CheckBits[4] = ecc_bit[0]^ecc_bit[3]^ecc_bit[8]^tmp[0]^tmp[3];
			CheckBits[5] = ecc_bit[1]^ecc_bit[3]^ecc_bit[9]^tmp[1]^tmp[3];
			CheckBits[6] = ecc_bit[0]^ecc_bit[2]^ecc_bit[3]^ecc_bit[10]^tmp[0]^tmp[2]^tmp[3];
			CheckBits[7] = ecc_bit[1]^ecc_bit[11]^tmp[1];
			CheckBits[8] = ecc_bit[2]^ecc_bit[12]^tmp[2];
			CheckBits[9] = ecc_bit[3]^ecc_bit[13]^tmp[3];
			CheckBits[10] = ecc_bit[0]^ecc_bit[3]^ecc_bit[14]^tmp[0]^tmp[3];
			CheckBits[11] = ecc_bit[0]^ecc_bit[1]^ecc_bit[3]^ecc_bit[15]^tmp[0]^tmp[1]^tmp[3];
			CheckBits[12] = ecc_bit[1]^ecc_bit[2]^ecc_bit[3]^ecc_bit[16]^tmp[1]^tmp[2]^tmp[3];
			CheckBits[13] = ecc_bit[2]^ecc_bit[17]^tmp[2];
			CheckBits[14] = ecc_bit[0]^ecc_bit[3]^ecc_bit[18]^tmp[0]^tmp[3];
			CheckBits[15] = ecc_bit[1]^ecc_bit[3]^ecc_bit[19]^tmp[1]^tmp[3];
			CheckBits[16] = ecc_bit[0]^ecc_bit[2]^ecc_bit[3]^ecc_bit[20]^tmp[0]^tmp[2]^tmp[3];
			CheckBits[17] = ecc_bit[0]^ecc_bit[1]^ecc_bit[21]^tmp[0]^tmp[1];
			CheckBits[18] = ecc_bit[0]^ecc_bit[1]^ecc_bit[2]^ecc_bit[22]^tmp[0]^tmp[1]^tmp[2];
			CheckBits[19] = ecc_bit[1]^ecc_bit[2]^ecc_bit[3]^ecc_bit[23]^tmp[1]^tmp[2]^tmp[3];
			CheckBits[20] = ecc_bit[2]^ecc_bit[24]^tmp[2];
			CheckBits[21] = ecc_bit[3]^ecc_bit[25]^tmp[3];
			CheckBits[22] = ecc_bit[3]^ecc_bit[26]^tmp[3];
			CheckBits[23] = ecc_bit[3]^ecc_bit[27]^tmp[3];
			CheckBits[24] = ecc_bit[3]^ecc_bit[28]^tmp[3];
			CheckBits[25] = ecc_bit[3]^ecc_bit[29]^tmp[3];
			CheckBits[26] = ecc_bit[3]^ecc_bit[30]^tmp[3];
			CheckBits[27] = ecc_bit[3]^ecc_bit[31]^tmp[3];
			CheckBits[28] = ecc_bit[0]^ecc_bit[3]^ecc_bit[32]^tmp[0]^tmp[3];
			CheckBits[29] = ecc_bit[1]^ecc_bit[3]^ecc_bit[33]^tmp[1]^tmp[3];
			CheckBits[30] = ecc_bit[0]^ecc_bit[2]^ecc_bit[3]^ecc_bit[34]^tmp[0]^tmp[2]^tmp[3];
			CheckBits[31] = ecc_bit[0]^ecc_bit[1]^ecc_bit[35]^tmp[0]^tmp[1];
			CheckBits[32] = ecc_bit[0]^ecc_bit[1]^ecc_bit[2]^ecc_bit[36]^tmp[0]^tmp[1]^tmp[2];
			CheckBits[33] = ecc_bit[0]^ecc_bit[1]^ecc_bit[2]^ecc_bit[3]^ecc_bit[37]^tmp[0]^tmp[1]^tmp[2]^tmp[3];
			CheckBits[34] = ecc_bit[1]^ecc_bit[2]^ecc_bit[38]^tmp[1]^tmp[2];
			CheckBits[35] = ecc_bit[0]^ecc_bit[2]^ecc_bit[3]^ecc_bit[39]^tmp[0]^tmp[2]^tmp[3];
			CheckBits[36] = ecc_bit[1]^ecc_bit[40]^tmp[1];
			CheckBits[37] = ecc_bit[2]^ecc_bit[41]^tmp[2];
			CheckBits[38] = ecc_bit[0]^ecc_bit[3]^ecc_bit[42]^tmp[0]^tmp[3];
			CheckBits[39] = ecc_bit[1]^ecc_bit[3]^ecc_bit[43]^tmp[1]^tmp[3];
			CheckBits[40] = ecc_bit[2]^ecc_bit[3]^ecc_bit[44]^tmp[2]^tmp[3];
			CheckBits[41] = ecc_bit[45];
			CheckBits[42] = ecc_bit[0]^ecc_bit[46]^tmp[0];
			CheckBits[43] = ecc_bit[0]^ecc_bit[1]^ecc_bit[47]^tmp[0]^tmp[1];
			CheckBits[44] = ecc_bit[1]^ecc_bit[2]^ecc_bit[48]^tmp[1]^tmp[2];
			CheckBits[45] = ecc_bit[2]^ecc_bit[3]^ecc_bit[49]^tmp[2]^tmp[3];
			CheckBits[46] = ecc_bit[50];
			CheckBits[47] = ecc_bit[0]^ecc_bit[51]^tmp[0];
			CheckBits[48] = ecc_bit[1]^ecc_bit[52]^tmp[1];
			CheckBits[49] = ecc_bit[2]^ecc_bit[53]^tmp[2];
			CheckBits[50] = ecc_bit[3]^ecc_bit[54]^tmp[3];
			CheckBits[51] = ecc_bit[0]^ecc_bit[3]^ecc_bit[55]^tmp[0]^tmp[3];
			CheckBits[52] = ecc_bit[1]^ecc_bit[3]^ecc_bit[56]^tmp[1]^tmp[3];
			CheckBits[53] = ecc_bit[2]^ecc_bit[3]^ecc_bit[57]^tmp[2]^tmp[3];
			CheckBits[54] = ecc_bit[0]^ecc_bit[58]^tmp[0];
			CheckBits[55] = ecc_bit[1]^ecc_bit[59]^tmp[1];
			CheckBits[56] = ecc_bit[2]^ecc_bit[60]^tmp[2];
			CheckBits[57] = ecc_bit[0]^ecc_bit[3]^ecc_bit[61]^tmp[0]^tmp[3];
			CheckBits[58] = ecc_bit[0]^ecc_bit[1]^ecc_bit[3]^ecc_bit[62]^tmp[0]^tmp[1]^tmp[3];
			CheckBits[59] = ecc_bit[1]^ecc_bit[2]^ecc_bit[3]^ecc_bit[63]^tmp[1]^tmp[2]^tmp[3];
			CheckBits[60] = ecc_bit[0]^ecc_bit[2]^ecc_bit[64]^tmp[0]^tmp[2];
			CheckBits[61] = ecc_bit[1]^ecc_bit[3]^ecc_bit[65]^tmp[1]^tmp[3];
			CheckBits[62] = ecc_bit[0]^ecc_bit[2]^ecc_bit[3]^ecc_bit[66]^tmp[0]^tmp[2]^tmp[3];
			CheckBits[63] = ecc_bit[0]^ecc_bit[1]^ecc_bit[67]^tmp[0]^tmp[1];
			CheckBits[64] = ecc_bit[0]^ecc_bit[1]^ecc_bit[2]^ecc_bit[68]^tmp[0]^tmp[1]^tmp[2];
			CheckBits[65] = ecc_bit[1]^ecc_bit[2]^ecc_bit[3]^ecc_bit[69]^tmp[1]^tmp[2]^tmp[3];
			CheckBits[66] = ecc_bit[0]^ecc_bit[2]^ecc_bit[70]^tmp[0]^tmp[2];
			CheckBits[67] = ecc_bit[1]^ecc_bit[3]^ecc_bit[71]^tmp[1]^tmp[3];
			CheckBits[68] = ecc_bit[0]^ecc_bit[2]^ecc_bit[3]^ecc_bit[72]^tmp[0]^tmp[2]^tmp[3];
			CheckBits[69] = ecc_bit[1]^ecc_bit[73]^tmp[1];
			CheckBits[70] = ecc_bit[2]^ecc_bit[74]^tmp[2];
			CheckBits[71] = ecc_bit[3]^ecc_bit[75]^tmp[3];
			CheckBits[72] = ecc_bit[0]^ecc_bit[3]^ecc_bit[76]^tmp[0]^tmp[3];
			CheckBits[73] = ecc_bit[1]^ecc_bit[3]^ecc_bit[77]^tmp[1]^tmp[3];
			CheckBits[74] = ecc_bit[2]^ecc_bit[3]^ecc_bit[78]^tmp[2]^tmp[3];
			CheckBits[75] = ecc_bit[0]^ecc_bit[79]^tmp[0];
			CheckBits[76] = ecc_bit[0]^ecc_bit[1]^ecc_bit[80]^tmp[0]^tmp[1];
			CheckBits[77] = ecc_bit[1]^ecc_bit[2]^ecc_bit[81]^tmp[1]^tmp[2];
			CheckBits[78] = ecc_bit[0]^ecc_bit[2]^ecc_bit[3]^ecc_bit[82]^tmp[0]^tmp[2]^tmp[3];
			CheckBits[79] = ecc_bit[1]^ecc_bit[83]^tmp[1];
			CheckBits[80] = ecc_bit[2]^ecc_bit[84]^tmp[2];
			CheckBits[81] = ecc_bit[0]^ecc_bit[3]^ecc_bit[85]^tmp[0]^tmp[3];
			CheckBits[82] = ecc_bit[1]^ecc_bit[3]^ecc_bit[86]^tmp[1]^tmp[3];
			CheckBits[83] = ecc_bit[0]^ecc_bit[2]^ecc_bit[3]^ecc_bit[87]^tmp[0]^tmp[2]^tmp[3];
			CheckBits[84] = ecc_bit[1]^ecc_bit[88]^tmp[1];
			CheckBits[85] = ecc_bit[2]^ecc_bit[89]^tmp[2];
			CheckBits[86] = ecc_bit[3]^ecc_bit[90]^tmp[3];
			CheckBits[87] = ecc_bit[0]^ecc_bit[3]^ecc_bit[91]^tmp[0]^tmp[3];
			CheckBits[88] = ecc_bit[1]^ecc_bit[3]^ecc_bit[92]^tmp[1]^tmp[3];
			CheckBits[89] = ecc_bit[0]^ecc_bit[2]^ecc_bit[3]^ecc_bit[93]^tmp[0]^tmp[2]^tmp[3];
			CheckBits[90] = ecc_bit[1]^ecc_bit[94]^tmp[1];
			CheckBits[91] = ecc_bit[0]^ecc_bit[2]^ecc_bit[95]^tmp[0]^tmp[2];
			CheckBits[92] = ecc_bit[0]^ecc_bit[1]^ecc_bit[3]^ecc_bit[96]^tmp[0]^tmp[1]^tmp[3];
			CheckBits[93] = ecc_bit[0]^ecc_bit[1]^ecc_bit[2]^ecc_bit[3]^ecc_bit[97]^tmp[0]^tmp[1]^tmp[2]^tmp[3];
			CheckBits[94] = ecc_bit[0]^ecc_bit[1]^ecc_bit[2]^ecc_bit[98]^tmp[0]^tmp[1]^tmp[2];
			CheckBits[95] = ecc_bit[0]^ecc_bit[1]^ecc_bit[2]^ecc_bit[3]^ecc_bit[99]^tmp[0]^tmp[1]^tmp[2]^tmp[3];
			CheckBits[96] = ecc_bit[0]^ecc_bit[1]^ecc_bit[2]^ecc_bit[100]^tmp[0]^tmp[1]^tmp[2];
			CheckBits[97] = ecc_bit[1]^ecc_bit[2]^ecc_bit[3]^ecc_bit[101]^tmp[1]^tmp[2]^tmp[3];
			CheckBits[98] = ecc_bit[2]^ecc_bit[102]^tmp[2];
			CheckBits[99] = ecc_bit[0]^ecc_bit[3]^ecc_bit[103]^tmp[0]^tmp[3];
			CheckBits[100] = ecc_bit[1]^ecc_bit[3]^ecc_bit[104]^tmp[1]^tmp[3];
			CheckBits[101] = ecc_bit[2]^ecc_bit[3]^ecc_bit[105]^tmp[2]^tmp[3];
			CheckBits[102] = ecc_bit[106];
			CheckBits[103] = ecc_bit[0]^ecc_bit[107]^tmp[0];
			CheckBits[104] = ecc_bit[0]^ecc_bit[1]^ecc_bit[108]^tmp[0]^tmp[1];
			CheckBits[105] = ecc_bit[0]^ecc_bit[1]^ecc_bit[2]^ecc_bit[109]^tmp[0]^tmp[1]^tmp[2];
			CheckBits[106] = ecc_bit[1]^ecc_bit[2]^ecc_bit[3]^ecc_bit[110]^tmp[1]^tmp[2]^tmp[3];
			CheckBits[107] = ecc_bit[0]^ecc_bit[2]^ecc_bit[111]^tmp[0]^tmp[2];
			CheckBits[108] = ecc_bit[0]^ecc_bit[1]^ecc_bit[3]^tmp[0]^tmp[1]^tmp[3];
			CheckBits[109] = ecc_bit[0]^ecc_bit[1]^ecc_bit[2]^ecc_bit[3]^tmp[0]^tmp[1]^tmp[2]^tmp[3];
			CheckBits[110] = ecc_bit[1]^ecc_bit[2]^tmp[1]^tmp[2];
			CheckBits[111] = ecc_bit[2]^ecc_bit[3]^tmp[2]^tmp[3];
		} else if(ecc_size == 7) {
			CheckBits[0] = ecc_bit[0]^ecc_bit[2]^ecc_bit[4]^tmp[0]^tmp[2];
			CheckBits[1] = ecc_bit[1]^ecc_bit[3]^ecc_bit[5]^tmp[1]^tmp[3];
			CheckBits[2] = ecc_bit[6];
			CheckBits[3] = ecc_bit[0]^ecc_bit[7]^tmp[0];
			CheckBits[4] = ecc_bit[1]^ecc_bit[8]^tmp[1];
			CheckBits[5] = ecc_bit[0]^ecc_bit[2]^ecc_bit[9]^tmp[0]^tmp[2];
			CheckBits[6] = ecc_bit[0]^ecc_bit[1]^ecc_bit[3]^ecc_bit[10]^tmp[0]^tmp[1]^tmp[3];
			CheckBits[7] = ecc_bit[0]^ecc_bit[1]^ecc_bit[11]^tmp[0]^tmp[1];
			CheckBits[8] = ecc_bit[1]^ecc_bit[2]^ecc_bit[12]^tmp[1]^tmp[2];
			CheckBits[9] = ecc_bit[0]^ecc_bit[2]^ecc_bit[3]^ecc_bit[13]^tmp[0]^tmp[2]^tmp[3];
			CheckBits[10] = ecc_bit[1]^ecc_bit[2]^ecc_bit[3]^ecc_bit[14]^tmp[1]^tmp[2]^tmp[3];
			CheckBits[11] = ecc_bit[3]^ecc_bit[15]^tmp[3];
			CheckBits[12] = ecc_bit[0]^ecc_bit[2]^ecc_bit[16]^tmp[0]^tmp[2];
			CheckBits[13] = ecc_bit[0]^ecc_bit[1]^ecc_bit[3]^ecc_bit[17]^tmp[0]^tmp[1]^tmp[3];
			CheckBits[14] = ecc_bit[1]^ecc_bit[18]^tmp[1];
			CheckBits[15] = ecc_bit[2]^ecc_bit[19]^tmp[2];
			CheckBits[16] = ecc_bit[0]^ecc_bit[3]^ecc_bit[20]^tmp[0]^tmp[3];
			CheckBits[17] = ecc_bit[0]^ecc_bit[1]^ecc_bit[2]^ecc_bit[21]^tmp[0]^tmp[1]^tmp[2];
			CheckBits[18] = ecc_bit[0]^ecc_bit[1]^ecc_bit[2]^ecc_bit[3]^ecc_bit[22]^tmp[0]^tmp[1]^tmp[2]^tmp[3];
			CheckBits[19] = ecc_bit[0]^ecc_bit[1]^ecc_bit[3]^ecc_bit[23]^tmp[0]^tmp[1]^tmp[3];
			CheckBits[20] = ecc_bit[0]^ecc_bit[1]^ecc_bit[24]^tmp[0]^tmp[1];
			CheckBits[21] = ecc_bit[0]^ecc_bit[1]^ecc_bit[2]^ecc_bit[25]^tmp[0]^tmp[1]^tmp[2];
			CheckBits[22] = ecc_bit[0]^ecc_bit[1]^ecc_bit[2]^ecc_bit[3]^ecc_bit[26]^tmp[0]^tmp[1]^tmp[2]^tmp[3];
			CheckBits[23] = ecc_bit[0]^ecc_bit[1]^ecc_bit[3]^ecc_bit[27]^tmp[0]^tmp[1]^tmp[3];
			CheckBits[24] = ecc_bit[1]^ecc_bit[28]^tmp[1];
			CheckBits[25] = ecc_bit[0]^ecc_bit[2]^ecc_bit[29]^tmp[0]^tmp[2];
			CheckBits[26] = ecc_bit[1]^ecc_bit[3]^ecc_bit[30]^tmp[1]^tmp[3];
			CheckBits[27] = ecc_bit[0]^ecc_bit[31]^tmp[0];
			CheckBits[28] = ecc_bit[1]^ecc_bit[32]^tmp[1];
			CheckBits[29] = ecc_bit[2]^ecc_bit[33]^tmp[2];
			CheckBits[30] = ecc_bit[3]^ecc_bit[34]^tmp[3];
			CheckBits[31] = ecc_bit[2]^ecc_bit[35]^tmp[2];
			CheckBits[32] = ecc_bit[3]^ecc_bit[36]^tmp[3];
			CheckBits[33] = ecc_bit[0]^ecc_bit[2]^ecc_bit[37]^tmp[0]^tmp[2];
			CheckBits[34] = ecc_bit[1]^ecc_bit[3]^ecc_bit[38]^tmp[1]^tmp[3];
			CheckBits[35] = ecc_bit[0]^ecc_bit[39]^tmp[0];
			CheckBits[36] = ecc_bit[1]^ecc_bit[40]^tmp[1];
			CheckBits[37] = ecc_bit[0]^ecc_bit[2]^ecc_bit[41]^tmp[0]^tmp[2];
			CheckBits[38] = ecc_bit[1]^ecc_bit[3]^ecc_bit[42]^tmp[1]^tmp[3];
			CheckBits[39] = ecc_bit[0]^ecc_bit[43]^tmp[0];
			CheckBits[40] = ecc_bit[1]^ecc_bit[44]^tmp[1];
			CheckBits[41] = ecc_bit[0]^ecc_bit[2]^ecc_bit[45]^tmp[0]^tmp[2];
			CheckBits[42] = ecc_bit[0]^ecc_bit[1]^ecc_bit[3]^ecc_bit[46]^tmp[0]^tmp[1]^tmp[3];
			CheckBits[43] = ecc_bit[1]^ecc_bit[47]^tmp[1];
			CheckBits[44] = ecc_bit[2]^ecc_bit[48]^tmp[2];
			CheckBits[45] = ecc_bit[0]^ecc_bit[3]^ecc_bit[49]^tmp[0]^tmp[3];
			CheckBits[46] = ecc_bit[0]^ecc_bit[1]^ecc_bit[2]^ecc_bit[50]^tmp[0]^tmp[1]^tmp[2];
			CheckBits[47] = ecc_bit[1]^ecc_bit[2]^ecc_bit[3]^ecc_bit[51]^tmp[1]^tmp[2]^tmp[3];
			CheckBits[48] = ecc_bit[0]^ecc_bit[3]^ecc_bit[52]^tmp[0]^tmp[3];
			CheckBits[49] = ecc_bit[0]^ecc_bit[1]^ecc_bit[2]^ecc_bit[53]^tmp[0]^tmp[1]^tmp[2];
			CheckBits[50] = ecc_bit[0]^ecc_bit[1]^ecc_bit[2]^ecc_bit[3]^ecc_bit[54]^tmp[0]^tmp[1]^tmp[2]^tmp[3];
			CheckBits[51] = ecc_bit[0]^ecc_bit[1]^ecc_bit[3]^ecc_bit[55]^tmp[0]^tmp[1]^tmp[3];
			CheckBits[52] = ecc_bit[0]^ecc_bit[1]^tmp[0]^tmp[1];
			CheckBits[53] = ecc_bit[0]^ecc_bit[1]^ecc_bit[2]^tmp[0]^tmp[1]^tmp[2];
			CheckBits[54] = ecc_bit[0]^ecc_bit[1]^ecc_bit[2]^ecc_bit[3]^tmp[0]^tmp[1]^tmp[2]^tmp[3];
			CheckBits[55] = ecc_bit[1]^ecc_bit[3]^tmp[1]^tmp[3];
		} else {
			printf("Warning : Non-Support ECC Size!\n");
			exit(0);
		}
		for(n=0; n<ecc_size*8; n++) {
			ecc_bit[n] = CheckBits[n] & 1;
		}
	}

	for (i=0; i<ecc_size*8; i++) {
		if (i%8 == 7) {
#if 1
			//Right-MSB
			ecc_data[i/8] = (ecc_bit[i-7] << 0) | (ecc_bit[i-6] << 1)
				| (ecc_bit[i-5] << 2) | (ecc_bit[i-4] << 3)
				| (ecc_bit[i-3] << 4) | (ecc_bit[i-2] << 5)
				| (ecc_bit[i-1] << 6) | (ecc_bit[i-0] << 7);
#else
			//Left-MSB
			ecc_data[i/8] = (ecc_bit[i-7] << 7) | (ecc_bit[i-6] << 6)
				| (ecc_bit[i-5] << 5) | (ecc_bit[i-4] << 4)
				| (ecc_bit[i-3] << 3) | (ecc_bit[i-2] << 2)
				| (ecc_bit[i-1] << 1) | (ecc_bit[i-0] << 0);
#endif
		}
	}
}


unsigned int s_nTable[256] = {
	0x00000000, 0x04C11DB7, 0x9823B6E, 0xD4326D9, 0x130476DC, 0x17C56B6B, 0x1A864DB2,
	0x1E475005, 0x2608EDB8, 0x22C9F00F, 0x2F8AD6D6, 0x2B4BCB61, 0x350C9B64, 0x31CD86D3,
	0x3C8EA00A, 0x384FBDBD, 0x4C11DB70, 0x48D0C6C7, 0x4593E01E, 0x4152FDA9, 0x5F15ADAC,
	0x5BD4B01B, 0x569796C2, 0x52568B75,  0x6A1936C8, 0x6ED82B7F, 0x639B0DA6, 0x675A1011,
	0x791D4014, 0x7DDC5DA3, 0x709F7B7A, 0x745E66CD,  0x9823B6E0, 0x9CE2AB57, 0x91A18D8E,
	0x95609039,0x8B27C03C, 0x8FE6DD8B, 0x82A5FB52, 0x8664E6E5,  0xBE2B5B58, 0xBAEA46EF,
	0xB7A96036,0xB3687D81, 0xAD2F2D84, 0xA9EE3033, 0xA4AD16EA, 0xA06C0B5D,  0xD4326D90,
	0xD0F37027,0xDDB056FE, 0xD9714B49, 0xC7361B4C, 0xC3F706FB, 0xCEB42022, 0xCA753D95,
	0xF23A8028,0xF6FB9D9F, 0xFBB8BB46, 0xFF79A6F1, 0xE13EF6F4, 0xE5FFEB43, 0xE8BCCD9A,
	0xEC7DD02D,0x34867077, 0x30476DC0, 0x3D044B19, 0x39C556AE, 0x278206AB, 0x23431B1C,
	0x2E003DC5,0x2AC12072,  0x128E9DCF, 0x164F8078, 0x1B0CA6A1, 0x1FCDBB16, 0x18AEB13,
	0x54BF6A4,0x808D07D, 0xCC9CDCA,  0x7897AB07, 0x7C56B6B0, 0x71159069, 0x75D48DDE, 0x6B93DDDB,
	0x6F52C06C, 0x6211E6B5, 0x66D0FB02,  0x5E9F46BF, 0x5A5E5B08, 0x571D7DD1, 0x53DC6066,
	0x4D9B3063, 0x495A2DD4, 0x44190B0D, 0x40D816BA,  0xACA5C697, 0xA864DB20, 0xA527FDF9,
	0xA1E6E04E, 0xBFA1B04B, 0xBB60ADFC, 0xB6238B25, 0xB2E29692,  0x8AAD2B2F, 0x8E6C3698,
	0x832F1041, 0x87EE0DF6, 0x99A95DF3, 0x9D684044, 0x902B669D, 0x94EA7B2A,  0xE0B41DE7,
	0xE4750050, 0xE9362689, 0xEDF73B3E, 0xF3B06B3B, 0xF771768C, 0xFA325055, 0xFEF34DE2,
	0xC6BCF05F, 0xC27DEDE8, 0xCF3ECB31, 0xCBFFD686, 0xD5B88683, 0xD1799B34, 0xDC3ABDED,
	0xD8FBA05A,  0x690CE0EE, 0x6DCDFD59, 0x608EDB80, 0x644FC637, 0x7A089632, 0x7EC98B85,
	0x738AAD5C, 0x774BB0EB,  0x4F040D56, 0x4BC510E1, 0x46863638, 0x42472B8F, 0x5C007B8A,
	0x58C1663D, 0x558240E4, 0x51435D53,  0x251D3B9E, 0x21DC2629, 0x2C9F00F0, 0x285E1D47,
	0x36194D42, 0x32D850F5, 0x3F9B762C, 0x3B5A6B9B,  0x315D626, 0x7D4CB91, 0xA97ED48,
	0xE56F0FF, 0x1011A0FA, 0x14D0BD4D, 0x19939B94, 0x1D528623,  0xF12F560E, 0xF5EE4BB9,
	0xF8AD6D60, 0xFC6C70D7, 0xE22B20D2, 0xE6EA3D65, 0xEBA91BBC, 0xEF68060B,  0xD727BBB6,
	0xD3E6A601, 0xDEA580D8, 0xDA649D6F, 0xC423CD6A, 0xC0E2D0DD, 0xCDA1F604, 0xC960EBB3,
	0xBD3E8D7E, 0xB9FF90C9, 0xB4BCB610, 0xB07DABA7, 0xAE3AFBA2, 0xAAFBE615, 0xA7B8C0CC,
	0xA379DD7B,  0x9B3660C6, 0x9FF77D71, 0x92B45BA8, 0x9675461F, 0x8832161A, 0x8CF30BAD,
	0x81B02D74, 0x857130C3,  0x5D8A9099, 0x594B8D2E, 0x5408ABF7, 0x50C9B640, 0x4E8EE645,
	0x4A4FFBF2, 0x470CDD2B, 0x43CDC09C,  0x7B827D21, 0x7F436096, 0x7200464F, 0x76C15BF8,
	0x68860BFD, 0x6C47164A, 0x61043093, 0x65C52D24,  0x119B4BE9, 0x155A565E, 0x18197087,
	0x1CD86D30, 0x29F3D35, 0x65E2082, 0xB1D065B, 0xFDC1BEC,  0x3793A651, 0x3352BBE6,
	0x3E119D3F, 0x3AD08088, 0x2497D08D, 0x2056CD3A, 0x2D15EBE3, 0x29D4F654,  0xC5A92679,
	0xC1683BCE, 0xCC2B1D17, 0xC8EA00A0, 0xD6AD50A5, 0xD26C4D12, 0xDF2F6BCB, 0xDBEE767C,
	0xE3A1CBC1, 0xE760D676, 0xEA23F0AF, 0xEEE2ED18, 0xF0A5BD1D, 0xF464A0AA, 0xF9278673,
	0xFDE69BC4,  0x89B8FD09, 0x8D79E0BE, 0x803AC667, 0x84FBDBD0, 0x9ABC8BD5, 0x9E7D9662,
	0x933EB0BB, 0x97FFAD0C,  0xAFB010B1, 0xAB710D06, 0xA6322BDF, 0xA2F33668,0xBCB4666D,
	0xB8757BDA, 0xB5365D03, 0xB1F740B4,
};

unsigned int CRC32(unsigned char * pBuffer, unsigned int nstage1_size)
{
	unsigned int nResult = 0xFFFFFFFF;
	while (nstage1_size--) nResult = (nResult << 8) ^ s_nTable[(nResult >> 24) ^ *pBuffer++];
	return nResult;
}

static void display_help(int status)
{
	printf(
"inser stage1 ecc and multi copy.\n"
"\n"
"-h          --help               Display this help and exit\n"
"-i input    --input=input        input file\n"
"-o output   --output=output      output file\n"
"-c count    --count=count        stage1 copy count\n"
"-s start    --start=start        stage1 start exclude BOOT HEAD\n"
"-l length   --length=length      stage1 size\n"
"-p pagesize --pagesize=pagesize  flash page size\n"
"-a          --padding            block size padding\n"
"\n"
	);
	exit(status);
}

// Option variables

static const char *input_file;
static const char *output_file;
static int         copy_count;

static int         stage1_size;
static int         stage1_start;
static int         stage1_copy_count;
static int         page_size;
static int         padflag = 0;

static unsigned long simple_strtoul(const char *cp,char **endp,unsigned int base)
{
	unsigned long result = 0,value;

	if (*cp == '0') {
		cp++;
		if ((*cp == 'x') && isxdigit(cp[1])) {
			base = 16;
			cp++;
		}
		if (!base) {
			base = 8;
		}
	}
	if (!base) {
		base = 10;
	}
	while (isxdigit(*cp) && (value = isdigit(*cp) ? *cp-'0' : (islower(*cp)
					? toupper(*cp) : *cp)-'A'+10) < base) {
		result = result*base + value;
		cp++;
	}
	if (endp)
		*endp = (char *)cp;
	return result;
}

static void process_options(int argc, char * const argv[])
{
	int error = 0;
	bool oob_default = true;

	for (;;) {
		int option_index = 0;
		static const char short_options[] = "hai:o:s:l:c:p:";
		static const struct option long_options[] = {
			{"input",  required_argument, 0, 'i'},
			{"output", required_argument, 0, 'o'},
			{"start",  required_argument, 0, 's'},
			{"lenght",  required_argument, 0, 'l'},
			{"count",  required_argument, 0, 'c'},
			{"pagesize",  required_argument, 0, 'p'},
			{"padding",  no_argument, 0, 'a'},
			{0, 0, 0, 0},
		};

		int c = getopt_long(argc, argv, short_options,
				long_options, &option_index);
		if (c == EOF) {
			break;
		}

		switch (c) {
			case 'i':
				input_file = optarg;
				break;
			case 'o':
				output_file = optarg;
				break;
			case 's':
				stage1_start = simple_strtoul(optarg, NULL, 10);
				break;
			case 'l':
				stage1_size = simple_strtoul(optarg, NULL, 10);
				break;
			case 'c':
				stage1_copy_count = simple_strtoul(optarg, NULL, 10);
				break;
			case 'p':
				page_size = simple_strtoul(optarg, NULL, 10);
				break;
			case 'a':
				padflag = 1;
				break;
			case 'h':
				display_help(EXIT_SUCCESS);
				break;
		}
	}
}



int main(int argc, char * const argv[])
{
	FILE *file_r, *file_w;
	int ret;
	int crc_len = 4;
	unsigned int crc_res;
	unsigned char *buf, *buf_ecc;
	int readlen, writelen;

	unsigned char ff_ecc[28];
	unsigned char ecc[28];
	int i, subpage_count;
	int page_pad_len;
	int block_pad_len;
	unsigned char *buf_block_pad;

	process_options(argc, argv);

	if (stage1_size % 512) {
		printf("stage1 size should be algin 512\n");
		exit(-1);
	}

	file_r = fopen(input_file, "rb");
	if (file_r == 0) {
		printf("file_r open err.\n");
		ret = -1;
		goto exit;
	}

	file_w = fopen(output_file, "wb");
	if (file_w == 0) {
		printf("file_w open err.\n");
		fclose(file_r);
		ret = -1;
		goto exit;
	}

	unsigned char *ff = malloc(512);

	for (i=0; i<512; i++)
		ff[i] = 0xff;

	nand_ecc_cal(ff, 512, 28, ff_ecc);
#if 0
	printf("512 0xff generated ecc:\n");
	for (i=0; i<28; i++)
		printf("0x%02x ", ff_ecc[i]);
	printf("\n");
#endif

	buf = malloc(stage1_size);
	memset(buf, 0xff, stage1_size);
	buf_ecc = malloc(stage1_size * 16);
	memset(buf_ecc, 0xff, stage1_size * 16);

	ret = fread(buf, 1, stage1_size, file_r);
	if (ret != stage1_size) {
		printf("read failed %d.\n", ret);
		ret = -1;
		goto close;
	}

	crc_res = CRC32(buf + stage1_start, stage1_size - stage1_start - crc_len);
	*(unsigned int *)(buf + stage1_size - crc_len) = crc_res;

	readlen = 0;
	writelen = 0;
	subpage_count = 0;
	while (readlen < stage1_size) {
		nand_ecc_cal(&buf[readlen], 512, 28, ecc);
		memcpy(&buf_ecc[writelen], &buf[readlen], 512);
		readlen += 512;
		writelen += 512;
		for (i = 0; i < 28; i++) {
			ecc[i] = ~(ecc[i] ^ ff_ecc[i]);
		}
		memcpy(&buf_ecc[writelen], ecc, 28);
		writelen += 28;
		subpage_count++;
		/* 每页只放2个512数据 */
		if (subpage_count == 2) {
			subpage_count = 0;
			page_pad_len = page_size - writelen % page_size;
			writelen += page_pad_len;
		}
	}

	if(padflag == 1) {
		block_pad_len = roundup(writelen, page_size*64) - writelen;
		buf_block_pad = malloc(block_pad_len);
		memset(buf_block_pad, 0x00, block_pad_len);
	}

	for(i = 0; i < stage1_copy_count; i++) {
		ret = fwrite(buf_ecc, 1, writelen, file_w);
		if (ret != writelen) {
			printf("make %s from %s failed .\n", output_file, input_file);
			ret = -1;
			goto close;
		}
		if(padflag == 1) {
			ret = fwrite(buf_block_pad, 1, block_pad_len, file_w);
			if (ret != block_pad_len) {
				printf("make %s from %s failed 2.\n", output_file, input_file);
				ret = -1;
				goto close;
			}
		}
	}

	/* write whole loader.bin */
	fseek( file_r, stage1_size, SEEK_SET);

	while(1) {
		ret = fread(buf, 1, stage1_size, file_r);
		fwrite(buf, 1, ret, file_w);
		if (ret < stage1_size)
			break;
	}
	ret = 0;

close:
	fclose(file_r);
	fclose(file_w);
exit:
	free(ff);
	free(buf);
	free(buf_ecc);
	if(padflag)
		free(buf_block_pad);
	exit(ret);
}

