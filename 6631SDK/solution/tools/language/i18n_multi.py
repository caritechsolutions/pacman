#!/usr/bin/env python2
# -*- coding: utf-8 -*-

##
# Module/script example of the xlrd API for extracting information
# about named references, named constants, etc.
#
# <p>Copyright ?2006 Stephen John Machin, Lingfo Pty Ltd</p>
# <p>This module is part of the xlrd package, which is released under a BSD-style licence.</p>
##

import xlrd
import sys, os
import glob
import types
import codecs
from xml.dom.minidom import Text, Element

i18n_xml = None
lang_xml = None
bk = None

def create_xml():

    global i18n_xml
    global bk

    xml_name=lang_xml_path+ "i18n.xml"
    xls_name=xls_path+ "i18n.xls"
    print "xml = %s" % (xml_name)

    i18n_xml = codecs.open (xml_name, "w", "utf-8")
    bk = xlrd.open_workbook(xls_name)

def create_i18n_xml():

    global i18n_xml
    global lang_xml
    global bk

    sh = bk.sheet_by_index(0)
    nrows = sh.nrows
    ncols = sh.ncols
    print "rows = %d; cols = %d" % (nrows, ncols)

    print >> i18n_xml, "<?xml version=\"1.0\" encoding=\"utf-8\" standalone=\"yes\"?>"
    print >> i18n_xml, "<i18n>"

    for i in range(1,ncols):
        row_data = sh.row_values(0)
        print >> i18n_xml, "    <language id= \"%s\">%s.xml</language>" %\
        ((row_data[i]),(row_data[i]))

        # for language.xml
        lang_name = lang_xml_path+(row_data[i])+".xml"
        lang_xml = codecs.open (lang_name, "w", "utf-8")
        print >> lang_xml, "<?xml version=\"1.0\" encoding=\"utf-8\" standalone=\"yes\"?>"
        print >> lang_xml, "    <language name=\"%s\">" % (row_data[i])
        row_data = sh.row_values(1)
        print >> lang_xml, "        <font>%s</font>" % (row_data[i])

        for j in range(2,nrows):
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

            print >> lang_xml, "        <tran id=\"%s\"                >%s</tran>" % (xmlID, xmlVal)

        print >> lang_xml, "    </language>"
        lang_xml.close()

    print >> i18n_xml, "</i18n>"

    i18n_xml.close()


if __name__ == "__main__":
    if len(sys.argv) != 2:
        print "python i18n_multi.py <language_dir_path>"
        print "     eg python i18n_multi.py /code/solution/theme/classic_dark/language"
        sys.exit(1)

    language_dir_path=sys.argv[1]
    lang_xml_path= language_dir_path + '/'
    xls_path=language_dir_path + '/'

    create_xml()
    create_i18n_xml()
