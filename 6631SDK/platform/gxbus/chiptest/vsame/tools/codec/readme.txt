./Mp2Dec -i 0000.mpeg2.es -o c_disp.yuv -m 1000(指定解码帧数量)
./Mp4Dec -i 0000.mpeg4.es -o c_disp.yuv -m 1000(指定解码帧数量)
./DivDec -i 0000.div.es -o c_disp.yuv -m 1000(指定解码帧数量)
./AvcDec -i 0000.h264.es -o c_dec.yuv -m 1000(指定解码帧数量) -r
./AvsDec -i 0000.avs.es -cfg ldecod.cfg -m 1000(指定解码帧数量) -dbg 0
./Vc1Dec -i 0000.vc1.es -o c_disp.yuv -m 1000(指定解码帧数量)
./Vp8Dec -i 0000.vp8.es -o c_disp.yuv -m 1000(指定解码帧数量)
./HevcDec -b 0000.h265.es -p c_dec.yuv -m 1000

注意: 
1. avs比较特殊，还要包含一个cfg文件，解码结果只能放在当前目录下，不能指定yuv名称,解码结果包含c_dec.yuv和c_disp.yuv
2. 解码结果都是YUV格式，c_dec.yuv表示解码顺序的yuv， c_disp.yuv表示播放顺序的yuv
3. 解码结果都是Y一片，U一片，V一片，YUV是分开存放的


