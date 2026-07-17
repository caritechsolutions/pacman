#! /usr/bin/env python3
# -*- coding: UTF-8 -*-

import os
import sys
import subprocess
from common import *
import argparse
import textwrap
import tempfile
from ota import sign_ota_image
from boot import sign_boot
from mrs import make_mrs
from gmtool import GmToolBase
from h5 import sign_h5_app
import shutil
import distutils.dir_util
from version import version

def ota_directory(key_dir, sub_dir, max_size=None):
    print("-> sign ", sub_dir, flush=True)
    if not os.path.isdir(sub_dir):
        print(sub_dir, " not exist, skip", flush=True)
        return
    flash_all_conf = sub_dir + os.sep + "flash_ota.conf"

    if not os.path.exists(flash_all_conf) or not os.path.isfile(flash_all_conf):
        print(flash_all_conf, " not exist, skip", flush=True)
        return

    temp = tempfile.TemporaryDirectory()
    sign_ota_image(flash_all_conf, key_dir, temp.name, True, max_size)


def gx3215_signature(key_dir, image_dir):
    if image_dir.endswith(os.sep):
        image_dir = image_dir[:-1]

    caqc_suma_dir  = image_dir + os.sep + "GX3215B_CAQC_SUMA"
    caqc_novel_dir = image_dir + os.sep + "GX3215B_CAQC_NOVEL"
    loader_qc_dir  = image_dir + os.sep + "GX3215B_LoaderQC"

    print("\nGX3215B Signature CAQC")
    ota_directory(key_dir, caqc_suma_dir)
    ota_directory(key_dir, caqc_novel_dir)

    if not os.path.isdir(loader_qc_dir):
        print(loader_qc_dir, " not exist, skip")
        return

    print("\nGX3215B Signature LoaderQC")
    loader = loader_qc_dir + os.sep + "bootloader.bin"

    if os.path.exists(loader) and os.path.isfile(loader):
        print("-> sign ", loader)
        temp = tempfile.TemporaryDirectory()
        sign_boot(key_dir, temp.name, loader)
    else:
        print(loader, " not exist, skip")


if __name__ == '__main__':
    parser = argparse.ArgumentParser(formatter_class=argparse.RawDescriptionHelpFormatter, \
    description=textwrap.dedent( \
    '''%(prog)s: signature GX3215B image
eg: %(prog)s input output key_path\n '''),
    epilog="""
help message:
        config.txt内容格式:(不能修改文件名且 config.txt 需要存放在 gx3215b_signature 同级目录下)
        input=input_image_path(手动指定)
        output=output_image_path(手动指定)
        keypath=key(手动指定)

        key_path内的文件名:(不能修改文件名)
        blk0_pri.bin
        blk0_pub.bin
        blk1_pri.bin
        blk1_pub.bin

        image_path内的文件名:(不能修改文件名)
        GX3215B_CAQC_NOVEL
        GX3215B_CAQC_SUMA
        GX3215B_LoaderQC
""")
    parser.add_argument("dirs",       nargs=3,                help = "input image directory and output image directory and key directory")
    parser.add_argument("-v", "--version",  action='version', version='%(prog)s ' + version)
    config_path = os.path.dirname(sys.argv[0])

    if os.path.isfile(config_path + os.sep + "config.txt") and len(sys.argv) == 1:
        fo = open(config_path + os.sep + "config.txt", "r")
        config_file = fo.readlines()
        fo.close()

        input_dir = config_file[0].replace("input=", "").replace("\n", "").strip()
        image_dir = config_file[1].replace("output=", "").replace("\n", "").strip()
        key_dir = config_file[2].replace("keypath=", "").replace("\n", "").strip()

        if (len(input_dir) == 0 or len(image_dir) == 0 or len(key_dir) == 0):
            parser.print_help()
            sys.exit(-1)

    else:
        if (len(sys.argv) == 1):
            parser.print_help()
            sys.exit(-1)

        args = parser.parse_args()
        input_dir  = args.dirs[0]
        image_dir  = args.dirs[1]
        key_dir    = args.dirs[2]

    try:
        test = GmToolBase()
        msg = test.RunGmsslVersion()
        print(msg)
    except:
        print("run Gmssl tools fail, make sure install Gmssl!", flush=True)
        print("http://gmssl.org", flush=True)
        print("quick start: ", "http://gmssl.org/docs/quickstart.html", flush=True)
        sys.exit(-1)

    try:
        temp = tempfile.TemporaryDirectory()

        distutils.dir_util.copy_tree(input_dir, image_dir)

        if os.path.isfile(image_dir):
            save_path = image_dir + ".save"
            sign_h5_app(image_dir, key_dir, temp.name, save_path)
            shutil.move(save_path, image_dir)
        else:
            gx3215_signature(key_dir, image_dir)
    except UsrError as e:
        print("error: ", e.msg, flush=True)
    except:
        print(str(sys.exc_info()))
    else:
        print("\nGX3215B Signature Finish!", flush=True)



