#! /usr/bin/env python3
# -*- coding: UTF-8 -*-

import os
import sys
import argparse
from common import *
import textwrap

def is5CopyFirmware(input):
    copyLen = [0x800, 0x1000, 0x2000, 0x4000, 0x8000, 0xc000]

    for length in copyLen:
        lastMagic = None
        for i in range(5):
            offset = length * i
            try:
                magic = ReadPartBytesOfFile(input, offset, 4)
                if lastMagic == None:
                    if magic != b'\xaa\x55\xaa\x55' and magic != b'\xbb\x55\xbb\x55':
                        break
                    else:
                        lastMagic = magic
                else:
                    if magic != lastMagic:
                        break
            except UsrError as e:
                print("error", e.msg)
                return False, None
            except:
                return False, None
        else:
            return True, length

    return False, None

def ModifyMagic(input, output, magic):
    ret, length = is5CopyFirmware(input)
    if not ret:
        print("error: %s is not 5 copy stage1 firmware, exit."%(input))
        return

    print("stage1 length is 0x%x"%length)
    stage1 = ReadPartBytesOfFile(input, 4, length - 4)
    stage1_one = magic + stage1

    stage1All = bytes()

    for _ in range(5):
        stage1All += stage1_one

    firmware = stage1All + ReadPartBytesOfFile(input, 5 * length)
    SaveData(output, firmware)
    print("save %s ok, magic: %s"%(output, bin2hex(magic)))

if __name__ == '__main__':
    parser = argparse.ArgumentParser(formatter_class=argparse.RawDescriptionHelpFormatter, \
    description=textwrap.dedent( \
    '''%(prog)s modify image file header maigc
eg: mkmagic.py -a -i output/loader-spinand.bin -o loader.bin
eg: mkmagic.py -b -i output/loader-spinand.bin -o loader.bin''' \
            ), epilog='')
    parser.add_argument("-a", "--aa55aa55", action="store_true", dest="a5", help = "modify header to aa55aa55")
    parser.add_argument("-b", "--bb55bb55", action="store_true", dest="b5", help = "modify header to bb55bb55")
    parser.add_argument("-i", "--input",    action="store",      required=True, dest="input", help = "input file to be modified")
    parser.add_argument("-o", "--onput",    action="store",      required=True, dest="output", help = "output file to be saved")
    parser.add_argument("-v", "--version",  action='version',    version='%(prog)s 1.0')

    if len(sys.argv) == 1:
        parser.print_help()
        exit()

    args = parser.parse_args()

    if args.a5 and args.b5:
        parser.error("options (-a OR -b) are mutually exclusive")
        exit()

    if args.a5:
        ModifyMagic(args.input, args.output, b'\xaa\x55\xaa\x55')
        exit()

    if args.b5:
        ModifyMagic(args.input, args.output, b'\xbb\x55\xbb\x55')
        exit()



