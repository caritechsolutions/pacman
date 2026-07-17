#encoding=utf-8

import sys
import getopt

def split_yuv(input_file, w, h, bpp, frame_num=0): #{
    fp = open(input_file, 'rb')

    if frame_num == 0:
        if bpp == 8:
            frame_size = w*h*3/2 #8bpp takes 8 bit
        else:
            frame_size = w*h*2*3/2 #10bpp takes 16 bit
        fp.seek(0, 2)
        size = fp.tell()
        fp.seek(0, 0)
        frame_num = int(size/frame_size)

    #frame_num = 2
    print("bpp = ", bpp);
    for n in range(frame_num):
        data_y = []
        data_u = []
        data_v = []

        if bpp == 8:
            #load y data
            for i in range(h):
                for j in range(w):
                    data_y.append(fp.read(1))
            #load u data
            data_u = []
            for i in range(h//2):
                for j in range(w//2):
                    data_u.append(fp.read(1))
            #load v data
            data_v = []
            for i in range(h//2):
                for j in range(w//2):
                    data_v.append(fp.read(1))
        else:
            #load y data
            for i in range(h):
                for j in range(w):
                    val = int.from_bytes(fp.read(2), byteorder='little', signed=False)
                    val = (val>>2)&0xff
                    data_y.append(val.to_bytes(length=1, byteorder='little', signed=False))
            #load u data
            data_u = []
            for i in range(h//2):
                for j in range(w//2):
                    val = int.from_bytes(fp.read(2), byteorder='little', signed=False)
                    val = (val>>2)&0xff
                    data_u.append(val.to_bytes(length=1, byteorder='little', signed=False))
            #load v data
            data_v = []
            for i in range(h//2):
                for j in range(w//2):
                    val = int.from_bytes(fp.read(2), byteorder='little', signed=False)
                    val = (val>>2)&0xff
                    data_v.append(val.to_bytes(length=1, byteorder='little', signed=False))
        #endif

        #write y
        ofp = open(str(n)+".yuv", 'wb')
        for i in range(len(data_y)):
            ofp.write(data_y[i])
        #write uv
        for i in range(len(data_u)):
            ofp.write(data_u[i])
            ofp.write(data_v[i])
        ofp.close()
    fp.close()
    return frame_num
#}

def print_usage(): #{
    print("Split yuv file to single frame, and conver I420 to NV12(yuv420p), V1.0 by zhaogf.")
    print("usage :./split.py <-i> <-w> <-h> [-n]")
    print("\t -i : input file")
    print("\t -n : frame-number")
    print("\t -w : width")
    print("\t -h : height")
#}

if __name__ == '__main__':
    try:
        options, args = getopt.getopt(sys.argv[1:], "i:w:h:n:b:", ["help", "input-file=", "width=", "height=", "number=", "bpp="])
    except getopt.GetoptError:
        sys.exit()

    bpp = 8
    for option, value in options:
        #print(option, value)
        if option in ("", "--help"):
            print_usage()
        if option in ("-i", "--input-file"):
            #print("input_file = ", value)
            input_file = value
        if option in ("-w", "--width"):
            #print("width = ", value)
            w = int(value)
        if option in ("-h", "--height"):
            #print("height = ", value)
            h = int(value)
        if option in ("-n", "--number"):
            #print("number = ", value)
            number = int(value)
        if option in ("-b", "--bpp"):
            print("bit-per-pixel = ", value)
            bpp = int(value)

    if 'input_file' in locals() and 'w' in locals() and 'h' in locals():
        if 'number' in locals():
            pass
            #split_yuv(input_file, w, h, bpp, number)
        else:
            pass
            #split_yuv(input_file, w, h, bpp)
    else:
        print_usage()

