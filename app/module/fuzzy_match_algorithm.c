// Smith-Waterman algorithm
/*
 * The Smith-Waterman algorithm is a well-known algorithm for performing local sequence alignment; that is, for determining similar regions
 * between two strings or biological sequences. It is particularly used in bioinformatics to align nucleotide or protein sequences. The
 * algorithm is a variation of the Needleman-Wunsch algorithm, which is used for global sequence alignment.
 * In the context of the Smith-Waterman algorithm, the constants MATCH_SCORE, MISMATCH_SCORE, and GAP_PENALTY have the following meanings and
 * roles:
 *    MATCH_SCORE: This is the score given when two characters in the strings being compared are the same (a match). A higher match score
 *                 increases the overall alignment score when characters from both strings align perfectly. In the provided code, the match
 *                 score is set to 3.
 *    MISMATCH_SCORE: This is the penalty (negative score) applied when two characters in the strings being compared do not match. A mismatch
 *                 penalty reduces the overall alignment score, reflecting the fact that the two strings are less similar at that position.
 *                 In the provided code, the mismatch score is set to -3.
 *    GAP_PENALTY: This is the penalty (negative score) applied when aligning a character in one string with a gap in the other string. This
 *                 situation arises when one string has an extra character that does not align with any character in the other string.
 *                 Introducing gaps is necessary to allow for insertions and deletions when aligning the strings. In the provided code, the
 *                 gap penalty is set to -2.
 * The algorithm works by constructing a matrix where each cell represents the maximum score of aligning substrings ending at those positions.
 * The score for each cell is calculated by taking the maximum of the following:
 *    1.The score of the diagonal cell (top-left) plus MATCH_SCORE if the characters match or MISMATCH_SCORE if they don't (representing a
 *      character match/mismatch).
 *    2.The score of the cell above plus GAP_PENALTY (representing a deletion).
 *    3.The score of the cell to the left plus GAP_PENALTY (representing an insertion).
 *    4.Zero, which represents the option of starting a new alignment at this position (since the Smith-Waterman algorithm is for local
 *      alignments, it allows for alignments to begin and end anywhere within the strings).
 * The algorithm keeps track of the maximum score found in any cell (max_score), which represents the highest scoring local alignment between
 * the two strings. After filling the matrix, the max_score is returned as the result of the function, indicating the best local alignment score
 * between the input strings s1 and s2.
 * */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

//#define SMITH_WATERMAN_ALG
#define LEVENSHTEIN_DISTANCE_ALG

#ifdef SMITH_WATERMAN_ALG
#define MATCH_SCORE 3
#define MISMATCH_SCORE -3
#define GAP_PENALTY -2

static int smith_waterman(char *s1, char *s2) {
    int len1 = strlen(s1);
    int len2 = strlen(s2);
    int i, j;
    int score, match, delete, insert;

    int *prev_H = (int *)calloc(len2 + 1, sizeof(int));
    int *curr_H = (int *)calloc(len2 + 1, sizeof(int));
    int max_score = 0;

    for (i = 1; i <= len1; i++) {
        for (j = 1; j <= len2; j++) {
            match = prev_H[j - 1] + (s1[i - 1] == s2[j - 1] ? MATCH_SCORE : MISMATCH_SCORE);
            delete = prev_H[j] + GAP_PENALTY;
            insert = curr_H[j - 1] + GAP_PENALTY;
            score = match > delete ? match : delete;
            score = score > insert ? score : insert;
            score = score > 0 ? score : 0;
            curr_H[j] = score;

            if (score > max_score) {
                max_score = score;
            }
        }
        int *temp = prev_H;
        prev_H = curr_H;
        curr_H = temp;
        memset(curr_H, 0, (len2 + 1) * sizeof(int));
    }
    free(prev_H);
    free(curr_H);

    // calcurate percent
    int max_len = (len1>len2)?len1:len2;
    max_score = (max_score*100)/(max_len*MATCH_SCORE);
    return max_score;
}

#endif

#ifdef LEVENSHTEIN_DISTANCE_ALG
// 计算最小值的宏
#define MIN(a, b) ((a) < (b) ? (a) : (b))
// 计算最大值的宏
#define MAX(a, b) ((a) > (b) ? (a) : (b))

// 计算两个字符串的Levenshtein距离，按字符串较大长度作为基数，计算编辑距离的百分比（动态规划版本）
int levenshtein_distance(const char *s, const char *t) {
    int i, j;
    int len_s = strlen(s);
    int len_t = strlen(t);
    int **d = (int **)malloc((len_s + 1) * sizeof(int *));
    for (i = 0; i <= len_s; i++) {
        d[i] = (int *)malloc((len_t + 1) * sizeof(int));
    }

    for (i = 0; i <= len_s; i++) {
        d[i][0] = i;
    }
    for (j = 0; j <= len_t; j++) {
        d[0][j] = j;
    }

    for (i = 1; i <= len_s; i++) {
        for (j = 1; j <= len_t; j++) {
            int cost = (s[i - 1] == t[j - 1]) ? 0 : 1;
            d[i][j] = MIN(MIN(d[i - 1][j] + 1, d[i][j - 1] + 1), d[i - 1][j - 1] + cost);
        }
    }

    int dist = d[len_s][len_t];

    for (i = 0; i <= len_s; i++) {
        free(d[i]);
    }
    free(d);

    // calcurate percent
    int max_len = MAX(len_s, len_t);
    return (1 - (((double)dist)/max_len)) * 100;
}
#endif

