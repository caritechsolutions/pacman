#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "gxcore.h"
#include "app_config.h"

#ifdef MEMORY_STABILITY_TEST_SUPPORT
#if defined(MEMORY_TEST_DDR2_64MB)
    #define ddr2_64MB
    #define MEMORY_TEST_BYTES 0x280000
#elif defined(MEMORY_TEST_DDR2_128MB)
    #define ddr2_128MB
    #define MEMORY_TEST_BYTES 0x500000
#elif defined(MEMORY_TEST_DDR3_128MB)
    #define ddr3_128MB
    #define MEMORY_TEST_BYTES 0x500000
#elif defined(MEMORY_TEST_DDR3_256MB)
    #define ddr3_256MB
    #define MEMORY_TEST_BYTES 0x500000
#elif defined(MEMORY_TEST_DDR3_512MB)
    #define ddr3_512MB
    #define MEMORY_TEST_BYTES 0x500000
#endif

static char *memory_test_buf_a;
static char *memory_test_buf_b;
static char thiz_test_is_working = -1;

#define TEST_NARROW_WRITES
extern int printf(const char *fmt, ...);

typedef unsigned long ul;
typedef unsigned long long ull;
typedef unsigned long volatile ulv;
typedef unsigned char volatile u8v;
typedef unsigned short volatile u16v;

#define memtest_printf(fmt, ...) printf(fmt, ##__VA_ARGS__)

//#ifdef CONFIG_ARCH_ARM
#define rand32() ((unsigned int) rand() | ( (unsigned int) rand() << 16))

#ifdef ULONG_MAX
#undef ULONG_MAX
#endif
#define ULONG_MAX  4294967295UL

#if (ULONG_MAX == 4294967295UL)
    #define rand_ul() rand32()
    #define UL_ONEBITS 0xffffffff
    #define UL_LEN 32
    #define CHECKERBOARD1 0x55555555
    #define CHECKERBOARD2 0xaaaaaaaa
    #define UL_BYTE(x) ((x | x << 8 | x << 16 | x << 24))
#elif (ULONG_MAX == 18446744073709551615ULL)
    #define rand64() (((ul) rand32()) << 32 | ((ul) rand32()))
    #define rand_ul() rand64()
    #define UL_ONEBITS 0xffffffffffffffffUL
    #define UL_LEN 64
    #define CHECKERBOARD1 0x5555555555555555
    #define CHECKERBOARD2 0xaaaaaaaaaaaaaaaa
    #define UL_BYTE(x) (((ul)x | (ul)x<<8 | (ul)x<<16 | (ul)x<<24 | (ul)x<<32 | (ul)x<<40 | (ul)x<<48 | (ul)x<<56))
#else
    #error long on this platform is not 32 or 64 bits
#endif

struct test {
    char *name;
    int (*fp)(ulv* bufa, ulv* bufb, size_t count);
};

union {
    unsigned char bytes[UL_LEN/8];
    ul val;
} mword8;

union {
    unsigned short u16s[UL_LEN/16];
    ul val;
} mword16;

char progress[] = "-\\|/";
#define PROGRESSLEN 4
#define PROGRESSOFTEN 2500
#define ONE 0x00000001L

int err_flag = 1;

