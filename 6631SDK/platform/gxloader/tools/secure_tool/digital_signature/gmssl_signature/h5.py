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
from datetime import datetime
import struct
from crc32 import crc32


def sign_h5_app(h5_app_path, key_dir, temp_dir, output_path):
    source_size = os.path.getsize(h5_app_path)
    dest_file   = temp_dir + os.sep + "dest.bin"
    s = open(h5_app_path, "rb")
    d = open(dest_file, "wb")

    magic       = bytearray(32)
    version     = bytearray(8)
    createday   = bytearray(4)
    createtime  = bytearray(4)
    signed_len  = bytearray(4)
    signature   = bytearray(64)
    crc_value   = bytearray(4)

    head_len = len(magic) + len(version) + len(createday) + len(createtime) \
                    + len(signed_len) + len(signature) + len(crc_value)

    magic   = "GuoxinImg_ADVCA_ImgHead_MagicNum".encode()
    version[0:] = "00000001".encode()
    date = datetime.now().timetuple()
    date_str = "%04d%02d%02d"%(date[0], date[1], date[2])
    createday = hex2byte(date_str)
    date_str = "%02d%02d%02d00"%(date[3], date[4], date[5])
    createtime = hex2byte(date_str)

    max_buffer_len = 1 * 1024 * 1024


    if source_size <= head_len:
        print("insert magic")
        offset = 0
        signed_len  = struct.pack("I", source_size)
    else:
        s_head = s.read(len(magic))
        if s_head == magic:
            print("exist magic")
            offset = head_len
            signed_len  = struct.pack("I", source_size - head_len)
        else:
            print("insert magic")
            offset = 0
            signed_len  = struct.pack("I", source_size)

    s.seek(offset)
    read_offs = offset

    while (read_offs != source_size):
        read_size = min(source_size - read_offs, max_buffer_len)
        minor_data = s.read(read_size)
        d.write(minor_data)
        read_offs += read_size

    d.flush()
    d.close()

    gm = GmTool(key_dir, temp_dir)
    signed_data = dest_file + ".sign"
    gm.sign_for_other_file(dest_file, signed_data, "blk0_pri.bin")

    data = magic + version + createday + createtime + signed_len + \
            ReadPartBytesOfFile(signed_data)

    crc_value = crc32(data)
    crc_data = struct.pack('I', crc_value)

    data = data + crc_data

    output_f = open(output_path, "wb")
    output_f.write(data)

    s.seek(offset)
    read_offs = offset

    while (read_offs != source_size):
        read_size = min(source_size - read_offs, max_buffer_len)
        minor_data = s.read(read_size)
        output_f.write(minor_data)
        read_offs += read_size

    output_f.flush()
    output_f.close()
    s.close()

if __name__ == '__main__':
    parser = argparse.ArgumentParser(formatter_class=argparse.RawDescriptionHelpFormatter, \
    description=textwrap.dedent( \
    '''%(prog)s: signature ota
eg: %(prog)s -k key_directory -o out.bin file_to_sign\n '''), epilog='')
    parser.add_argument("-k",         dest='key', required=True,       help = "key directory path")
    parser.add_argument("-o",         dest='out', required=True,       help = "output directory path")
    parser.add_argument("path",       nargs=1,                         help = "input  file path")
    parser.add_argument("-v", "--version",  action='version', version='%(prog)s v1.0.0')

    if len(sys.argv) == 1:
        parser.print_help()
        sys.exit(-1)

    args = parser.parse_args()

    print(args)

    if True:
        temp = tempfile.TemporaryDirectory()
        flash_all_path = args.path[0]
        #sign_h5_app(flash_all_path, args.key, temp.name)
        sign_h5_app(flash_all_path, args.key, "tmp", args.out)
    try:
        pass
    except UsrError as e:
        print("error: ", e.msg)
    except:
        print(str(sys.exc_info()))
    else:
        print("sign h5 app list file finish!")

