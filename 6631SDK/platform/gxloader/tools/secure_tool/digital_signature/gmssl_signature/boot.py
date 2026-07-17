#! /usr/bin/env python3
# -*- coding: UTF-8 -*-

import os
import sys
import argparse
import textwrap
import subprocess
from common import *
#from log import logger
from crc32 import crc32
import struct
from signature import GmTool
import tempfile

def sign_boot(key_dir, temp_dir, boot_path):
    gm = GmTool(key_dir, temp_dir)
    """
    if is_nand_firmware(boot_path):
        print("nand firmware")
        data = gm.sign_for_bootloader_file(boot_path, 2, 'nand')
    else:
        print("nor firmware")
        data = gm.sign_for_bootloader_file(boot_path, 2, 'nor')
    """
    chip_type, flash_type = get_chip_and_flash_type(boot_path)
    data = gm.sign_bootloader_file(chip_type, boot_path, flash_type)
    SaveData(boot_path, data)


if __name__ == '__main__':
    parser = argparse.ArgumentParser(formatter_class=argparse.RawDescriptionHelpFormatter, \
    description=textwrap.dedent( \
    '''%(prog)s: signature bootloader
eg: %(prog)s -k key_directory gxloader.bin\n '''), epilog='')
    parser.add_argument("-k",         dest='key', required=True,  help = "key directory path")
    parser.add_argument("path",       nargs=1,                    help = "image bin path to search")
    parser.add_argument("-v", "--version",  action='version',    version='%(prog)s v1.0.0')

    if len(sys.argv) == 1:
        parser.print_help()
        sys.exit(-1)

    args = parser.parse_args()

    try:
        temp = tempfile.TemporaryDirectory()
        print("temp: ", temp.name)
        boot_path = args.path[0]
        key_dir = args.key
        sign_boot(key_dir, temp.name, boot_path)
    except UsrError as e:
        print(e.msg)
    except:
        print(str(sys.exc_info()))
    else:
        print("signature file finish!")