int compare_regions(ulv *bufa, ulv *bufb, size_t count) {
    int r = 0;
    int addr_a;
    int col_a;
    int bnk_a;
    int row_a;
    size_t i;
    ulv *p1 = bufa;
    ulv *p2 = bufb;
    for (i = 0; i < count; i++, p1++, p2++) {
        if (*p1 != *p2) {
		addr_a = ((ul) (i * sizeof(ul))) + (unsigned int)&memory_test_buf_a[0];
                #ifdef ddr2_64MB
                //row[25-13], bank[12-11], col[10-1], dp[0]
                  row_a = (( (addr_a<<6)>>19  ) &0x1fff);
                  bnk_a = (( (addr_a<<19)>>30 ) &0x3);
                  col_a = (( (addr_a<<21)>>22 ) &0x3ff);
                #endif

                #ifdef ddr2_128MB
                //row[26-14], bank[13-11], col[10-1], dp[0]
                  row_a = (( (addr_a<<5)>>19  ) &0x1fff);
                  bnk_a = (( (addr_a<<18)>>29 ) &0x7);
                  col_a = (( (addr_a<<21)>>22 ) &0x3ff);
                #endif

		        #ifdef ddr3_128MB
                //row[26-14], bank[13-11], col[10-1], dp[0]
                  row_a = (( (addr_a<<5)>>19  ) &0x1fff);
                  bnk_a = (( (addr_a<<18)>>29 ) &0x7);
                  col_a = (( (addr_a<<21)>>22 ) &0x3ff);
                #endif

                #ifdef ddr3_256MB
                //row[27-14], bank[13-11], col[10-1], dp[0]
                  row_a = (( (addr_a<<4)>>18  ) &0x3fff);
                  bnk_a = (( (addr_a<<18)>>29 ) &0x7);
                  col_a = (( (addr_a<<21)>>22 ) &0x3ff);
                #endif

                #ifdef ddr3_512MB
                //row[28-14], bank[13-11], col[10-1], dp[0]
                  row_a = (( (addr_a<<3)>>17  ) &0x7fff);
                  bnk_a = (( (addr_a<<18)>>29 ) &0x7);
                  col_a = (( (addr_a<<21)>>22 ) &0x3ff);
                #endif

		memtest_printf("FAILURE: 0x%08lx != 0x%08lx, xor_result=0x%0*lx, addr_a: bank=0x%x,row=0x%x,col=0x%x;\n",
                (ul) *p1, (ul) *p2, 8, ((ul) *p1)^((ul) *p2), bnk_a&0x00000007, row_a&0x0000ffff, col_a&0x00000fff);
	    err_flag = 1;
//            memtest_printf("Skip \n");
            r = -1;
        }
    }
    return r;
}

int test_random_value(ulv *bufa, ulv *bufb, size_t count) {
    ulv *p1 = bufa;
    ulv *p2 = bufb;
    size_t i;

    for (i = 0; i < count; i++) {
        *p1++ = *p2++ = rand_ul();
    }
    return compare_regions(bufa, bufb, count);
}

int test_xor_comparison(ulv *bufa, ulv *bufb, size_t count) {
    ulv *p1 = bufa;
    ulv *p2 = bufb;
    size_t i;
    ul q = rand_ul();

    for (i = 0; i < count; i++) {
        *p1++ ^= q;
        *p2++ ^= q;
    }
    return compare_regions(bufa, bufb, count);
}

int test_sub_comparison(ulv *bufa, ulv *bufb, size_t count) {
    ulv *p1 = bufa;
    ulv *p2 = bufb;
    size_t i;
    ul q = rand_ul();
    for (i = 0; i < count; i++) {
        *p1++ -= q;
        *p2++ -= q;
    }
    return compare_regions(bufa, bufb, count);
}

int test_mul_comparison(ulv *bufa, ulv *bufb, size_t count) {
    ulv *p1 = bufa;
    ulv *p2 = bufb;
    size_t i;
    ul q = rand_ul();

    for (i = 0; i < count; i++) {
        *p1++ *= q;
        *p2++ *= q;
    }
    return compare_regions(bufa, bufb, count);
}

int test_div_comparison(ulv *bufa, ulv *bufb, size_t count) {
    ulv *p1 = bufa;
    ulv *p2 = bufb;
    size_t i;
    ul q = rand_ul();

    for (i = 0; i < count; i++) {
        if (!q) {
            q++;
        }
        *p1++ /= q;
        *p2++ /= q;
    }
    return compare_regions(bufa, bufb, count);
}

int test_or_comparison(ulv *bufa, ulv *bufb, size_t count) {
    ulv *p1 = bufa;
    ulv *p2 = bufb;
    size_t i;
    ul q = rand_ul();

    for (i = 0; i < count; i++) {
        *p1++ |= q;
        *p2++ |= q;
    }
    return compare_regions(bufa, bufb, count);
}

