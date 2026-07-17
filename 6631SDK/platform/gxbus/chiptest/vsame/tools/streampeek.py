#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import subprocess
import json
import sys
import os
import re
import hashlib
import argparse
import yuvsplit

debug_modle = True 
video_codec_tools = {'h264': './codec/AvcDec -o c_disp.yuv -r -i ',
        'mpeg2video':'./codec/Mp2Dec -o c_disp.yuv -i ',
        'hevc':'./codec/HevcDec -p c_disp.yuv -b ',
        'mpeg4':'./codec/Mp4Dec -o c_disp.yuv -i ',
        'vc1':'./codec/Vc1Dec -o c_disp.yuv -i ',
        'vp8':'./codec/Vp8Dec -o c_disp.yuv -i ',
        'div':'./codec/DivDec -o c_disp.yuv -i ',
        'avs':'./codec/AvsDec -dbg 0 -cfg ./codec/ldecod.cfg -i ',
        'None':'',
        }

#file_path = sys.argv[1]
def peak_description(filepath):
    ffprobe = "ffprobe -v quiet -print_format json -show_format -show_streams " +  '"' + filepath + '"'
    try:
        stream_description = subprocess.check_output(ffprobe, shell=True, universal_newlines=True)
    except subprocess.CalledProcessError:
        return None
        
    stream_info = json.loads(stream_description)
    return stream_info

