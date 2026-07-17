#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import xlrd
import sys
import os
import codecs
from xml.dom.minidom import Text, Element

def create_xml(lang_xml_path, xls_path):
    """
    创建 i18n.xml 和对应的语言 XML 文件
    """
    xml_name = os.path.join(lang_xml_path, "i18n.xml")
    xls_name = os.path.join(xls_path, "i18n.xls")
    print(f"xml = {xml_name}")

    with codecs.open(xml_name, "w", "utf-8") as i18n_xml:
        bk = xlrd.open_workbook(xls_name)
        sh = bk.sheet_by_index(0)
        nrows = sh.nrows
        ncols = sh.ncols
        print(f"rows = {nrows}; cols = {ncols}")

        print("<?xml version=\"1.0\" encoding=\"utf-8\" standalone=\"yes\"?>", file=i18n_xml)
        print("<i18n>", file=i18n_xml)

        for i in range(1, ncols):
            row_data = sh.row_values(0)
            print(f"    <language id= \"{row_data[i]}\">{row_data[i]}.xml</language>", file=i18n_xml)

            # 为每种语言创建对应的 XML 文件
            lang_name = os.path.join(lang_xml_path, f"{row_data[i]}.xml")
            with codecs.open(lang_name, "w", "utf-8") as lang_xml:
                print("<?xml version=\"1.0\" encoding=\"utf-8\" standalone=\"yes\"?>", file=lang_xml)
                print(f"    <language name=\"{row_data[i]}\">", file=lang_xml)
                row_data = sh.row_values(1)
                print(f"        <font>{row_data[i]}</font>", file=lang_xml)

                for j in range(2, nrows):
                    row_data = sh.row_values(j)
                    if row_data[i] == "" or row_data[i] == row_data[0]:
                        continue

                    t = Text()
                    t.data = row_data[0]
                    e = Element('')
                    e.appendChild(t)
                    xmlID = e.toxml()[2:-3]

                    t = Text()
                    t.data = row_data[i]
                    e = Element('')
                    e.appendChild(t)
                    xmlVal = e.toxml()[2:-3]

                    print(f"        <tran id=\"{xmlID}\"                >{xmlVal}</tran>", file=lang_xml)

                print("    </language>", file=lang_xml)

        print("</i18n>", file=i18n_xml)

def process_language_directories(base_directory):
    """
    遍历指定目录下的所有名为 language 的子目录
    """
    for root, dirs, files in os.walk(base_directory):
        if 'language' in dirs:
            language_dir_path = os.path.join(root, 'language')
            print(f"Processing language directory: {language_dir_path}")
            create_xml(language_dir_path, language_dir_path)

if __name__ == "__main__":
    if len(sys.argv) != 2:
        print("用法: python3 i18n_multi_all_theme.py <base_directory>")
        print("例如: python3 i18n_multi_all_theme.py /code/solution/theme")
        sys.exit(1)

    base_directory = sys.argv[1]
    process_language_directories(base_directory)