int test_and_comparison(ulv *bufa, ulv *bufb, size_t count) {
    ulv *p1 = bufa;
    ulv *p2 = bufb;
    size_t i;
    ul q = rand_ul();

    for (i = 0; i < count; i++) {
        *p1++ &= q;
        *p2++ &= q;
    }
    return compare_regions(bufa, bufb, count);
}

int test_seqinc_comparison(ulv *bufa, ulv *bufb, size_t count) {
    ulv *p1 = bufa;
    ulv *p2 = bufb;
    size_t i;
    ul q = rand_ul();

    for (i = 0; i < count; i++) {
        *p1++ = *p2++ = (i + q);
    }
    return compare_regions(bufa, bufb, count);
}

int test_solidbits_comparison(ulv *bufa, ulv *bufb, size_t count) {
    ulv *p1 = bufa;
    ulv *p2 = bufb;
    unsigned int j;
    ul q;
    size_t i;

    for (j = 0; j < 64; j++) {
        q = (j % 2) == 0 ? UL_ONEBITS : 0;
        p1 = (ulv *) bufa;
        p2 = (ulv *) bufb;
        for (i = 0; i < count; i++) {
            *p1++ = *p2++ = (i % 2) == 0 ? q : ~q;
        }
        if (compare_regions(bufa, bufb, count)) {
            memtest_printf("Skip \n");
            return -1;
        }
    }
    return 0;
}

int test_checkerboard_comparison(ulv *bufa, ulv *bufb, size_t count) {
    ulv *p1 = bufa;
    ulv *p2 = bufb;
    unsigned int j;
    ul q;
    size_t i;

    for (j = 0; j < 64; j++) {
        q = (j % 2) == 0 ? CHECKERBOARD1 : CHECKERBOARD2;
        p1 = (ulv *) bufa;
        p2 = (ulv *) bufb;
        for (i = 0; i < count; i++) {
            *p1++ = *p2++ = (i % 2) == 0 ? q : ~q;
        }
        if (compare_regions(bufa, bufb, count)) {
            memtest_printf("Skip \n");
            return -1;
        }
    }
    return 0;
}

int test_blockseq_comparison(ulv *bufa, ulv *bufb, size_t count) {
    ulv *p1 = bufa;
    ulv *p2 = bufb;
    unsigned int j;
    size_t i;

    for (j = 0; j < 256; j++) {
        p1 = (ulv *) bufa;
        p2 = (ulv *) bufb;
        for (i = 0; i < count; i++) {
            *p1++ = *p2++ = (ul) UL_BYTE(j);
        }
        if (compare_regions(bufa, bufb, count)) {
            memtest_printf("Skip \n");
            return -1;
        }
    }
    return 0;
}

int test_walkbits0_comparison(ulv *bufa, ulv *bufb, size_t count) {
    ulv *p1 = bufa;
    ulv *p2 = bufb;
    unsigned int j;
    size_t i;

    for (j = 0; j < UL_LEN * 2; j++) {
        p1 = (ulv *) bufa;
        p2 = (ulv *) bufb;
        for (i = 0; i < count; i++) {
            if (j < UL_LEN) { /* Walk it up. */
                *p1++ = *p2++ = ONE << j;
            } else { /* Walk it back down. */
                *p1++ = *p2++ = ONE << (UL_LEN * 2 - j - 1);
            }
        }
        if (compare_regions(bufa, bufb, count)) {
            memtest_printf("Skip \n");
            return -1;
        }
    }
    return 0;
}

int test_walkbits1_comparison(ulv *bufa, ulv *bufb, size_t count) {
    ulv *p1 = bufa;
    ulv *p2 = bufb;
    unsigned int j;
    size_t i;

    for (j = 0; j < UL_LEN * 2; j++) {
        p1 = (ulv *) bufa;
        p2 = (ulv *) bufb;
        for (i = 0; i < count; i++) {
            if (j < UL_LEN) { /* Walk it up. */
                *p1++ = *p2++ = UL_ONEBITS ^ (ONE << j);
            } else { /* Walk it back down. */
                *p1++ = *p2++ = UL_ONEBITS ^ (ONE << (UL_LEN * 2 - j - 1));
            }
        }
        if (compare_regions(bufa, bufb, count)) {
            memtest_printf("Skip \n");
            return -1;
        }
    }
    return 0;
}

