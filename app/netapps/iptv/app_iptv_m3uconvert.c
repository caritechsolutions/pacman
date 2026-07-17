#include<stdio.h>
#include<string.h>
#include<stdlib.h>
#include "app.h"

#undef DEBUG_PRINTF

#define NUM_LIMIT         (2000)
#define BUF_PER_LINE_SIZE (2048)
#define BUF_TYPE_STRING     (64)
#define BUF_FOR_OPRATE_SIZE (64)  //64 for other oprate space

#define M3U_FILE_HEAD    "#EXTM3U"
#define M3U8_FILE_TYPE   "#EXTGRP:"
#define M3U_FILE_EXTINF "#EXTINF:" /*-1 mean stream url*/
#define M3U_GROUP_TITLE   "group-title"

#define XML_ITEM_HEAD "\x20\x20\x20\x20<item>"
#define XML_ITEM_END   "\x20\x20\x20\x20</item>"

#define XML_KEY_HEAD "\x20\x20\x20\x20<key>"
#define XML_KEY_END  "</key>"

#define XML_VALUE_HEAD "\x20\x20\x20\x20<value>"
#define XML_VALUE_END "</value>"

#define XML_TYPE_HEAD "\x20\x20\x20\x20<type>"
#define XML_TYPE_END "</type>"

#define RESERVED_CHAR1  "&lt;"
#define RESERVED_CHAR2  "&gt;"
#define RESERVED_CHAR3  "&amp;"
#define RESERVED_CHAR4  "&apos;"
#define RESERVED_CHAR5  "&quot;"

#ifdef ECOS_OS
#define XML_PATH  "/mnt/m3u_convert_xml.xml"
#else
#define XML_PATH  "/tmp/m3u_convert_xml.xml"
#endif

static int sGlobalTitle = 0;
static char sGlobalTitle_type[256] = {0};

char *move_string(char *p, int num, int maxlen)
{
    int a = 0;

    if (p == NULL)
    {
        app_log_error("ERROR");
        return NULL;
    }
    a = strlen(p);
    if ((a + num) >= maxlen)
    {
        app_log_error("ERROR");
        return NULL;
    }

    while (a--)
    {
        *(p + a + num) = *(p + a);
    }

    return p;
}

char *Convert_string(char *p)
{
    char * tmpstr = NULL;
    char * save_head = p;

    if ( p == NULL )
    {
        return NULL;
    }

    while (*(p++) != '\0')
    {

        switch (*p)
        {

            case '<':
            {
                tmpstr = move_string(p, strlen(RESERVED_CHAR1) - 1, (BUF_PER_LINE_SIZE-(p-save_head)));
                if(tmpstr == NULL)
                    return NULL;
                strncpy(tmpstr, RESERVED_CHAR1, strlen(RESERVED_CHAR1));
            }
            break;
            case '>':
            {
                tmpstr = move_string(p, strlen(RESERVED_CHAR2) - 1, (BUF_PER_LINE_SIZE-(p-save_head)));
                if(tmpstr == NULL)
                    return NULL;
                strncpy(tmpstr, RESERVED_CHAR2, strlen(RESERVED_CHAR2));
            }
            break;
            case '&':
            {
                tmpstr = move_string(p, strlen(RESERVED_CHAR3) - 1, (BUF_PER_LINE_SIZE-(p-save_head)));
                if(tmpstr == NULL)
                    return NULL;
                strncpy(tmpstr, RESERVED_CHAR3, strlen(RESERVED_CHAR3));
            }
            break;
            case 39:
            {
                tmpstr = move_string(p, strlen(RESERVED_CHAR4) - 1, (BUF_PER_LINE_SIZE-(p-save_head)));
                if(tmpstr == NULL)
                    return NULL;
                strncpy(tmpstr, RESERVED_CHAR4, strlen(RESERVED_CHAR4));
            }
            break;
            case 34:
            {
                tmpstr = move_string(p, strlen(RESERVED_CHAR5) - 1, (BUF_PER_LINE_SIZE-(p-save_head)));
                if(tmpstr == NULL)
                        return NULL;
                strncpy(tmpstr, RESERVED_CHAR5, strlen(RESERVED_CHAR5));
            }
            break;
            default:
                break;
        }
    }

    return save_head;
}