def sha256sum(filepath, avinfo):
    global debug_modle

    w = int(avinfo['width'])
    h = int(avinfo['height'])

    mediahash = b''
    with open(filepath, 'rb') as fd:
        for i in range(h*3//2):
            data = fd.read(w)
            h = hashlib.new('sha256', data)
            value = h.digest()
            #print(filepath, i, value.hex())
            mediahash += value
            if debug_modle:
                print('frame: %s, line: %d, data: %s' % (filepath, i, data.hex()))
                print('frame: %s, line: %d, hash: %s' % (filepath, i, value.hex()))
    h = hashlib.new('sha256', mediahash)
    hashvalue = h.digest().hex()
    if debug_modle:
        print('framehash: %s' % (hashvalue))
        print("==========================")
        print('line-hashs: %s' % mediahash.hex())
    
    #shahash = " sha256sum -b " +  '"' + filepath + '"'
    #try:
    #    hashvalue = subprocess.check_output(shahash, shell=True, universal_newlines=True)
    #except subprocess.CalledProcessError:
    #    print(filepath,'error')
    #    return None
    #hashvalue = hashvalue.split()[0]
    #print(hashvalue, len(hashvalue))
    return hashvalue


def get_value( key, dict_info, key_parent = None):
    if dict_info == None:
        return None
    if key_parent == None and key in dict_info.keys():
        return dict_info[key]
    if key_parent != None and key_parent in dict_info.keys():
        if key in dict_info[key_parent].keys():
            return dict_info[key_parent][key]
    return None

def peak_video(file_path):
    video_info = {}
    stream_info = peak_description(file_path)
    streams = get_value('streams', stream_info)
    if streams:
        for stream in streams:
            if get_value('codec_type', stream).lower() == 'video':
                video_info['codec'] = get_value('codec_name', stream)
                video_info['width'] = get_value('width', stream)
                video_info['height'] = get_value('height', stream)
                video_info['height'] = get_value('height', stream)
                if get_value('profile', stream) == "Main 10":
                    video_info['bpp'] = 10
                else:
                    video_info['bpp'] = 8
                video_info['hash'] = list()
                break
    else:
        filename = os.path.basename(file_path)
        solution = re.sub('.*_(\d+[xX]\d+).*', '\g<1>', filename)
        if solution != filename:
            solution = solution.lower().split('x')
            width = solution[0]
            height = solution[1]
        if filename.lower().endswith('.avs'):
            video_info['codec'] = 'avs'
        #add other codec here

        if 'codec' in video_info.keys():
            video_info['width'] = width
            video_info['height'] = height
            video_info['frames'] = 0
            video_info['hash'] = list()
        else:
            print('Peek %s failed !!' %file_path)
    return video_info
    


def peak_audio(file_path):
    audio_info = {}
    stream_info = peak_description(file_path)
    streams = get_value('streams', stream_info)
    if streams:
        for stream in streams:
            if get_value('codec_type').lower() == 'audio':
                video_info['codec'] = get_value('codec_name', stream)
                break
    return audio_info

def video_decode(sfile, codec, max_frames = 0):
    if max_frames == 0:
        cmd = video_codec_tools[codec] + '"' + sfile + '"'
    else:
        cmd = video_codec_tools[codec] + '"' + sfile + '"' + ' -m ' + str(max_frames)
    os.system(cmd)
    return 'c_disp.yuv'
    

def audio_decode(sfile, codec):
    pass

def peak_onefile(sfile, tfile, avtype, max_frames = 0):
    '''
    sfile:
    tfile:
    avtype: audio or video
    '''
    print('Processing %s ..........' %sfile)
    global debug_modle
    if avtype == "video":
        videoinfo = peak_video(sfile)
        if videoinfo == {}:
            return None
        if videoinfo['codec'] == None:
            return None
        if videoinfo['codec'].lower() not in video_codec_tools.keys():
            print("ToDo warming file %s of %s no tool !!!!!!!!!" %(sfile, videoinfo['codec'].lower()))
            return None
        yuvfilename = video_decode(sfile, videoinfo['codec'].lower(), max_frames)
        # ToDo yuv to one frame
        framenumber = yuvsplit.split_yuv('c_disp.yuv', int(videoinfo['width']), int(videoinfo['height']), int(videoinfo['bpp']))

        for i in range(0,framenumber):
            framefile = str(i) + '.yuv'
            hashvalue = sha256sum(framefile, videoinfo)
            videoinfo['hash'].append(hashvalue)
            
            if not debug_modle:
                os.remove(framefile)
        with open(tfile, 'w') as fd:
            videoinfo['frames'] = framenumber
            jsons = json.dumps(videoinfo)
            fd.write(jsons)
        return videoinfo

def update_streamjson(fullinfo, avfile, jsonfile):
    if fullinfo == None:
        return
    streaminfo = {}
    codecname = fullinfo['codec']
    if not os.path.exists(jsonfile):
        streaminfo[codecname] = [ avfile ]
    else:
        with open(jsonfile, 'r') as fd:
            streaminfo = json.load(fd)
            if codecname in streaminfo.keys():
                if not avfile in streaminfo[codecname]:
                    streaminfo[codecname].append(avfile)
            else:
                streaminfo[codecname] = [ avfile ]
    with open(jsonfile,'w') as fd:
        data = json.dumps(streaminfo)
        fd.write(data)

def main( dirpath, avtype, force = False , max_frames = 0):
    abs_dirpath = os.path.abspath(dirpath)
    root_path = os.path.dirname(abs_dirpath)
    stream_json = os.path.join(root_path,'stream_' + avtype + '.json')
    stream_info_basename = os.path.basename(abs_dirpath) + '_info'
    stream_info_path = os.path.join(root_path, stream_info_basename)
    
    if not os.path.isdir(dirpath):
        print("%s is not a directory" %dirpath)
        return
    for root, dirs, files in os.walk(dirpath):
        for filename in files:
            sfile = os.path.join(root, filename)
            tfile = sfile + '.json'
            tfile = '/'.join(tfile.split('/')[1:])
            #tfile = re.sub(dirpath, '', sfile + '.json') #check is python3.7 ok
            #if tfile[0] == '/':
            #    tfile = tfile[1:]
            tfile = os.path.join(stream_info_path, tfile)
            print(tfile)
            if force == False and os.path.exists(tfile):
                continue
            if not os.path.exists(os.path.dirname(tfile)):
                    os.makedirs(os.path.dirname(tfile))
            print(sfile)
            avinfo = peak_onefile(sfile, tfile, avtype, max_frames = max_frames)
            if avinfo == {}:
                #print('Warning %s peak failed !!' % sfile, file=stderror)
                print('Warning %s peak failed !!' % sfile)
                continue
            update_streamjson(avinfo, sfile, stream_json)



if __name__ == '__main__':
    parser = argparse.ArgumentParser(description = "")
    parser.add_argument('-d', dest='stream_dir', help='码流对应路径')
    parser.add_argument('-t',  dest='type', choices=['audio', 'video'], default = 'video', help='指定音视频类型,默认为视频')
    parser.add_argument('-m',  dest='max',  default = '0', help='最大解码帧数量')
    parser.add_argument('--debug',  dest='debug', default = False, nargs='?', const=True, help='调试模式')
    if len(sys.argv) == 1:
        sys.argv.append('-h')
    args = parser.parse_args()
    debug_modle = args.debug
    main(args.stream_dir, args.type, max_frames = int(args.max))
    #sha256sum('./streampeak.py')