int test_bitspread_comparison(ulv *bufa, ulv *bufb, size_t count) {
    ulv *p1 = bufa;
    ulv *p2 = bufb;
    unsigned int j;
    size_t i;

    for (j = 0; j < UL_LEN * 2; j++) {
        p1 = (ulv *) bufa;
        p2 = (ulv *) bufb;
        for (i = 0; i < count; i++) {
            if (j < UL_LEN) { /* Walk it up. */
                *p1++ = *p2++ = (i % 2 == 0)
                    ? (ONE << j) | (ONE << (j + 2))
                    : UL_ONEBITS ^ ((ONE << j)
                                    | (ONE << (j + 2)));
            } else { /* Walk it back down. */
                *p1++ = *p2++ = (i % 2 == 0)
                    ? (ONE << (UL_LEN * 2 - 1 - j)) | (ONE << (UL_LEN * 2 + 1 - j))
                    : UL_ONEBITS ^ (ONE << (UL_LEN * 2 - 1 - j)
                                    | (ONE << (UL_LEN * 2 + 1 - j)));
            }
        }
        if (compare_regions(bufa, bufb, count)) {
            memtest_printf("Skip \n");
            return -1;
        }
    }
    return 0;
}

int test_bitflip_comparison(ulv *bufa, ulv *bufb, size_t count) {
    ulv *p1 = bufa;
    ulv *p2 = bufb;
    unsigned int j, k;
    ul q;
    size_t i;

    for (k = 0; k < UL_LEN; k++) {
        q = ONE << k;
        for (j = 0; j < 8; j++) {
            q = ~q;
            p1 = (ulv *) bufa;
            p2 = (ulv *) bufb;
            for (i = 0; i < count; i++) {
                *p1++ = *p2++ = (i % 2) == 0 ? q : ~q;
            }
            if (compare_regions(bufa, bufb, count)) {
		memtest_printf("Skip \n");
                return -1;
            }
        }
    }
    return 0;
}

#ifdef TEST_NARROW_WRITES
int test_8bit_wide_random(ulv* bufa, ulv* bufb, size_t count) {
    u8v *p1, *t;
    ulv *p2;
    int attempt;
    unsigned int b, j = 0;
    size_t i;

    printf(" ");
    for (attempt = 0; attempt < 2;  attempt++) {
        if (attempt & 1) {
            p1 = (u8v *) bufa;
            p2 = bufb;
        } else {
            p1 = (u8v *) bufb;
            p2 = bufa;
        }
        for (i = 0; i < count; i++) {
            t = mword8.bytes;
            *p2++ = mword8.val = rand_ul();
            for (b=0; b < UL_LEN/8; b++) {
                *p1++ = *t++;
            }
            if (!(i % PROGRESSOFTEN)) {
                printf("\b");
                printf("%c", progress[++j % PROGRESSLEN]);
            }
        }
        if (compare_regions(bufa, bufb, count)) {
            return -1;
        }
    }
    printf("\b \b");
    return 0;
}

int test_16bit_wide_random(ulv* bufa, ulv* bufb, size_t count) {
    u16v *p1, *t;
    ulv *p2;
    int attempt;
    unsigned int b, j = 0;
    size_t i;

    printf(" ");
    for (attempt = 0; attempt < 2; attempt++) {
        if (attempt & 1) {
            p1 = (u16v *) bufa;
            p2 = bufb;
        } else {
            p1 = (u16v *) bufb;
            p2 = bufa;
        }
        for (i = 0; i < count; i++) {
            t = mword16.u16s;
            *p2++ = mword16.val = rand_ul();
            for (b = 0; b < UL_LEN/16; b++) {
                *p1++ = *t++;
            }
            if (!(i % PROGRESSOFTEN)) {
                printf("\b");
                printf("%c", progress[++j % PROGRESSLEN]);
            }
        }
        if (compare_regions(bufa, bufb, count)) {
            return -1;
        }
    }
    printf("\b \b");
    return 0;
}
#endif