char *del_cr_lf(char *p)
{
    int a = 0;

    if ( p == NULL)
    {
        return NULL;
    }

    while ((a++) < BUF_PER_LINE_SIZE)
    {
        if ((*(p + a) == '\r') && (*(p + a + 1) != '\n'))
        {
            strtok(p, "\r");
            break;
        }
        if (*(p + a) == '\n')
        {
            strtok(p, "\n");
            break;
        }
        if ((* (p + a) == '\r') && (*(p + a + 1) == '\n'))
        {
            strtok(p, "\r\n");
            break;
        }
    }

    return p;
}

char *net_m3u_convert_xml(char *File_path, char* Dest_file)
{
    FILE *fp_src = NULL;
    FILE *fp_des = NULL;

    int  i = 0;
    int  item_count = 0;
    int  type_flag_first_line  = 0;
    int  type_flag_second_line = 0;
    char *head = NULL;
    char *group_title = NULL;
    char *string_type = NULL;
    char *string_line = NULL;
    char *string_line1_for_type = NULL;
    char *string_line2_for_type = NULL;
    char *tmp = NULL;

    fp_src = fopen(File_path, "r");
    if (fp_src == NULL)
    {
        return NULL;
    }

    fp_des = fopen(Dest_file, "w+");
    if (fp_des == NULL)
    {
        fclose(fp_src);
        return NULL;
    }

    string_line = GxCore_Malloc(sizeof(char) * (BUF_PER_LINE_SIZE + BUF_FOR_OPRATE_SIZE));
    if (string_line  == NULL)
    {
        fclose(fp_des);
        fclose(fp_src);
        return NULL;
    }

    string_line1_for_type = GxCore_Malloc(sizeof(char) * (BUF_PER_LINE_SIZE + BUF_FOR_OPRATE_SIZE));
    if (string_line1_for_type == NULL)
    {
        fclose(fp_des);
        fclose(fp_src);
        GxCore_Free(string_line);
        return NULL;
    }

    string_line2_for_type = GxCore_Malloc(sizeof(char) * (BUF_PER_LINE_SIZE + BUF_FOR_OPRATE_SIZE));
    if (string_line2_for_type == NULL)
    {
        fclose(fp_des);
        fclose(fp_src);
        GxCore_Free(string_line);
        GxCore_Free(string_line1_for_type);
        return NULL;
    }

    group_title = GxCore_Malloc(sizeof(char) * (BUF_TYPE_STRING));
    if (group_title == NULL)
    {
        fclose(fp_des);
        fclose(fp_src);
        GxCore_Free(string_line);
        GxCore_Free(string_line1_for_type);
        GxCore_Free(string_line2_for_type);
        return NULL;
    }

    memset(group_title, 0, BUF_TYPE_STRING);
    head = group_title;

    sGlobalTitle = 0;
    memset(sGlobalTitle_type, 0x0, 128);

    /*add xml header*/
    fputs("<?xml version=\"1.0\"  encoding=\"utf-8\"?>", fp_des);
    fputs("\r\n", fp_des);
    fputs("<list>", fp_des);

    memset(string_line, 0, BUF_PER_LINE_SIZE);
    /*check m3u file header */
    fgets(string_line, BUF_PER_LINE_SIZE, fp_src);

    if (strncasecmp(string_line, M3U_FILE_HEAD, strlen(M3U_FILE_HEAD)) != 0)
    {
        if (strncasecmp((string_line + 3), M3U_FILE_HEAD, strlen(M3U_FILE_HEAD)) != 0)
        {
            fclose(fp_des);
            fclose(fp_src);
            GxCore_Free(string_line);
            GxCore_Free(string_line1_for_type);
            GxCore_Free(string_line2_for_type);
            return NULL;
        }
    }

    memset(string_line, 0, BUF_PER_LINE_SIZE);
    /*parser m3u file */
    while (fgets(string_line, BUF_PER_LINE_SIZE, fp_src) != NULL)
    {
        if (feof(fp_src))
        {
            break;
        }

#ifdef DEBUG_PRINTF
        app_log_error("string_data:%s\n", string_line);
#endif

        if (strncasecmp(string_line, M3U_FILE_EXTINF, strlen(M3U_FILE_EXTINF)) != 0)
        {
            continue;
        }

        if ((tmp = strstr(string_line, M3U_GROUP_TITLE)) != NULL)
        {
            type_flag_first_line = 1;
            sGlobalTitle = 1;
            while (*(tmp++) != '\"'); //get the first "
            if (*tmp == '\"')
            {
                type_flag_first_line = 0; //if only ""
            }
            else
            {
                if (strchr(tmp, '\"') == NULL) //check the second "
                {
                    type_flag_first_line = 0;
                }
                else
                {
                    i = 0;

                    while (*(tmp) != '\"')
                    {
                        i++;
                        if (i > BUF_TYPE_STRING - 1)
                        {
                            break;
                        }
                        *(group_title++) = *(tmp++);
                    }
                    group_title = head;
                    memset(string_line1_for_type, 0, BUF_PER_LINE_SIZE);
                    strcpy(string_line1_for_type, group_title);
                    tmp = Convert_string(string_line1_for_type);
                    tmp = move_string(tmp, strlen(XML_TYPE_HEAD),BUF_PER_LINE_SIZE);
                    strncpy(tmp, XML_TYPE_HEAD, strlen(XML_TYPE_HEAD));
                    tmp = del_cr_lf(tmp);
                    strcat(tmp, XML_TYPE_END);
                    string_type = tmp;
                    strcpy(sGlobalTitle_type, string_type);

#ifdef DEBUG_PRINTF
                    app_log_error("========type_flag_first_line =%d ============type:(%s)\n", type_flag_first_line, string_type);
#endif
                }
            }
        }

        fputs("\r\n", fp_des);
        fputs(XML_ITEM_HEAD, fp_des);
        fputs("\r\n", fp_des);

        tmp = strchr(string_line, ',');
        if (tmp != NULL)
        {
            tmp = Convert_string(tmp + 1);
            tmp = move_string(tmp, strlen(XML_KEY_HEAD),(BUF_PER_LINE_SIZE-(tmp-string_line)));
            strncpy(tmp, XML_KEY_HEAD, strlen(XML_KEY_HEAD));
            tmp = del_cr_lf(tmp);
            strcat(tmp, XML_KEY_END);
            fputs(tmp, fp_des);
#ifdef DEBUG_PRINTF
            app_log_error("====================key:[%s]\n", tmp);
#endif
        }
        else
        {
#ifdef DEBUG_PRINTF
            app_log_error("====================key error\n");
#endif
        }
        fputs("\r\n", fp_des);

        /////////////////save the type/////////////////////////////////////////
        if (type_flag_first_line == 0)
        {
            memset(string_line2_for_type, 0, BUF_PER_LINE_SIZE);
            fgets(string_line2_for_type, BUF_PER_LINE_SIZE, fp_src);
            if (strncasecmp(string_line2_for_type, M3U8_FILE_TYPE, strlen(M3U8_FILE_TYPE)) == 0)
            {
                type_flag_second_line = 1;
                tmp = strchr(string_line2_for_type, ':');
                if (tmp != NULL)
                {
                    tmp = Convert_string(tmp + 1);
                    tmp = move_string(tmp, strlen(XML_TYPE_HEAD),(BUF_PER_LINE_SIZE-(tmp-string_line2_for_type)));
                    strncpy(tmp, XML_TYPE_HEAD, strlen(XML_TYPE_HEAD));
                    tmp = del_cr_lf(tmp);
                    strcat(tmp, XML_TYPE_END);
                    string_type = tmp;
                    strcpy(sGlobalTitle_type, string_type);
#ifdef DEBUG_PRINTF
                    app_log_error("==========seconline==========type:[%s]\n", string_type);
#endif
                }
                else
                {
#ifdef DEBUG_PRINTF
                    app_log_error("111can not find group from second line \n");
#endif
                }
            }

#ifdef DEBUG_PRINTF
            else
            {
                app_log_error("111can not find group from second line \n");
            }
#endif
        }
        else
        {
            memset(string_line2_for_type, 0, BUF_PER_LINE_SIZE);
            fgets(string_line2_for_type, BUF_PER_LINE_SIZE, fp_src);
            if (strncasecmp(string_line2_for_type, M3U8_FILE_TYPE, strlen(M3U8_FILE_TYPE)) == 0)
            {
                type_flag_second_line = 1;
                tmp = strchr(string_line2_for_type, ':');
                if (tmp != NULL)
                {
                    tmp = Convert_string(tmp + 1);
                    tmp = move_string(tmp, strlen(XML_TYPE_HEAD),(BUF_PER_LINE_SIZE-(tmp-string_line2_for_type)));
                    strncpy(tmp, XML_TYPE_HEAD, strlen(XML_TYPE_HEAD));
                    tmp = del_cr_lf(tmp);
                    strcat(tmp, XML_TYPE_END);
                    string_type = tmp;

                    strcpy(sGlobalTitle_type, string_type);

#ifdef DEBUG_PRINTF
                    app_log_error("========firstline ============type:[%s]\n", string_type);
#endif
                }
                else
                {
#ifdef DEBUG_PRINTF
                    app_log_error("2222can not find group from second line \n");
#endif
                }
            }

#ifdef DEBUG_PRINTF
            else
            {
                app_log_error("2222can not find group from second line \n");
            }
#endif
        }
        ///////////////////////////////////////////////////////////////////////
        if (type_flag_second_line == 0/*&&type_flag_first_line==0*/)
        {
            memset(string_line, 0, BUF_PER_LINE_SIZE);
            strcpy(string_line, string_line2_for_type);
        }
        else
        {
            memset(string_line, 0, BUF_PER_LINE_SIZE);
            fgets(string_line, BUF_PER_LINE_SIZE, fp_src);
        }

        tmp = Convert_string(string_line);
        if(tmp == NULL)
            continue;
        tmp = move_string(string_line, strlen(XML_VALUE_HEAD),BUF_PER_LINE_SIZE);
        if(tmp == NULL)
            continue;
        strncpy(tmp, XML_VALUE_HEAD, strlen(XML_VALUE_HEAD));
        tmp = del_cr_lf(tmp);
        strcat(tmp, XML_VALUE_END);
        fputs(tmp, fp_des);

#ifdef DEBUG_PRINTF
        app_log_error("====================value:[%s]\n", tmp);
#endif
        if (type_flag_first_line == 1 || type_flag_second_line == 1)
        {
            type_flag_first_line = 0;
            type_flag_second_line = 0;
            fputs("\r\n", fp_des);

#ifdef DEBUG_PRINTF
            app_log_error("====================type2:[%s]\n", string_type);
#endif
            fputs(string_type, fp_des);
            string_type = NULL;
            fputs("\r\n", fp_des);
        }
        else
        {
            if (sGlobalTitle)
            {
                fputs("\r\n", fp_des);

                fputs(sGlobalTitle_type, fp_des);
                fputs("\r\n", fp_des);

#ifdef DEBUG_PRINTF
                app_log_error("========find global ============type:[%s]\n", sGlobalTitle_type);
#endif
            }
            else
            {
                fputs("\r\n", fp_des);
                fputs("\x20\x20\x20\x20<type>Other</type>", fp_des);
                fputs("\r\n", fp_des);
            }
        }

        fputs(XML_ITEM_END, fp_des);

        memset(group_title, 0, BUF_TYPE_STRING);
        memset(string_line, 0, BUF_PER_LINE_SIZE);
        memset(string_line1_for_type, 0, BUF_PER_LINE_SIZE);
        memset(string_line2_for_type, 0, BUF_PER_LINE_SIZE);

        item_count++;
        if (item_count >= NUM_LIMIT)
        {
            break;
        }
    }
    /*add xml file tail*/
    fputs("\r\n", fp_des);
    fputs("</list>", fp_des);

    GxCore_Free(group_title);
    GxCore_Free(string_line);
    GxCore_Free(string_line1_for_type);
    GxCore_Free(string_line2_for_type);

    fclose(fp_des);
    fclose(fp_src);

    return  Dest_file;
}