#include "gxcore.h"

//#define ALG_DEBUG
//
#ifdef ALG_DEBUG
#define ALG_PRINT(...)       printf(__VA_ARGS__)
#else
#define ALG_PRINT(...)       (void)0
#endif

typedef int (*fuzzy_algo)(const char *src, const char *dest);
#if defined(LEVENSHTEIN_DISTANCE_ALG)
static fuzzy_algo thiz_alog = levenshtein_distance;
#elif defined(SMITH_WATERMAN_ALG)
static fuzzy_algo thiz_alog = smith_waterma;
#endif

typedef struct {
    char *str;
    int score;
    int index;
} StringScore;

static void to_lowercase(char *str) {
    int i = 0;
    for (i = 0; str[i]; i++) {
        str[i] = tolower(str[i]);
    }
}

static int compare_string_scores(const void *a, const void *b) {
    StringScore *ss1 = (StringScore *)a;
    StringScore *ss2 = (StringScore *)b;
    return (ss2->score - ss1->score);
}

// 完成指定字符串和字符串数据库进行模糊匹配检索，函数提供指定数量的数据结果返回和指定评分过滤的功能。以函数形参进行功能说明：从“str2_count”个字符串
// 指针数组“str2_list”中模糊化匹配“str1”字符串，根据分数过滤参数“passing_score”筛选匹配结果，最多返回“index_count”个符合条件的结果到“index”内存中。
// index中存储的是按匹配度从高到低排序过的符合条件的字符串在“str2_list”指针数组中的位置下标索引。
// params:
//           str1[in]: 输入字符串指针
//      str2_list[in]: 待比较的字符串数组指针
//     str2_count[in]: 待比较的字符串个数
//         index[out]: 按匹配度从高到低排序结果，str2_list中的字符串索引位置
//index_count[in/out]: 输入index内存中字符串保存个数，也是当前函数允许的最大排序结果返回个数。函数执行返回会修改成当前匹配排序结果的个数
//  passing_score[in]: 匹配分数过滤门限(0~100, 实际是百分比)，大于等于该门限的字符串保留到结果排序。当该参数是“-1”的时候就是不关心该门限控制
// return:
//                 -1: 函数执行失败
//                  0: 函数执行成功
int app_fuzzy_strings_matching(char *str1, char **str2_list, unsigned int str2_count, int *index, unsigned int *index_count, int passing_score) {
#define TEMP_BUF_SIZE 64
    StringScore *pscores = NULL;
#ifdef ALG_DEBUG
    uint64_t start = 0, end = 0;
    int cpu_time_used = 0;
#endif
    int i = 0;
    int j = 0;
    int value = 0;
    int str_len = 0;
    int ret = 0;
    char buf[TEMP_BUF_SIZE] = {0};
    char temp[TEMP_BUF_SIZE] = {0};

    if((NULL == str1) || (0 == strlen(str1)) || (NULL == str2_list) || (0 == str2_count)
            || (NULL == index) || (NULL == index_count) || (0 ==index_count)){
        return -1;
    }

#ifdef ALG_DEBUG
    start = GxCore_TickStart(0);// ms
#endif
    str_len = strlen(str1);
    if(TEMP_BUF_SIZE <= str_len){
        return -1;
    }
    pscores = GxCore_Malloc(str2_count*sizeof(StringScore));
    if(NULL == pscores){
        return -1;
    }

    memset(temp, 0, sizeof(buf));
    memcpy(temp, str1, str_len);

    to_lowercase(temp);

    for (i = 0; i < str2_count; i++) {
        memset(buf, 0, sizeof(buf));
        str_len = strlen(str2_list[i]);
        if(TEMP_BUF_SIZE <= str_len){
            ALG_PRINT("\nERROR, %s, %d\n", __func__, __LINE__);
            break;
        }
        memcpy(buf, str2_list[i], str_len);
        to_lowercase(buf);
        if(thiz_alog)
        {
            value = thiz_alog(temp,buf);
        }
        if((-1 == passing_score)
                || (passing_score <= value))
        {
            pscores[j].str = str2_list[i];
            pscores[j].index = i;
            pscores[j].score = value;
            j++;
        }
    }

    if(0 == j){
        *index_count = 0;
#ifdef ALG_DEBUG
        ALG_PRINT("\nERROR, %s, %d\n", __func__, __LINE__);
        end = GxCore_TickStart(0);// ms
        cpu_time_used = (int)((end - start));
#endif
        goto exit;
    }

    qsort(pscores, j, sizeof(StringScore), compare_string_scores);

    value = ((*index_count) > j)?j:(*index_count);
    *index_count = value;
    for( i = 0; i < value; i++)
    {
        index[i] = pscores[i].index;
    }

#ifdef ALG_DEBUG
    end = GxCore_TickStart(0);// ms
    cpu_time_used = (int)((end - start));
#endif
exit:
#if 0
    printf("Input(%s), Match result:\n", str1);
    for (i = 0; i < str2_count; i++) {
        printf("Score: %d, String: %s\n", pscores[i].score, pscores[i].str);
    }
#else
    ALG_PRINT("Input(%s), DataTotal: %d, PassCount(>=%d): %d\n", str1, str2_count, passing_score, j);
#endif
    ALG_PRINT("Execution time: %d milliseconds\n", cpu_time_used);
    GxCore_Free(pscores);
    return ret;
}