struct test tests[] = {
    { "Random Value"          , test_random_value }            ,
    { "Compare XOR"           , test_xor_comparison }          ,
    { "Compare SUB"           , test_sub_comparison }          ,
    { "Compare MUL"           , test_mul_comparison }          ,
    { "Compare DIV"           , test_div_comparison }          ,
    { "Compare OR"            , test_or_comparison }           ,
    { "Compare AND"           , test_and_comparison }          ,
    { "Sequential Increment"  , test_seqinc_comparison }       ,
    { "Solid Bits"            , test_solidbits_comparison }    ,
    { "Block Sequential"      , test_blockseq_comparison }     ,
    { "Checkerboard"          , test_checkerboard_comparison } ,
    { "Bit Spread"            , test_bitspread_comparison }    ,
    { "Bit Flip"              , test_bitflip_comparison }      ,
    { "Walking Ones"          , test_walkbits1_comparison }    ,
    { "Walking Zeroes"        , test_walkbits0_comparison }    ,
//    #ifdef TEST_NARROW_WRITES
//    { "8-bit Writes"          , test_8bit_wide_random }        ,
//    { "16-bit Writes"         , test_16bit_wide_random }       ,
//    #endif
//    { NULL, NULL }
};

typedef enum _MessageTypeEnum{
    MT_INFO = 0,
    MT_ERROR,
    MT_NONE,
}MessageTypeEnum;

static char thiz_message_buf[1024] = {0};
static unsigned int thiz_err_count = 0;
static void mt_message(char *str, MessageTypeEnum type)
{
    if(NULL == str)
    {
        printf("\n[MT Error] Parameters error!!!(%s)\n", __func__);
        return ;
    }
    // normal output
    printf("%s", str);
#ifdef MEMORY_TEST_OUTPUT_OSD
{
    int ret = 0;
extern int app_message_to_osd(char *str, unsigned int error_flag, unsigned int err_count);
    ret = app_message_to_osd(str, ((type == MT_ERROR)?1:0), ((type == MT_ERROR)?(++thiz_err_count):thiz_err_count));
    if(-2 == ret)
    {
        printf("\n[MT Info] OSD dialog is not exist!\n");
    }
    else if(ret != 0)
    {
        printf("\n[MT Error] Parameters error!!!\n");
    }
}
#endif
}

