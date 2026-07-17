#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import subprocess
import json
import hashlib
import argparse
import sys
import os

def main( streamjson, indexs):
    if not os.path.exists(streamjson):
        print('%s 文件不存在' %(streamjson))
        sys.exit()
    
    if indexs == []:
        print("请指定要查询的hash索引")
        sys.exit()

    with open(streamjson, 'r') as fd:
        streaminfo = json.load(fd)
    for i in indexs:
        i = int(i)
        if i >= streaminfo['frames']:
            print('%d 索引值无效，最大值不超过 %d'  %(i, streaminfo['frames']))
            sys.exit()
        if i == 0:
            i = 1
        print(streaminfo['hash'][i-1])



if __name__ == '__main__':
    parser = argparse.ArgumentParser(description = "")
    parser.add_argument('-f', dest='streamjson', help='码流信息json文件')
    parser.add_argument('-i', dest='index', nargs='+', default = [], help='帧索引，支持多个')
    if len(sys.argv) == 1:
        sys.argv.append('-h')
    args = parser.parse_args()

    main(args.streamjson, args.index)
    #sha256sum('./streampeak.py')



