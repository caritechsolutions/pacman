#!/usr/bin/env python
# -*- coding:utf-8 -*-

import socket
import sys
import threading
import hashlib
import time
from vsame import Command


cdata = b''

CMD = Command()

ip_port = ('192.168.188.166', int(sys.argv[1]))
sk = socket.socket()
sk.connect(ip_port)

finish = 0
def get_data(argv):
    global finish
    soc = argv
    isfinish = 0
    maxframe = 100
    for i in range(0, maxframe):
        filename = str(i) + '.yuv'
        with open(filename, 'rb') as fd:
            data = fd.read()
            #print(len(data))
            h = hashlib.new('sha256', data)
            value = h.digest()
            if i == maxframe - 1:
                isfinish = 1
            cmd = CMD.pack('framehash', 0, i, isfinish, value)
            print(filename, value)
            soc.sendall(cmd)
    finish = 1
    print('File finish !!!!')

#sk.settimeout(1000)
#sk.setblocking(False)


i = 0
cmddata = b''
cmdheadsize = 10
get_stop_cmd = 0
while True:
        if get_stop_cmd == 1 and finish == 0:
            continue
        if get_stop_cmd == 1 and finish == 1:
                data = CMD.pack('stopack', 0)
                sk.sendall(data)
                time.sleep(1)
                print('Test finish!')
                break

        dlen = len(cmddata)
        if dlen < cmdheadsize and get_stop_cmd == 0:
            import pdb
            #pdb.set_trace()
            data = sk.recv(1024)
            cmddata += data
            dlen = len(cmddata)
            if dlen < cmdheadsize:
                continue
        cmd = CMD.head_unpack(cmddata[0:cmdheadsize])
        print(cmd)
        dlen = len(cmddata)
        if  dlen < cmd['dlen'] + cmdheadsize and get_stop_cmd == 0:
            data = sk.recv(1024)
            cmddata += data
            continue
        #print(cmd)
        data = cmddata[cmdheadsize: cmdheadsize + cmd['dlen']]
        cmddata = cmddata[cmdheadsize + cmd['dlen']:]
        if cmd['cmdid'] == 0:
            decodecmd = CMD.decode_unpack(data)
            cmd.update(decodecmd)
            #print(cmd)
            data = CMD.pack('decodeack', 0)
            sk.sendall(data)
            t = threading.Thread(target = get_data, args=(sk,))
            t.start()
        if cmd['cmdid'] == 2:
            #print(cmd)
            dlen = cmd['dlen']
            pushdatacmd = CMD.pushdata_unpack(data)
            #print(pushdatacmd)
        if cmd['cmdid'] == 4:
            #print(cmd)
            get_stop_cmd = 1
#    data = sk.recv(65*1024)
#    cmddata += data
#    dlen = len(cmddata)
#    if dlen >= 10:
#        cmd = CMD.head_unpack(cmddata[0:10])
#        #print(cmd)
#        if cmd:
#            print('Command Id: %d' %cmd['cmdid'])
#            if cmd['cmdid'] == 0:
#                decodecmd = CMD.decode_unpack(cmddata[10:11])
#                cmddata = cmddata[12:]
#                #print(decodecmd)
#                data = CMD.pack('decodeack', 0)
#                sk.sendall(data)
#                t = threading.Thread(target = get_data, args=(sk,))
#                t.start()
#            if cmd['cmdid'] == 2:
#                dlen = cmd['dlen']
#                pushdatacmd = CMD.pushdata_unpack(data[10:10+dlen])
#                cmddata = cmddata[10+dlen:]
#                #print(pushdatacmd)
#            if cmd['cmdid'] == 4:
#                data = CMD.pack('stopack', 0)
#                sk.sendall(data)
    #i += 1
    #data = CMD.pack('framehash', 0, i, '123456789012345678901234567890qw')
    #sk.sendall(data)

    #inp = input('please input:')
    #sk.sendall(inp.encode(encoding = 'utf-8'))
    #if inp == 'exit':
    #    break
    #if inp == 'goon':
    #    i = 100
    #    while i > 0:
    #        i = i - 1
    #        data = sk.recv(1024)


sk.close()



        

