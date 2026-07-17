#!/usr/bin/env python3
# coding=utf-8
import requests
import json
import execjs  # 必须，需要先用pip 安装，用来执行js脚本
from urllib.parse import quote
import sys
import xlrd
from xlutils.copy import copy
debug = False


class Py4Js:

    def __init__(self):
        self.ctx = execjs.compile("""
            function TL(a) {
                var k = "";
                var b = 406644;
                var b1 = 3293161072;
                var jd = ".";
                var $b = "+-a^+6";
                var Zb = "+-3^+b+-f";
                for (var e = [], f = 0, g = 0; g < a.length; g++) {
                    var m = a.charCodeAt(g);
                    128 > m ? e[f++] = m : (2048 > m ? e[f++] = m >> 6 | 192 : (55296 == (m & 64512) && g + 1 < a.length && 56320 == (a.charCodeAt(g + 1) & 64512) ? (m = 65536 + ((m & 1023) << 10) + (a.charCodeAt(++g) & 1023),
                    e[f++] = m >> 18 | 240,
                    e[f++] = m >> 12 & 63 | 128) : e[f++] = m >> 12 | 224,
                    e[f++] = m >> 6 & 63 | 128),
                    e[f++] = m & 63 | 128)
                }
                a = b;
                for (f = 0; f < e.length; f++) a += e[f],
                a = RL(a, $b);
                a = RL(a, Zb);
                a ^= b1 || 0;
                0 > a && (a = (a & 2147483647) + 2147483648);
                a %= 1E6;
                return a.toString() + jd + (a ^ b)
            };
            function RL(a, b) {
                var t = "a";
                var Yb = "+";
                for (var c = 0; c < b.length - 2; c += 3) {
                    var d = b.charAt(c + 2),
                    d = d >= t ? d.charCodeAt(0) - 87 : Number(d),
                    d = b.charAt(c + 1) == Yb ? a >>> d: a << d;
                    a = b.charAt(c) == Yb ? a + d & 4294967295 : a ^ d
                }
                return a
            }
        """)

    def get_tk(self, text):
        return self.ctx.call("TL", text)


def build_url(text, tk, tl='zh-CN'):
    """
    需要用转URLEncoder
    :param text:
    :param tk:
    :param tl:
    :return:
    """
    return 'https://translate.google.com.hk/translate_a/single?client=webapp&sl=auto&tl=' + tl + '&hl=zh-CN&dt=at&dt=bd&dt=ex&dt=ld&dt=md&dt=qca&dt=rw&dt=rm&dt=ss&dt=t&source=btn&ssel=0&tsel=0&kc=0&tk=' \
           + str(tk) + '&q=' + quote(text, encoding='utf-8')