char *m3u_convert_xml(char *File_path)
{
    return net_m3u_convert_xml(File_path,XML_PATH);
}

char *net_xml_convert_xml(char *File_path, char* Dest_file)
{
    FILE *fp_src = NULL;
    FILE *fp_des = NULL;

    char  head_data[100] = {0};

#ifdef DEBUG_PRINTF
    app_log_error("===========come to %s %d\n", __FUNCTION__, __LINE__);
#endif

    fp_src = fopen(File_path, "r");
    if (fp_src == NULL)
    {
        return NULL;
    }

#ifdef DEBUG_PRINTF
    app_log_error("===========come to %s %d\n", __FUNCTION__, __LINE__);
#endif

    fp_des = fopen(Dest_file, "w+");
    if (fp_des == NULL)
    {
        fclose(fp_src);
        return NULL;
    }

    rewind(fp_src);
    fgets(head_data, 100, fp_src);

    if (head_data[0] == 0xef && head_data[1] == 0xbb && head_data[2] == 0xbf)
    {
        fseek(fp_src, 3, SEEK_SET); //ef bb bf
        app_log_error("============UTF-8\n");
    }

    while (!feof(fp_src))
    {
        fputc(fgetc(fp_src), fp_des);
    }

    fclose(fp_des);
    fclose(fp_src);
    return Dest_file;
}

char *xml_convert_xml(char *File_path)
{
    return net_xml_convert_xml(File_path,XML_PATH);
}
