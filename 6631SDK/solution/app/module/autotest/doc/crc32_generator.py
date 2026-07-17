#!/usr/bin/env python2
# -*- coding: utf-8 -*-

import sys
import os
import zlib

reload(sys)
sys.setdefaultencoding('utf-8')

if __name__ == '__main__':
    arg_len = len(sys.argv)
    if(arg_len > 1):
        _str = sys.argv[1]
        print _str.replace('"','')
        print hex(zlib.crc32(_str) & 0xffffffff)
    else:
        print "python crc32_generator.py \"strings\""

