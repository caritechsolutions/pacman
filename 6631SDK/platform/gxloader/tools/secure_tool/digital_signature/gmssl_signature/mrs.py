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

def MakeMRS(key_dir, temp_dir, binPath):
    MRSLEN = 768
    try:
        readbin = ReadPartBytesOfFile(binPath["HEAD"])
        if len(readbin) < MRSLEN:
            raise UsrError("read MRS HEAD error, less %d bytes"%MRSLEN)
        mrsBytes = bytearray(readbin)

        print("mrs version: ", "%02x%02x"%(readbin[1], readbin[0]))
    except Exception:
        raise UsrError("make MRS Version error")

    gm = GmTool(key_dir, temp_dir)
    try:
        gm.sign_for_other_file(binPath["TEE"], gm.tmpDir + "trustedcore.sign")
    except Exception:
        raise UsrError("sign trustedcore error")

    try:
        gm.sign_for_other_file(binPath["KERNEL"], gm.tmpDir + "kernel2.sign")
    except Exception:
        raise UsrError("sign kernel error")

    try:
        gm.sign_for_other_file(binPath["HASH"], gm.tmpDir + "shatable.sign")
    except Exception:
        raise UsrError("sign shatable error")


    mrsBytes[361:425] = ReadPartBytesOfFile(gm.tmpDir + "trustedcore.sign")
    mrsBytes[425:489] = ReadPartBytesOfFile(gm.tmpDir + "kernel2.sign")
    mrsBytes[489:553] = ReadPartBytesOfFile(gm.tmpDir + "shatable.sign")

    crc = crc32(mrsBytes[: (MRSLEN-4)])
    mrsBytes[(MRSLEN-4):MRSLEN]= struct.pack('I', crc)

    DeleteFile(gm.tmpDir + "trustedcore.sign")
    DeleteFile(gm.tmpDir + "kernel2.sign")
    DeleteFile(gm.tmpDir + "shatable.sign")

    return mrsBytes[:MRSLEN]


def make_mrs(flash_all_path, key_dir, temp_dir):
    workdir = os.path.dirname(flash_all_path)

    binTable = ParseConf(flash_all_path)

    binPath = {}
    binPath["HEAD"] = workdir + os.sep + binTable["HEAD"]
    binPath["TEE"] = workdir + os.sep + binTable["TEE"]
    binPath["KERNEL"] = workdir + os.sep + binTable["KERNEL"]
    binPath["HASH"] = workdir + os.sep + binTable["HASH"]

    try:
        mrsBytes = MakeMRS(key_dir, temp_dir, binPath)
    except UsrError as e:
        raise UsrError("error: %s"%e.msg)
    except Exception:
        raise UsrError("error: make Flash Head file failed")

    try:
        SaveData(binPath["HEAD"], mrsBytes)
    except:
        raise UsrError("error: save %s file failed"%binPath["HEAD"])

    return binTable

if __name__ == '__main__':
    parser = argparse.ArgumentParser(formatter_class=argparse.RawDescriptionHelpFormatter, \
    description=textwrap.dedent( \
    '''%(prog)s: signature mrs
eg: %(prog)s -k key_directory flash_all.conf\n '''), epilog='')
    parser.add_argument("-k",         dest='key', required=True,  help = "key directory path")
    parser.add_argument("path",       nargs=1,                    help = "flash_all.conf path")
    parser.add_argument("-v", "--version",  action='version',    version='%(prog)s v1.0.0')

    if len(sys.argv) == 1:
        parser.print_help()
        sys.exit(-1)

    args = parser.parse_args()

    try:
        temp = tempfile.TemporaryDirectory()
        flash_all_path = args.path[0]
        key_dir = args.key
        make_mrs(flash_all_path, key_dir, temp.name)
    except UsrError as e:
        print("error: ", e.msg)
    except:
        print(str(sys.exc_info()))
    else:
        print("update MRS file finish!")



