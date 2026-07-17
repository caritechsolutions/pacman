#include <stdio.h>

unsigned short FILE_Read16(const FILE *pFile)
{
	unsigned char pData[2] = { 0 };
	unsigned short Value = 0;
	int Ret = 0;
	Ret = fread(pData, 2, 1, (FILE *) pFile);
	if (0 == Ret) {
		return 0;
	}
	Value = *pData;
	Value |= (unsigned short)(*(pData + 1) << 8);

	//fseek(pFile, 2, SEEK_CUR);
	return Value;
}

unsigned int FILE_Read32(const FILE *pFile)
{
	unsigned char pData[4] = { 0 };
	unsigned int Value;
	int Ret = 0;

	Ret = fread(pData, 4, 1, (FILE *) pFile);
	if (0 == Ret)
		return 0;

	Value = *pData;
	Value |= (*(pData + 1) << 8);
	Value |= ((unsigned int)*(pData + 2) << 16);
	Value |= ((unsigned int)*(pData + 3) << 24);

	return Value;
}

unsigned int FILE_Seek(const FILE *pFile, int pos)
{
	return fseek((FILE *) pFile, pos, SEEK_CUR);
}