def translate(js, text, tl='zh-CN'):
    header = {
        'authority': 'translate.google.com.hk',
        'method': 'GET',
        'path': '',
        'scheme': 'https',
        'accept': '*/*',
        'accept-encoding': 'gzip, deflate, br',
        'accept-language': 'zh-CN,zh;q=0.9,ja;q=0.8',
        # 'cookie': '_ga=GA1.3.110668007.1547438795; _gid=GA1.3.791931751.1548053917; 1P_JAR=2019-1-23-1; NID=156=biJbQQ3j2gPAJVBfdgBjWHjpC5m9vPqwJ6n6gxTvY8n1eyM8LY5tkYDRsYvacEnWNtMh3ux0-lUJr439QFquSoqEIByw7al6n_yrHqhFNnb5fKyIWMewmqoOJ2fyNaZWrCwl7MA8P_qqPDM5uRIm9SAc5ybSGZijsjalN8YDkxQ',
         'cookie':'_ga=GA1.3.110668007.1547438795; _gid=GA1.3.1522575542.1548327032; 1P_JAR=2019-1-24-10; NID=156=ELGmtJHel1YG9Q3RxRI4HTgAc3l1n7Y6PAxGwvecTJDJ2ScgW2p-CXdvh88XFb9dTbYEBkoayWb-2vjJbB-Rhf6auRj-M-2QRUKdZG04lt7ybh8GgffGtepoA4oPN9OO9TeAoWDY0HJHDWCUwCpYzlaQK-gKCh5aVC4HVMeoppI',
        # 'cookie': '',
        'user-agent': 'Mozilla/5.0 (Windows NT 10.0; WOW64)  AppleWebKit/537.36 (KHTML, like Gecko) Chrome/63.0.3239.108 Safari/537.36',
        'x-client-data': 'CKi1yQEIhrbJAQijtskBCMG2yQEIqZ3KAQioo8oBCL+nygEI7KfKAQjiqMoBGPmlygE='
    }
    url = build_url(text, js.get_tk(text), tl)
    res = []
    try:
        r = requests.get(url, headers=header)
        result = json.loads(r.text)
        r.encoding = "UTF-8"
        if debug:
            print(r.url)
            print(r.headers)
            print(r.request.headers)
            print(result)

        res = result[0]
        if res is None:
            if result[7] is not None:
                # 如果我们文本输错，提示你是不是要找xxx的话，那么重新把xxx正确的翻译之后返回
                try:
                    correct_text = result[7][0].replace('<b><i>', ' ').replace('</i></b>', '')
                    if debug:
                        print(correct_text)
                    correct_url = build_url(correct_text, js.get_tk(correct_text), tl)
                    correct_response = requests.get(correct_url)
                    correct_result = json.loads(correct_response.text)
                    res = correct_result[0]
                except Exception as e:
                    if debug:
                        print(e)
                    res = []

    except Exception as e:
        res = []
        if debug:
            print(url)
            print("翻译" + text + "失败")
            print("错误信息:")
            print(e)
    finally:
        return res

def find_empty_rows_index(workbook):
    """
    找到 .xls 文件中的空行（所有单元格内容为空的行）。

    :param workbook: 打开的 .xls 文件对象
    :return: 空行的行号列表（从0开始）
    """
    sheet = workbook.sheet_by_index(0)  # 默认操作第一个工作表

    for row_idx in range(sheet.nrows):
        is_empty_row = True
        for col_idx in range(sheet.ncols):
            cell_value = sheet.cell_value(row_idx, col_idx)
            if cell_value != "":
                is_empty_row = False
                break  # 如果有一个单元格不为空，则不是空行
        if is_empty_row:
            print(f"Row {row_idx} is empty.")
            return row_idx
    return sheet.nrows

def write_into_xls(path):
    rb=xlrd.open_workbook(path,formatting_info=True)
    empty_row_index = find_empty_rows_index(rb)
    wb=copy(rb)
    sheet=wb.get_sheet(0)
    for c in range(0,len(results)):
        sheet.write(empty_row_index, c, results[c])
    wb.save(path)


tolangs=[]
results=[]
if __name__ == '__main__':
    tolangs=[]
    if len(sys.argv)<2:
        print ("python3 autoTranslate.py <language_dir_path>")
        print ('eg:python3 autoTranslate.py /code/solution/theme/classic_dark/language "hello world"')
        print ('eg:python3 autoTranslate.py /code/solution/app/netapps/weather/str "hello world"')
        sys.exit()
    input_str= sys.argv[2]
    language_dir_path=sys.argv[1]
    if language_dir_path.find('classic_dark') >=0 or language_dir_path.find('classic_blue') >= 0 or language_dir_path.find('grid_purple') >= 0:
        tolangs=['en','zh-CN','ar','fa','fr','es','pt','tr','vi','ru','de','th']
    elif language_dir_path.find('classic_purple') >=0 or language_dir_path.find('sky_blue') >= 0:
        tolangs=['en','id','hr','hy','ka','fr','ru','uk','ky','uz','kk','vi','bg','pl','it','de','es','ko','th','my','lt','et','lv','cs','el','sq','fa']
    elif language_dir_path.find('str') >=0:
        tolangs=['en','zh-CN','ar','fa','fr','es','pt','tr','vi','ru','de','th','id','hr','hy','ka','uk','ky','uz','kk','bg','pl','it','ko','my','lt','et','lv','cs','el','sq','fa']
    js = Py4Js()
    results.append(input_str)
    for l in tolangs:
        result = translate(js, input_str, l)
        results.append(result[0][0])
    path=language_dir_path + '/i18n.xls'
    write_into_xls(path)
    print(results)

