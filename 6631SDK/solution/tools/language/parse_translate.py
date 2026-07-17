#!/usr/bin/env python2
# coding=utf-8

import os
import xlrd
import xlwt
import re
import sys

str_id_list=[]
define_id_sorted_list=[]
xls_id_sorted_list=[]
useless_id_list=[]

def del_diff_id(theme):
    global str_id_list
    diff_id=[]
    begin=False
    count=0
    with open('../../app/include/app_str_id.h') as f_id:
        for line in f_id:
            if theme in line:
                begin=True
            elif " STR_ID" in line:
                if begin:
                    diff_id.append(re.split('"',line)[1])
            elif "#endif" in line:
                if begin:
                    if count == 0:
                        begin=False
                    else:
                        count=count-1
            elif ((begin is True) and (line.startswith("#if"))):
                count=count+1

    str_id_list = list(set(str_id_list).difference(set(diff_id)))

def get_define_str_id():
    global define_id_sorted_list
    global theme

    with open('../../app/include/app_str_id.h') as f_id:
        for line in f_id:
            if "Not translated" in line:
                break
            if " STR_ID" in line:
                str_id= re.split(('"'),line)
                if len(str_id)>1 and str_id[1] != "":
                    str_id_list.append(str_id[1])
    if 'mini_16bit' in theme:
        del_diff_id("THEME_LINUX")
    elif 'linux' in theme:
        del_diff_id("THEME_MINI_16BIT")

    define_id_sorted_list=sorted(str_id_list)
    with open('./define_id_sorted','wr') as f:
        for i in define_id_sorted_list:
            f.write(i+'\n')

def sort_i18n_xls(theme):
    global xls_id_sorted_list
    wd=xlrd.open_workbook(theme + '/language/i18n.xls')
    sheet=wd.sheets()[0]
    nrow=sheet.nrows
    ncol=sheet.ncols
    org_xls_id_list=[]
    for row in range(2,nrow):
        xls_id = sheet.cell(row,0).value.encode('utf-8')
        if xls_id != "":
            org_xls_id_list.append(xls_id)

    xls_id_sorted_list=sorted(org_xls_id_list)
    temp_xls=xlwt.Workbook()
    sort_sheet=temp_xls.add_sheet("Sort_list")
    for r in range(0,2):
        for c in range(0,ncol):
            word=sheet.cell(r,c).value
            sort_sheet.write(r,c,word)

    w_row=2
    for i in xls_id_sorted_list:
        for j in range(2,nrow):
            if i == sheet.cell(j,0).value.encode('utf-8'):
                for col in range(0,ncol):
                    sort_sheet.write(w_row,col,sheet.cell(j,col).value)
                w_row = w_row + 1
    temp_xls.save('./xls_id_sorted.xls')

def parse_info():
    global define_id_sorted_list
    global xls_id_sorted_list
    global theme
    print "Has define str id %d"%(len(define_id_sorted_list))
    print "Has translate str id %d"%(len(xls_id_sorted_list))
    if len(define_id_sorted_list) != len(set(define_id_sorted_list)):
        repeat_id=[]
        for i in set(define_id_sorted_list):
            count=0
            for j in define_id_sorted_list:
                if i == j:
                    count=count+1
            if count > 1:
                repeat_id.append(i)

        print 'Define has repeat id'
        for i in repeat_id:
            print '     %s'%i
    if len(xls_id_sorted_list) != len(set(xls_id_sorted_list)):
        repeat_id=[]
        for i in set(xls_id_sorted_list):
            count=0
            for j in xls_id_sorted_list:
                if i == j:
                    count=count+1
            if count > 1:
                repeat_id.append(i)

        print 'xls has repeat id'
        for i in repeat_id:
            print '     %s'%i
    diff_list=[]
    useless_word=[]
    use_word=[]
    for i in range(0,len(xls_id_sorted_list)):
        if xls_id_sorted_list[i] not in define_id_sorted_list:
            diff_list.append(xls_id_sorted_list[i])
    if len(diff_list) == 0:
        print "All translated words has define"
    else:
        for word in diff_list:
            if '"' in word:
                continue
            if os.system('grep -rq "%s" ../../app %s/widget'%(word,theme)) != 0:
                useless_word.append(word)
            else:
                use_word.append(word)
        if len(useless_word) != 0:
            print "Follow words not find in project, you can delete it"
            for word in useless_word:
                print "     %s"%(word)
        if len(use_word) != 0:
            print "Follow words could find in app,you need check and add define"
            for word in use_word:
                print "     %s"%(word)
        if len(use_word) == 0 and len(useless_word) == 0:
            print "All translated words has define"

    diff_list=[]
    for i in define_id_sorted_list:
        if i not in xls_id_sorted_list:
            diff_list.append(i)
    if len(diff_list) == 0:
        print "All define words has translated"
    else:
        print "Has %d word define but not translate"%(len(diff_list))
        for word in diff_list:
            print "     %s"%(word)

theme=None

if __name__ == '__main__':

    if len(sys.argv)<2:
        print 'python parse_translate linux/mini_16bit'
        sys.exit()
    theme = '../../theme/' + sys.argv[1]

    get_define_str_id()

    sort_i18n_xls(theme)

    parse_info()

