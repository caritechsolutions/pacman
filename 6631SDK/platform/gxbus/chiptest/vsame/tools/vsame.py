#!/usr/bin/env python
# -*- coding:utf-8 -*-
import socketserver as SocketServer
import time
import threading
import json
import shutil
import sys
import os
import re
import argparse
from struct import *
import vsame_config
from importlib import reload

stream_list_json = ''
root_dir = ''
testavtype = 'video'
teststreamtype = 'ES'
report_json = ''
report_json_bak = ''

av_codec = {'video':{
        'mpeg2video':0,
        'h264':1,
        'hevc':2,
        'mpeg4':3,
        'avs':4,
        'div3':5
        }
        }

mediatype = { 'video':0, 'audio':1}
streamtype = {'TS':0, 'ES':1}
commands = {'decode':0,
        'decodeack':1,
        'pushdata':2,
        'framehash':3,
        'stop':4,
        'stopack':5,
        'hashover':6,
        }

debug = 1
frame_number = [0, 0]

class Command():

    head_fmt = '!IBBI'
    receive_head_fmt = '=IBBI'
    def __init__(self):
        self.magic = 0x55667788
        self.cmdheadsize = 10
        pass

    def pack(self, cmd, *argv):
        data = None
        if cmd == 'decode':
            data = self.decode_pack(argv[0],argv[1])
        elif cmd == 'stop':
            data = self.stop_pack(argv[0])
        elif cmd == 'stopack':
            data = self.stop_ack_pack(argv[0])
        elif cmd == 'decodeack':
            data = self.decode_ack_pack(argv[0])
        elif cmd == 'framehash':
            data = self.frame_hash_pack(argv[0],argv[1],argv[2],argv[3])
        elif cmd == 'pushdata':
            data = self.pushdata_pack(argv[0], argv[1], argv[2], argv[3])
        else:
            data = None
        return data


    def uppack(self):
        pass

    def decode_pack(self,clientid, codec):
        fmt = '!IBBIBB'
        cmdid = commands['decode']
        mtype = mediatype[testavtype]
        codecid = av_codec[testavtype][codec]
        data = pack(fmt,self.magic,cmdid,clientid,2,mtype,codecid)
        return data

    def stop_pack(self,clientid):
        fmt = self.head_fmt
        cmdid = commands['stop']
        data = pack(fmt,self.magic,cmdid,clientid,0)
        return data

    def stop_ack_pack(self,clientid):
        fmt = self.head_fmt
        cmdid = commands['stopack']
        data = pack(fmt,self.magic,cmdid,clientid,0)
        return data

    def decode_ack_pack(self,clientid):
        fmt = self.head_fmt
        cmdid = commands['decodeack']
        data = pack(fmt,self.magic,cmdid,clientid,0)
        return data

    def frame_hash_pack(self,clientid, frameid, isfinish,hashvalue):
        fmt = '!IBBIIB32s'
        cmdid = commands['framehash']
        data = pack(fmt,self.magic,cmdid,clientid,37, frameid, isfinish, hashvalue)
        return data

    def pushdata_pack(self, clientid, stype, eos, avdata = None):
        if avdata == None:
            fmt = '!IBBIBB'
            dlen = 2
        else:
            fmt = '!IBBIBB%ds' %(len(avdata))
            dlen = len(avdata) + 2

        cmdid = commands['pushdata']
        stypeid = streamtype[teststreamtype]
        if avdata == None:
            data = pack(fmt,self.magic,cmdid,clientid,dlen, stypeid,eos)
        else:
            data = pack(fmt,self.magic,cmdid,clientid,dlen, stypeid,eos, avdata)
            #data = pack(fmt,self.magic,cmdid,clientid,dlen, stypeid,eos, avdata.encode(encoding='utf-8'))
        return data

    def head_unpack(self,data):
        if len(data) < self.cmdheadsize:
            return None
        head_data = data[0:4]
        fmt = self.receive_head_fmt
        result = unpack(fmt, data)
        return {'magic':result[0], 'cmdid':result[1], 'clientid':result[2], \
                'dlen':result[3]}

    def frame_hash_unpack(self, data):
        if len(data) < 37:
            return None
        fmt = '=IB32s'
        result = unpack(fmt, data)
        return {'frameid':result[0], 'finish':result[1],'hash':result[2]}

    def decode_unpack(self, data):
        if len(data) < 2:
            return None
        fmt = '!BB'
        result = unpack(fmt, data)
        return {'mediatype':result[0], 'codec':result[1]}

    def pushdata_unpack(self, data):
        dlen = len(data)
        if dlen < 2:
            return None
        if dlen == 2:
            fmt = '!BB'
            result = unpack(fmt, data)
            return {'streamtype':result[0], 'eos':result[1]}
        else:
            fmt = '!BB%ds' %(dlen - 2)
            result = unpack(fmt, data)
            return {'streamtype':result[0], 'eos':result[1], 'data':result[2]}