static void memtest(void* arg)
{
	int i;
	int error_flag = 0;
#ifdef MEMORY_TEST_REPEAT
    char *TestMode = "Repeat";
    unsigned int times = 1;
#else
    char *TestMode = "Once";
#endif
#ifdef MEMORY_TEST_OUTPUT_OSD
    char *Outputmode = "OSD&UART";
#else
    char *Outputmode = "UART";
#endif
    char *blank_8 = "        ";
    char *blank_7 = "       ";

    thiz_err_count = 0;
    GxCore_ThreadDetach();
#ifdef MEMORY_TEST_REPEAT
Start:
#endif
    //printf("\n*********************************************************\n");
    //printf("                  Memory Test(%d)                        \n", times++);
    //printf("    Size: 0x%X, Mode: %s, Output:%s    \n",MEMORY_TEST_BYTES,TestMode, Outputmode);
    //printf("*********************************************************\n");
    printf("\n");
    mt_message("*********************************************************\n", MT_INFO);
    memset(thiz_message_buf,0,sizeof(thiz_message_buf));
#ifdef MEMORY_TEST_REPEAT
    sprintf(thiz_message_buf, "             Memory Test(%d)                   \n", times++);
#else
    sprintf(thiz_message_buf, "             Memory Test                   \n");
#endif
    mt_message(thiz_message_buf, MT_INFO);
    memset(thiz_message_buf,0,sizeof(thiz_message_buf));
    sprintf(thiz_message_buf, "    0x%X, %s, %s    \n", MEMORY_TEST_BYTES,TestMode, Outputmode);
    mt_message(thiz_message_buf, MT_INFO);
    mt_message("********************************************************\n", MT_INFO);



	memory_test_buf_a = GxCore_Malloc(MEMORY_TEST_BYTES);
	memory_test_buf_b = GxCore_Malloc(MEMORY_TEST_BYTES);
	if (memory_test_buf_b == 0 || memory_test_buf_b == 0) {
		//printf("[MemoryTest Error] Malloc 0x%x failed!!\n", MEMORY_TEST_BYTES);
        memset(thiz_message_buf,0,sizeof(thiz_message_buf));
        sprintf(thiz_message_buf, "[MT Error] Malloc 0x%x failed!!\n", MEMORY_TEST_BYTES);
        mt_message(thiz_message_buf, MT_ERROR);

		if (memory_test_buf_a != NULL)
			GxCore_Free(memory_test_buf_a);
		if (memory_test_buf_b != NULL)
			GxCore_Free(memory_test_buf_b);
        memory_test_buf_a = NULL;
        memory_test_buf_b = NULL;
		return;
	}

    //printf("[MemoryTest Info] buf_a=%p, buf_b=%p\n", memory_test_buf_a, memory_test_buf_b);
    memset(thiz_message_buf,0,sizeof(thiz_message_buf));
    sprintf(thiz_message_buf, "[MT Info] bufa=%p, bufb=%p\n", memory_test_buf_a, memory_test_buf_b);
    mt_message(thiz_message_buf, MT_INFO);

    while(1) {
		for(i = 0; i < sizeof(tests)/sizeof(struct test); i++)
		{
			//printf("Test %d: %s test start\n", i, tests[i].name);
            memset(thiz_message_buf,0,sizeof(thiz_message_buf));
            sprintf(thiz_message_buf, "Test %d: %s test start\n", i, tests[i].name);
            mt_message(thiz_message_buf, MT_INFO);


            if (tests[i].fp((ulv *)memory_test_buf_a, (ulv *)memory_test_buf_b, MEMORY_TEST_BYTES/sizeof(ulv)) != 0)
				error_flag = 1;
            if(0 == error_flag)
            {
			    //printf("%s %s test end, successfully\n", ((i>9)?blank_8:blank_7),tests[i].name);
                memset(thiz_message_buf,0,sizeof(thiz_message_buf));
                sprintf(thiz_message_buf, "%s %s test end, successfully!\n", ((i>9)?blank_8:blank_7),tests[i].name);
                mt_message(thiz_message_buf, MT_INFO);

            }
            else
            {
			    //printf("%s %s test end, failed\n",  ((i>9)?blank_8:blank_7), tests[i].name);
                memset(thiz_message_buf,0,sizeof(thiz_message_buf));
                sprintf(thiz_message_buf, "%s %s test end, failed!\n", ((i>9)?blank_8:blank_7),tests[i].name);
                mt_message(thiz_message_buf, MT_ERROR);
            }
            GxCore_ThreadDelay(100);
		}
        break;
	}
    if(memory_test_buf_a)
    {
        GxCore_Free(memory_test_buf_a);
        memory_test_buf_a = NULL;
    }
    if(memory_test_buf_b)
    {
        GxCore_Free(memory_test_buf_b);
        memory_test_buf_b = NULL;
    }
    //printf("[MemoryTest Info] Memory test finish...\n");
    mt_message("[MT Info] Memory test finish!!!\n", MT_INFO);

#ifdef MEMORY_TEST_REPEAT
    goto Start;
#endif
    thiz_test_is_working = 0;
}

void app_memory_test_start(void)
{
    int memtest_thread = -1;

    if(1 != thiz_test_is_working)
    {
        thiz_test_is_working = 1;
        GxCore_ThreadCreate("memtest", &memtest_thread, memtest, NULL, 100*1024, GXOS_DEFAULT_PRIORITY + 1);
    }
}
#endif
