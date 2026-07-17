#! /usr/bin/env python3
# -*- coding: UTF-8 -*-

import os
import sys
import subprocess
from common import *
from gmtool import GmToolBase
import argparse
import textwrap
import tempfile
from signature import GmTool
import platform
import time
from mrs import make_mrs


class ota(GmToolBase):
    system = platform.system()
    if system == "Windows":
        genflash = resource_path("bin" + os.sep + "genflash.exe")
    elif system == "Linux":
        genflash = resource_path("bin" + os.sep + "genflash.elf")


    def genflash_merge(self, conf, name):
        workdir = os.path.dirname(conf)
        self.RunSubProcess([self.genflash, "merge", conf, name], workdir)

def sign_ota_image(flash_all_path, key_dir, temp_dir, is_mrs, max_size=None):
    if is_mrs:
        make_mrs(flash_all_path, key_dir, temp_dir)

    workdir = os.path.dirname(flash_all_path)

    binTable = ParseConf(flash_all_path)
    flash_head_path = workdir + os.sep + binTable["HEAD"]

    ver = ReadPartBytesOfFile(flash_head_path, 0 , 2)
    mrsVer = "%02x%02x"%(ver[1], ver[0])

    dirname = workdir + os.sep + "GX3215B_OTA_" + mrsVer + \
            time.strftime("_%Y%m%d%H%M%S", time.localtime())
    os.makedirs(dirname)
    output = dirname + os.sep + "GX3215B_OTA_" + mrsVer + ".bin.merge"

    gm = ota()
    gm.genflash_merge(flash_all_path, output)
    CheckGenFileOK(output)

    if max_size != None:
        max_buffer_len = 2 * 1024 * 1024
        max_len = max_size * 1024 * 1024
        # print("max len 0x%x"%max_len)
        mergeLen = os.path.getsize(output)
        # print("data len 0x%x"%(mergeLen))
        fillLen = max_len - mergeLen
        print("fill len 0x%x"%fillLen)

        f = open(output, 'ab')

        reserved = bytearray(max_buffer_len)

        for i in range(max_buffer_len):
            reserved[i] = 0x00

        while (fillLen > 0):
            if fillLen >= max_buffer_len:
                f.write(reserved)
            else:
                f.write(reserved[0:fillLen])
            fillLen = fillLen - max_buffer_len
        f.flush()
        f.close()

    gm = GmTool(key_dir, temp_dir)
    signFile = dirname + os.sep + "GX3215B_OTA_" + mrsVer + ".sign"
    gm.sign_for_other_file(output, signFile)

    print("sign ota finish")

if __name__ == '__main__':
    parser = argparse.ArgumentParser(formatter_class=argparse.RawDescriptionHelpFormatter, \
    description=textwrap.dedent( \
    '''%(prog)s: signature ota
eg: %(prog)s -k key_directory flash_all.conf\n '''), epilog='')
    parser.add_argument("-k",         dest='key', required=True,       help = "key directory path")
    parser.add_argument("-m",         dest='mrs', action='store_true', help = "update mrs")
    parser.add_argument("-s",         dest='size', type=int,           help = "max size")
    parser.add_argument("path",       nargs=1,                         help = "flash_all.conf path")
    parser.add_argument("-v", "--version",  action='version', version='%(prog)s v1.0.0')

    if len(sys.argv) == 1:
        parser.print_help()
        sys.exit(-1)

    args = parser.parse_args()

    print(args)

    try:
        temp = tempfile.TemporaryDirectory()
        flash_all_path = args.path[0]
        sign_ota_image(flash_all_path, args.key, temp.name, args.mrs, args.size)
    except UsrError as e:
        print("error: ", e.msg)
    except:
        print(str(sys.exc_info()))
    else:
        print("update OTA file finish!")