connecttimes = 0
class MyServer(SocketServer.BaseRequestHandler):

    clients = {}
    cmdheadsize = 10

    def send_data_thread(self,filename, config):

        fd = open(filename, 'rb')
        packet_max_size = 16*1024
        config['issend'] = False
        i = 0
        print('channel:%d, file: send data %s  start' %(config['id'],filename))
        total_len = 0
        #import pdb
        #pdb.set_trace()
        eos = 0
        while True:
            i += 1
            data = fd.read(packet_max_size)
            #data = 'aaaa'
            readsize = len(data)
            total_len += readsize
            if readsize < packet_max_size or config['issend']:
                eos = 1
            self.send_data(config, teststreamtype, eos, data)
            #print('channel:%d eos:%d after send_data' %(config['id'], eos))
            #time.sleep(1)
            if debug:
                if frame_number[config['id'] - 1] == 100 and eos != 1:
                    eos = 1
                    continue
                print('channel:%d,Send data len:%d,eos: %d' %(config['id'],total_len,eos))
            if eos:
                #data = config['command'].pack('stop', 0)
                #config['request'].sendall(data)
                print('channel:%d, file: %s, send data done' %(config['id'],filename))
                #self.stop_decode(config)
                break
        fd.close()


    def start_decode(self,config, codec):
        conn = config['request']
        #av = avtype[av]
        #codecid = av_codec[av][codec]
        data = config['command'].pack('decode', config['id'], codec)
        conn.sendall(data)
        if debug:
            print('channel:%d, send decode cmd' %(config['id']))
        while True:
            #import pdb
            #pdb.set_trace()
            rdata = conn.recv(1024)
            #if debug:
            #    print('receive:', data)
            if len(rdata) < self.cmdheadsize:
                continue
            cmd = config['command'].head_unpack(rdata[0:self.cmdheadsize])
            #if debug:
            #    print(cmd)
            if cmd['cmdid'] == commands['decodeack']:
                ret = True
                break
            #else:
            #    ret = False
            #break
        return ret

    def send_data(self, config, dtype, eos, data):
        conn = config['request']
        data = config['command'].pack('pushdata', config['id'], dtype, eos, data)
        conn.sendall(data)
        return True

    def stop_decode(self, config):
        conn = config['request']
        data = config['command'].pack('stop', config['id'])
        print("stop decode")
        conn.sendall(data)
        while True:
            rdata = conn.recv(1024)
            if len(rdata) < self.cmdheadsize:
                continue
            cmd = config['command'].head_unpack(rdata[0:self.cmdheadsize])
            if cmd['cmdid'] == commands['stopack']:
                ret = True
            else:
                ret = False
            break
        return ret

    def run_decode(self, config, filename):
        t = threading.Thread(target = self.send_data_thread, args = (filename, config))
        t.start()
        return t

    def get_command(self, config):
        #self.comdata
        data_need = 0
        conn = self.request
        dlen = len(config['cmddata'])
        import pdb
        #pdb.set_trace()
        while True:
            #data = conn.recv(1024)
            #print(data)
            if dlen < self.cmdheadsize or data_need:
                data = conn.recv(1024)
                #print('.......................')
                #print(data.hex())
                #print('!!!!!!!!!!!!!!!!!!!!!!!')
                #print("receive:%d" %(len(data)))
                config['cmddata'] += data
                dlen = len(config['cmddata'])
                if dlen < self.cmdheadsize:
                    continue
            cmd = config['command'].head_unpack(config['cmddata'][0:self.cmdheadsize])
            #if debug:
            #    print(cmd)
            if  dlen < cmd['dlen'] + self.cmdheadsize:
                data_need = 1
                continue
            break
        data = config['cmddata'][self.cmdheadsize: self.cmdheadsize + cmd['dlen']]
        config['cmddata'] = config['cmddata'][self.cmdheadsize + cmd['dlen']:]
        if cmd['cmdid'] == commands['framehash']:
            cmdhash = config['command'].frame_hash_unpack(data)
            cmd.update(cmdhash)
            #if debug:
            #    print(cmdhash['frameid'], cmdhash['hash'].hex())
        if cmd['cmdid'] == commands['stopack']:
            print('one File Finished !!')
        return cmd


    def check_file(self, filename):
        streamfile = os.path.join(root_dir, filename)
        if os.path.exists(streamfile):
            return True, streamfile
        return False, streamfile


    def update_result(self, config, result):
        with open(config['reportjson'], 'r') as fd:
            all_results = json.load(fd)
        all_results['results'].append(result)
        shutil.copyfile(config['reportjson'], config['reportjsonbak'])
        with open(config['reportjson'], 'w') as fd:
            fd.write(json.dumps(all_results))

    def one_test(self, config, codename, filename):
        global frame_number
        frame_number[config['id']-1] = 0
        streaminfo = filename.split('/') #路径格式以 ./或../时会出错
        streaminfo[0] = streaminfo[0] + '_info'
        streaminfo = '/'.join(streaminfo) + '.json'
        #streaminfo = re.sub(streaminfo, streaminfo+'_info', filename + '.json')
        isstreamexist, streamfile = self.check_file(filename)
        isinfoexist, streaminfo = self.check_file(streaminfo)
        if (not isstreamexist) or (not isinfoexist):
            return

        if not os.path.exists(config['reportjson']):
            result = {'results':[]}
            with open(config['reportjson'], 'w') as fd:
                fd.write(json.dumps(result))
        with open(streaminfo, 'r') as fd:
            streamdetails = json.load(fd)
            streamresult = {'stream':filename}
            streamresult['pc-frame'] = streamdetails['frames']
            streamresult['board-frame'] = 0
            streamresult['diff-frame'] = []
            streamresult['result'] = 'Pass'

        clientid = config['id']
        #codec = 'mpeg2video'
        codec = codename
        self.start_decode(config, codec)
        thread = self.run_decode(config, filename)
        while True:
            cmd = self.get_command(config)
            if cmd == None:
                continue
            #if debug:
            #    print(cmd)
            if cmd['cmdid'] == commands['framehash']:
                streamresult['board-frame'] += 1
                frame_num = cmd['frameid']
                hashvalue = cmd['hash']
                frame_number[config['id'] - 1] += 1
                #print(cmd, hashvalue.hex())
                #print(frame_num, streamdetails['frames'],cmd['finish'], '.................') 
                #if frame_num > 30:
                #    import pdb
                #    pdb.set_trace()
                if frame_num >= streamdetails['frames'] and cmd['finish'] != 1:
                    if config['issend'] == False:
                        config['issend'] = True
                    continue
                if  frame_num < streamdetails['frames'] and hashvalue.hex() != streamdetails['hash'][frame_num]:
                    streamresult['diff-frame'].append(frame_num)
                    streamresult['result'] = 'Failed'
                #if cmd['finish'] or frame_num >= 10:
                #if cmd['finish']:
                #    #import pdb
                #    #pdb.set_trace()
                #    if len(streamresult['diff-frame']) != 0:
                #        streamresult['result'] = 'Failed'
                #    data = config['command'].pack('stop', 0)
                #    config['request'].sendall(data)
            #print(cmd)
            if cmd['cmdid'] == commands['hashover']:
                print(config['id'],' Get hash over Command!')
                if len(streamresult['diff-frame']) != 0:
                    streamresult['result'] = 'Failed'
                data = config['command'].pack('stop', 0)
                config['request'].sendall(data)
            if cmd['cmdid'] == commands['stopack']:
                #thread.join()
                print('update_result')
                import pdb
                #pdb.set_trace()
                print('channel: ',config['id'], streamresult)
                self.update_result(config, streamresult)
                print('channel: %d update_result done!!' %config['id'])
                break;


    def run_test(self, config):
        with open(config['streamjson'], 'r') as fd:
            stream_list = json.load(fd)
        if os.path.exists(config['reportjson']):
            os.remove(config['reportjson'])
        if os.path.exists(config['reportjsonbak']):
            os.remove(config['reportjsonbak'])
        for codec, streamfiles in stream_list.items():
            #print(config['id'], codec)
            #print('***********************************************')
            #print(streamfiles)
            #print('***********************************************')
            for streamfile in streamfiles:
                #print('runing %s ............' %streamfile)
                self.one_test(config, codec, streamfile)
                #print('%s done' %streamfile)

    def handle(self):
        #n = 10 
        #while n:
        #    time.sleep(2)
        #    print (self.request,self.client_address,self.server)
        #    str_c_addr = str(self.client_address)
        #    fmt = '%ds'%(len(str_c_addr))
        #    data = pack(fmt, str.encode(str_c_addr))
        #    self.request.sendall(data)
        #    n -= 1

        #import pdb
        #pdb.set_trace()

        global connecttimes
        client_config = reload(vsame_config)
        connecttimes += 1
        client_num = len(client_config.client_config)
        clientid = (connecttimes - 1)%client_num + 1
        config = client_config.client_config[str(clientid)]
        if 'avtype' not in config:
            config['avtype'] = 'video'
        if 'streamtype' not in config:
            config['streamtype'] = 'ES'
        config['streamjson'] = os.path.abspath(config['streamjson'])
        rootdir = os.path.dirname(config['streamjson'])
        config['reportjson'] = os.path.join(rootdir, str(clientid) + '_report.json')
        config['reportjsonbak'] = os.path.join(rootdir, str(clientid) + '_report.json.bak')
        if debug:
            print('clientid: %d, client_num: %d, connecttimes: %d' %(clientid, client_num, connecttimes))
            print(config)
        config['command'] = Command()
        config['id'] = clientid
        config['request'] = self.request
        config['cmddata'] = b''
        config['issend'] = False
        self.run_test(config)

        #self.command = Command()
        #self.cmddata = b''
        #self.issend = False
        #self.cmdheadsize = 10
        ##import pdb
        ##pdb.set_trace()
        #self.run_test()

        #while True:
        #    self.one_test('aaaaa')


def main(streamjson, avtype='video', port = 8008, streamtype = 'ES'):
    if (not os.path.exists(streamjson)) or (not os.path.isfile(streamjson)):
        print('%s is not exists or is not a file !' %streamjson)
        return
    global stream_list_json,root_dir,report_json
    global report_json_bak,testavtype, teststreamtype
    stream_list_json = os.path.abspath(streamjson)
    root_dir = os.path.dirname(stream_list_json)
    report_json = os.path.join(root_dir, 'report.json')
    report_json_bak = os.path.join(root_dir, 'report.json.bak')
    testavtype = avtype
    teststreamtype = streamtype
    server = SocketServer.ThreadingTCPServer(('192.168.31.38', port),MyServer)
    server.serve_forever()

if __name__ == '__main__':
    #parser = argparse.ArgumentParser(description = "")
    #parser.add_argument('-f', dest='streamjson', help='含有码流列表信息的json文件路径')
    #parser.add_argument('-t',  dest='avtype', choices=['audio', 'video'], default = 'video', help='指定音视频类型,默认为视频')
    #parser.add_argument('-s',  dest='streamtype', choices=['ES', 'TS'], default = 'ES', help='指定码流频类型,默认为 ES 流')
    #parser.add_argument('-p',  dest='port', default = '8008', help='指定码网络监听端口，默认为8008')
    #if len(sys.argv) == 1:
    #    sys.argv.append('-h')
    #args = parser.parse_args()
    #main(args.streamjson, args.avtype, int(args.port), args.streamtype)
    server = SocketServer.ThreadingTCPServer(('192.168.31.38', 8008),MyServer)
    server.serve_forever()


