## IPTV配置项

  <table style="float:left" border="1">
  <tr>
    <td rowspan="5">User-Agent:<br/>
    <td>使用方式：</td>
    <td>url -H User-Agent:"VLC/3.0.8 LibVLC/3.0.8"</td>
  </tr>
  <tr>
    <td>参数意义：</td>
    <td>告诉网站服务器，访问者是通过什么工具来请求</td>
  </tr>
  <tr>
    <td style="word-wrap:break-word;word-break:break-all;" width="110px";>范     围：</td>
    <td>V2默认设置:Lavf 51.14.0</td>
  </tr>
  <tr>
    <td>应用场景：</td>
    <td>有些服务器有做黑白名单限制;<br/>如果使用通用的方式会被服务器拒绝掉连接;<br/>(常用于一些网站，做了防盗链的处理)</td>
  </tr>
  <tr>
    <td style="word-wrap:break-word;word-break:break-all;" width="110px";>关联问题：</td>
    <td>#278968</td>
  </tr>
  <tr>
    <td rowspan="5">Connection:<br/>
    <td>使用方式：</td>
    <td>url -H Connection:"keep-alive"<br/>url -H Connection:"close"</td>
  </tr>
  <tr>
    <td>参数作用：</td>
    <td>决定当前的事务完成后,是否会关闭网络连接;<br/>如果该值是“keep-alive”,网络连接就是持久连接;</td>
  </tr>
  <tr>
    <td style="word-wrap:break-word;word-break:break-all;" width="110px";>范     围：</td>
    <td>V2默认设置:keep-alive</td>
  </tr>
  <tr>
    <td>应用场景：</td>
    <td>有些服务器每次请求时要求关闭连接,<br/>重新再创建链接后才能取数据;</td>
  </tr>
  <tr>
    <td style="word-wrap:break-word;word-break:break-all;" width="110px";>关联问题：</td>
    <td>None</td>
  </tr>
  <tr>
    <td rowspan="5">Referer:<br/>
    <td>使用方式：</td>
    <td>url -H Referer:"https://www.hotstar.com/in/1260"</td>
  </tr>
  <tr>
    <td>参数作用：</td>
    <td>告诉服务器该网页是从哪个页面链接过来的;<br/>如果服务器要求必须是某个地址或者某几个地址才能访问,<br/>而你发送的referer不符合他的要求或者不发送,<br/>就会拦截或者跳转到他要求的地址,<br/>然后再通过这个地址进行访问;</td>
  </tr>
  <tr>
    <td style="word-wrap:break-word;word-break:break-all;" width="110px";>范     围：</td>
    <td>V2默认设置:不设置</td>
  </tr>
  <tr>
    <td>应用场景：</td>
    <td>常用于一些网站，做了防盗链的处理;</td>
  </tr>
  <tr>
    <td style="word-wrap:break-word;word-break:break-all;" width="110px";>关联问题：</td>
    <td>#263368</td>
  </tr>
  <tr>
    <td rowspan="5">no_cache_flag:<br/>
    <td>使用方式：</td>
    <td>url -H no_cache_flag:1<br/>url -H no_cache_flag:0</td>
  </tr>
  <tr>
    <td>参数作用：</td>
    <td>不缓区数据，读多少播多少;</td>
  </tr>
  <tr>
    <td style="word-wrap:break-word;word-break:break-all;" width="110px";>范     围：</td>
    <td>默认设置:0;,正常Value:0、1<br/>如果设置1:不需要cache,即不校验数据,读多少数据就直接探测;</td>
  </tr>
  <tr>
    <td>应用场景：</td>
    <td>常用于一直局域网内的直播点播,或者自建的服务器服务;</td>
  </tr>
  <tr>
    <td style="word-wrap:break-word;word-break:break-all;" width="110px";>关联问题：</td>
    <td>#247582</td>
  </tr>
  <tr>
    <td rowspan="5">net_stream_live_mode:<br/>
    <td>使用方式：</td>
    <td>url -H net_stream_live_mode:1  or  url</td>
  </tr>
  <tr>
    <td>参数作用：</td>
    <td>如果该URL会进入lavf的函数中，不进行av_find_stream_info精准查找判断,不获取总时长;加快起播速度;</td>
  </tr>
  <tr>
    <td style="word-wrap:break-word;word-break:break-all;" width="110px";>范     围：</td>
    <td>默认设置:0;,正常Value:1、Other;</td>
  </tr>
  <tr>
    <td>应用场景：</td>
    <td>常用于一直局域网内的直播,或者自建的直播服务器服务;</td>
  </tr>
  <tr>
    <td style="word-wrap:break-word;word-break:break-all;" width="110px";>关联问题：</td>
    <td>#297446</td>
  </tr>
  <tr>
    <td rowspan="5">transport_type:<br/>
    <td>使用方式：</td>
    <td>url -H transport_type:tcp<br/>url -H transport_type:udp</td>
  </tr>
  <tr>
    <td>参数作用：</td>
    <td>选择rtsp的传输类型(tcp or udp),加快起播速度;<br/>(正常流程：先udp probe,fail后，再试tcp )</td>
  </tr>
  <tr>
    <td style="word-wrap:break-word;word-break:break-all;" width="110px";>范     围：</td>
    <td>V2默认设置:udp</td>
  </tr>
  <tr>
    <td>应用场景：</td>
    <td>有些服务器要求使用udp传输而不支持tcp方式;<br/>也有一些客户要求起播快一些，不要太慢;</td>
  </tr>
  <tr>
    <td style="word-wrap:break-word;word-break:break-all;" width="110px";>关联问题：</td>
    <td>#277009</td>
  </tr>
  <tr>
    <td rowspan="5">Set-Cookie:<br/>
    <td>使用方式：</td>
    <td>url -H Set-Cookie:"dmvk=5c90e0cbebf99; ts=844932; v1st=310773DE0FF3F6"</td>
  </tr>
  <tr>
    <td>参数作用：</td>
    <td>需要把cookie中的信息(sessionid,有效期等)回传服务器校验;<br/>sessionid保证了server and client唯一通信凭证,<br/>server根据client send sessionid作为唯一的key找到对应用户,<br/>用来区别和查询用户信息;</td>
  </tr>
  <tr>
    <td style="word-wrap:break-word;word-break:break-all;" width="110px";>范     围：</td>
    <td>V2默认设置:不设置,取决定服务器是否有响应cookie;</td>
  </tr>
  <tr>
    <td>应用场景：</td>
    <td>常用于一些网站，做了防盗链的处理;</td>
  </tr>
  <tr>
    <td style="word-wrap:break-word;word-break:break-all;" width="110px";>关联问题：</td>
    <td>#142927</td>
  </tr>
  <tr>
    <td rowspan="5">sdp_media_info:<br/>
    <td>使用方式：</td>
    <td>url -H sdp_media_info:"***"</td>
  </tr>
  <tr>
    <td>参数作用：</td>
    <td>生成的sdp信息，整合成的字符串传递下来;</td>
  </tr>
  <tr>
    <td style="word-wrap:break-word;word-break:break-all;" width="110px";>范     围：</td>
    <td>V2默认设置:无，上层必须设置</td>
  </tr>
  <tr>
    <td>应用场景：</td>
    <td>因推送rtp协议时,如果不按照mpegts方式推送的话,<br/>rtp只能传输一路(只能音频或者视频),，<br/>并且rtp在传输数据时，<br/>并不会把sdp,pps等信息一起传送过来的，它会放到sdp中,<br/>我们在推流时,要告知播放器这些信息才能播放;</td>
  </tr>
  <tr>
    <td style="word-wrap:break-word;word-break:break-all;" width="110px";>关联问题：</td>
    <td>#272996</td>
  </tr>
  <tr>
    <td rowspan="5">timeout:<br/>
    <td>使用方式：</td>
    <td>url -H timeout:10;<br/>url -H timeout:100;</td>
  </tr>
  <tr>
    <td>参数作用：</td>
    <td>配置timeout时长,默认：秒;</td>
  </tr>
  <tr>
    <td style="word-wrap:break-word;word-break:break-all;" width="110px";>范     围：</td>
    <td>V2默认设置:常用20s,tls:50s,取值范围:[10, ∞]</td>
  </tr>
  <tr>
    <td>应用场景：</td>
    <td>常用于网络差或者源比较差的情况,<br/>通过参数来控制握手时长,让连接在timeout前可连接上;</td>
  </tr>
  <tr>
    <td style="word-wrap:break-word;word-break:break-all;" width="110px";>关联问题：</td>
    <td>None</td>
  </tr>
  <tr>
    <td rowspan="5">xClientGUID:<br/>
    <td>使用方式：</td>
    <td>url -H xClientGUID:"***"</td>
  </tr>
  <tr>
    <td>参数作用：</td>
    <td>授权id,可理解为在响应服务器时,<br/>需要把这个guid传递回服务器匹配;</td>
  </tr>
  <tr>
    <td style="word-wrap:break-word;word-break:break-all;" width="110px";>范     围：</td>
    <td>V2默认设置:无;</td>
  </tr>
  <tr>
    <td>应用场景：</td>
    <td>某些视频才需要需要这个参数，用于防盗链;</td>
  </tr>
  <tr>
    <td style="word-wrap:break-word;word-break:break-all;" width="110px";>关联问题：</td>
    <td>#75435</td>
  </tr>
  <tr>
    <td rowspan="5">ffnonblock_flag:<br/>
    <td>使用方式：</td>
    <td>url -H ffnonblock_flag:"***"</td>
  </tr>
  <tr>
    <td>参数作用：</td>
    <td>value 1:设置IPTV非阻塞读取数据;<br/>0：设置IPTV阻塞读取读取;</td>
  </tr>
  <tr>
    <td style="word-wrap:break-word;word-break:break-all;" width="110px";>范     围：</td>
    <td>V2默认设置:0;取值:0 or 1</td>
  </tr>
  <tr>
    <td>应用场景：</td>
    <td>默认情况下是阻塞模式;当网络比较差或者网络异常以及源非常差或者seek时<br/>;画面会一直卡住处理可设置为非阻塞模式;</td>
  </tr>
  <tr>
    <td style="word-wrap:break-word;word-break:break-all;" width="110px";>关联问题：</td>
    <td>#280802</td>
  </tr>
  <tr>
    <td rowspan="6">ffprobesize:<br/>
    <td>使用方式：</td>
    <td>url -H ffprobesize:65536</td>
  </tr>
  <tr>
    <td>参数作用：</td>
    <td>设置探测数据的大小</td>
  </tr>
  <tr>
    <td style="word-wrap:break-word;word-break:break-all;" width="110px";>范     围：</td>
    <td>V2默认设置:根据应用场景需求设置探测大小,<br/>正常情况下:1&lt;&lt;20,但在hls模式中default:32kb</td>
  </tr>
  <tr>
    <td>应用场景：</td>
    <td>有些文件容器没必要探测那么多数据,就可以获取到所需要的数据;<br/>上层通过设置这个参数，可加快起播速度;</td>
  </tr>
  <tr>
    <td>与某些项区别：</td>
    <td>ffprobesize: set probing size(读多少数据进行探测);<br/>ffanalyzeduration: specify how many microseconds are analyzedto probe the input(读取数据多长时间内探测)<br/>ffmpeg提供二个同时存在,谁先达到就听谁的,然后停止probe;<br/>具体可参考:ffmpeg -h full | grep 'analyzeduration\|probesize'</td>
  </tr>
  <tr>
    <td style="word-wrap:break-word;word-break:break-all;" width="110px";>关联问题：</td>
    <td>#262802</td>
  </tr>
  <tr>
    <td rowspan="6">ffanalyzeduration:<br/>
    <td>使用方式：</td>
    <td>url -H ffanalyzeduration:"***"</td>
  </tr>
  <tr>
    <td>参数作用：</td>
    <td>1:设置探测的最大时长,单位为:秒;</td>
  </tr>
  <tr>
    <td style="word-wrap:break-word;word-break:break-all;" width="110px";>范     围：</td>
    <td>V2默认设置:0(取系统值5S);取值:[5S, ∞]</td>
  </tr>
  <tr>
    <td>应用场景：</td>
    <td>默认情况下使用系统默认设置的值是完全够用的<br/>;但有些流探测时间需要长一些才能获取到音视频数据;否则可能就探测一路;</td>
  </tr>
  <tr>
    <td>与某些项区别：</td>
    <td>ffprobesize: set probing size(读多少数据进行探测);<br/>ffanalyzeduration: specify how many microseconds are analyzedto probe the input(读取数据多长时间内探测)<br/>ffmpeg提供二个同时存在，可理解为:谁先达到就听谁的,然后停止probe;<br/>具体可参考:ffmpeg -h full | grep 'analyzeduration\|probesize'</td>
  </tr>
  <tr>
    <td style="word-wrap:break-word;word-break:break-all;" width="110px";>关联问题：</td>
    <td>#282803</td>
  </tr>
  <tr>
    <td rowspan="5">av_freerun:<br/>
    <td>使用方式：</td>
    <td>url -H av_freerun:"***"</td>
  </tr>
  <tr>
    <td>参数作用：</td>
    <td>1:上层对某个URL进行设置freerun模式;</td>
  </tr>
  <tr>
    <td style="word-wrap:break-word;word-break:break-all;" width="110px";>范     围：</td>
    <td>V2默认设置:0或者不设置;1:freerun</td>
  </tr>
  <tr>
    <td>应用场景：</td>
    <td>手机推手机拍摄的视频时因码率比较高,播放时会出现声音在播放视频卡顿或者有声音无视频情况;</td>
  </tr>
  <tr>
    <td style="word-wrap:break-word;word-break:break-all;" width="110px";>关联问题：</td>
    <td>#292152</td>
  </tr>
  <tr>
    <td rowspan="5">location:<br/>
    <td>使用方式：</td>
    <td>url -H location:"***"</td>
  </tr>
  <tr>
    <td>参数作用：</td>
    <td>1:不设置,用于V1版本时重定向参数传递;</td>
  </tr>
  <tr>
    <td style="word-wrap:break-word;word-break:break-all;" width="110px";>范     围：</td>
    <td>不设置,V1内部使用</td>
  </tr>
  <tr>
    <td>应用场景：</td>
    <td>V1版本,hls时多次重定向时重定向URL传递;</td>
  </tr>
  <tr>
    <td style="word-wrap:break-word;word-break:break-all;" width="110px";>关联问题：</td>
    <td>#302974</td>
  </tr>
  <tr>
    <td rowspan="5">vn_streams:<br/>
    <td>使用方式：</td>
    <td>url -H vn_streams:"***"</td>
  </tr>
  <tr>
    <td>参数作用：</td>
    <td>第几路的video track关闭(如:关闭这一路中的视频，只保留音频);</td>
  </tr>
  <tr>
    <td style="word-wrap:break-word;word-break:break-all;" width="110px";>范     围：</td>
    <td>不设置,或者 1 或者 2</td>
  </tr>
  <tr>
    <td>应用场景：</td>
    <td>特殊场景下使用(不想使用当前的视频时,想换成其他链接的视频);</td>
  </tr>
  <tr>
    <td style="word-wrap:break-word;word-break:break-all;" width="110px";>关联问题：</td>
    <td>#307740</td>
  </tr>
  <tr>
    <td rowspan="5">an_streams:<br/>
    <td>使用方式：</td>
    <td>url -H an_streams:"***"</td>
  </tr>
  <tr>
    <td>参数作用：</td>
    <td>1:不设置,用于V1版本时重定向参数传递;</td>
  </tr>
  <tr>
    <td style="word-wrap:break-word;word-break:break-all;" width="110px";>范     围：</td>
    <td>不设置,或者 1 或者 2</td>
  </tr>
  <tr>
    <td>应用场景：</td>
    <td>特殊场景下使用(不想使用当前的音频,想换成其他链接的音频);</td>
  </tr>
  <tr>
    <td style="word-wrap:break-word;word-break:break-all;" width="110px";>关联问题：</td>
    <td>#307740</td>
  </tr>
  <tr>
    <td rowspan="5">live_start_index:<br/>
    <td>使用方式：</td>
    <td>url -H live_start_index:"***"</td>
  </tr>
  <tr>
    <td>参数作用：</td>
    <td>1:设置hls直播源播放时,减少延迟,尽量达到实时性;</td>
  </tr>
  <tr>
    <td style="word-wrap:break-word;word-break:break-all;" width="110px";>范     围：</td>
    <td>hls直播源时,defalut: -3(取segment list最后三个切片链接);其他自定义;(需取负值);</td>
  </tr>
  <tr>
    <td>应用场景：</td>
    <td>hls直播场景(因某些hls直播服务器是伪直播,导致播放时会慢很多);</td>
  </tr>
  <tr>
    <td style="word-wrap:break-word;word-break:break-all;" width="110px";>关联问题：</td>
    <td>#314817</td>
  </tr>
  <tr>
    <td rowspan="5">audio_disable:<br/>
    <td>使用方式：</td>
    <td>url -H audio_disable:"***"</td>
  </tr>
  <tr>
    <td>参数作用：</td>
    <td>关掉audio模块;</td>
  </tr>
  <tr>
    <td style="word-wrap:break-word;word-break:break-all;" width="110px";>范     围：</td>
    <td>1: 关掉audio demux + audio解码器(走lavf流程);2: 关掉audio的解码器;other：不设置</td>
  </tr>
  <tr>
    <td>应用场景：</td>
    <td>用于调试+有些客户定制化需求(双路的方式:一路dvb+网络媒体);</td>
  </tr>
  <tr>
    <td style="word-wrap:break-word;word-break:break-all;" width="110px";>关联问题：</td>
    <td>#332565</td>
  </tr>
  <tr>
    <td rowspan="5">video_disable:<br/>
    <td>使用方式：</td>
    <td>url -H video_disable:"***"</td>
  </tr>
  <tr>
    <td>参数作用：</td>
    <td>关掉audio模块;</td>
  </tr>
  <tr>
    <td style="word-wrap:break-word;word-break:break-all;" width="110px";>范     围：</td>
    <td>1: 关掉video demux + video解码器(走lavf流程);2: 关掉video的解码器;other：不设置</td>
  </tr>
  <tr>
    <td>应用场景：</td>
    <td>用于调试+有些客户定制化需求(双路的方式:一路dvb+网络媒体);</td>
  </tr>
  <tr>
    <td style="word-wrap:break-word;word-break:break-all;" width="110px";>关联问题：</td>
    <td>#332565</td>
  </tr>
  <tr>
    <td rowspan="5">subtitle_disable:<br/>
    <td>使用方式：</td>
    <td>url -H subtitle_disable:"***"</td>
  </tr>
  <tr>
    <td>参数作用：</td>
    <td>关掉subtitle模块;</td>
  </tr>
  <tr>
    <td style="word-wrap:break-word;word-break:break-all;" width="110px";>范     围：</td>
    <td>1: 关掉subtitle demux + subtitle解码器(走lavf流程);other：不设置</td>
  </tr>
  <tr>
    <td>应用场景：</td>
    <td>用于调试+有些因内存不够的情况;</td>
  </tr>
  <tr>
    <td style="word-wrap:break-word;word-break:break-all;" width="110px";>关联问题：</td>
    <td>#332565</td>
  </tr>
  <tr>
    <td rowspan="5">xClientGUID:<br/>
    <td>使用方式：</td>
    <td>url -H xClientGUID:"***"</td>
  </tr>
  <tr>
    <td>参数作用：</td>
    <td>网络流媒体协议时某些服务器鉴权字段;</td>
  </tr>
  <tr>
    <td style="word-wrap:break-word;word-break:break-all;" width="110px";>范     围：</td>
    <td>字符串，无范围限制</td>
  </tr>
  <tr>
    <td>应用场景：</td>
    <td>某些服务器需要鉴权;</td>
  </tr>
  <tr>
    <td style="word-wrap:break-word;word-break:break-all;" width="110px";>关联问题：</td>
    <td>#75435</td>
  </tr>
  <tr>
    <td rowspan="5">decryption_key:<br/>
    <td>使用方式：</td>
    <td>url -H decryption_key:"***"</td>
  </tr>
  <tr>
    <td>参数作用：</td>
    <td>对mp4文件的解密;</td>
  </tr>
  <tr>
    <td style="word-wrap:break-word;word-break:break-all;" width="110px";>范     围：</td>
    <td>字符串，无范围限制</td>
  </tr>
  <tr>
    <td>应用场景：</td>
    <td>解密某些mp4文件，用于支持aes-128-cbc,aes-128-cbr等;</td>
  </tr>
  <tr>
    <td style="word-wrap:break-word;word-break:break-all;" width="110px";>关联问题：</td>
    <td>#143479</td>
  </tr>
  <tr>
    <td rowspan="5">offset:"xxx" end_offset:"xxx"<br/>
    <td>使用方式：</td>
    <td>url -H offset:"***" end_offset:"***"</td>
  </tr>
  <tr>
    <td>参数作用：</td>
    <td>设置地址便宜;</td>
  </tr>
  <tr>
    <td style="word-wrap:break-word;word-break:break-all;" width="110px";>范     围：</td>
    <td>无范围限制</td>
  </tr>
  <tr>
    <td>应用场景：</td>
    <td>某些hls的加密源，需要设置便宜地址，因节约内存，把options去掉，需要通过外面进行传递;</td>
  </tr>
  <tr>
    <td style="word-wrap:break-word;word-break:break-all;" width="110px";>关联问题：</td>
    <td>#327888</td>
  </tr>
  <tr>
    <td rowspan="5">http_seekable:"xxx"<br/>
    <td>使用方式：</td>
    <td>url -H http_seekable:"***"</td>
  </tr>
  <tr>
    <td>参数作用：</td>
    <td>0：关掉 Range 的请求 ；1：默认打开 Range 的请求； </td>
  </tr>
  <tr>
    <td style="word-wrap:break-word;word-break:break-all;" width="110px";>范     围：</td>
    <td>参数：0/1</td>
  </tr>
  <tr>
    <td>应用场景：</td>
    <td>某些Dash源时会遇到相关的问题;</td>
  </tr>
  <tr>
    <td style="word-wrap:break-word;word-break:break-all;" width="110px";>关联问题：</td>
    <td>#335451</td>
  </tr>
  <tr>
    <td rowspan="5">vn_streams:"xxx"<br/>
    <td>使用方式：</td>
    <td>url -H vn_streams:"***"</td>
  </tr>
  <tr>
    <td>参数作用：</td>
    <td>1：关掉 video 解码器 + demux 模块；2：关掉 video 解码器模块，但demux模块并不关闭； </td>
  </tr>
  <tr>
    <td style="word-wrap:break-word;word-break:break-all;" width="110px";>范     围：</td>
    <td>参数：1/2</td>
  </tr>
  <tr>
    <td>应用场景：</td>
    <td>某些客户不需要开视频功能;</td>
  </tr>
  <tr>
    <td style="word-wrap:break-word;word-break:break-all;" width="110px";>关联问题：</td>
    <td>#332565</td>
  </tr>
  <tr>
    <td rowspan="5">an_streams:"xxx"<br/>
    <td>使用方式：</td>
    <td>url -H an_streams:"***"</td>
  </tr>
  <tr>
    <td>参数作用：</td>
    <td>1：关掉 audio 解码器 + demux 模块；2：关掉 audio 解码器模块，但demux模块并不关闭； </td>
  </tr>
  <tr>
    <td style="word-wrap:break-word;word-break:break-all;" width="110px";>范     围：</td>
    <td>参数：1/2</td>
  </tr>
  <tr>
    <td>应用场景：</td>
    <td>某些客户不需要开音频功能;</td>
  </tr>
  <tr>
    <td style="word-wrap:break-word;word-break:break-all;" width="110px";>关联问题：</td>
    <td>#332565</td>
  </tr>
  <tr>
    <td rowspan="5">fifo_size:"xxx"<br/>
    <td>使用方式：</td>
    <td>url -H fifo_size:"***"</td>
  </tr>
  <tr>
    <td>参数作用：</td>
    <td style="word-wrap:break-word;word-break:break-all;">
       用于存储接收数据包(fifo缓冲区)
        <br>udp协议 ：fifo缓存区大于0,计算udp协议fifo缓存时，是(fifo_size * 188)才是整个fifo缓冲区的大小;
        <br>rist协议：接收数据缓冲区，该值必须是2的幂次方的值(主要为了优化内存和性能);
    </td>
  </tr>
  <tr>
    <td style="word-wrap:break-word;word-break:break-all;" width="110px";>范     围：</td>
    <td style="word-wrap:break-word;word-break:break-all;">
       不同协议的值范围:
        <br>udp协议 ：fifo_size X 188才是整个fifo缓冲区的大小,默认整个值为(小内存:1MB(5*1024*188),大内存:5MB(28*1024*188));
        <br>rist协议：8192(相对于librist默认值1024);
    </td>
  </tr>
  <tr>
    <td>应用场景：</td>
    <td>IPTV UDP协议或者RIST协议;</td>
  </tr>
  <tr>
    <td style="word-wrap:break-word;word-break:break-all;" width="110px";>关联问题：</td>
    <td>#344688</td>
  </tr>
  <tr>
    <td rowspan="5">rtmp_enhanced_codecs:"xxx"<br/>
    <td>使用方式：</td>
    <td>url -H rtmp_enhanced_codecs:"***"</td>
  </tr>
  <tr>
    <td>参数作用：</td>
    <td>把rtmp增强FLV格式支持HEVC，VP9，AV1 codec </td>
  </tr>
  <tr>
    <td style="word-wrap:break-word;word-break:break-all;" width="110px";>范     围：</td>
    <td>字符串</td>
  </tr>
  <tr>
    <td>应用场景：</td>
    <td>某些网站或者应用特殊定制或者特殊客户特殊定制;</td>
  </tr>
  <tr>
    <td style="word-wrap:break-word;word-break:break-all;" width="110px";>关联问题：</td>
    <td>#367374</td>
  </tr>
  <tr>
    <td rowspan="5">ssl_cipher_list:"xxx"<br/>
    <td>使用方式：</td>
    <td>url -H ssl_cipher_list:"***"</td>
  </tr>
  <tr>
    <td>参数作用：</td>
    <td>设置wolfssl加解密套件，其中ssl_cipher_list中的"XXX"请参考问题 #373375 中的介绍 </td>
  </tr>
  <tr>
    <td style="word-wrap:break-word;word-break:break-all;" width="110px";>范     围：</td>
    <td>字符串</td>
  </tr>
  <tr>
    <td>应用场景：</td>
    <td>https的网络应用，因有些客户反馈https卡顿，通过调整加密套件可以减少卡顿的情况;</td>
  </tr>
  <tr>
    <td style="word-wrap:break-word;word-break:break-all;" width="110px";>关联问题：</td>
    <td>#373375</td>
  </tr>
  <tr>
    <td rowspan="5">Authorization:"xxx"<br/>
    <td>使用方式：</td>
    <td>url -H Authorization:"***"</td>
  </tr>
  <tr>
    <td>参数作用：</td>
    <td>设置http请求时允许客户端向服务器传达认证信息,目前ff开源代码中并没有该参数(某些网站或者应用特殊定制) </td>
  </tr>
  <tr>
    <td style="word-wrap:break-word;word-break:break-all;" width="110px";>范     围：</td>
    <td>字符串</td>
  </tr>
  <tr>
    <td>应用场景：</td>
    <td>某些网站或者应用特殊定制;</td>
  </tr>
  <tr>
    <td style="word-wrap:break-word;word-break:break-all;" width="110px";>关联问题：</td>
    <td>#377006</td>
  </tr>
  <tr>
    <td rowspan="5">CustomHeaders:"xxx"<br/>
    <td>使用方式：</td>
    <td>url -H CustomHeaders:"***"</td>
  </tr>
  <tr>
    <td>参数作用：</td>
    <td>设置http请求时自定义数据头 </td>
  </tr>
  <tr>
    <td style="word-wrap:break-word;word-break:break-all;" width="110px";>范     围：</td>
    <td>字符串</td>
  </tr>
  <tr>
    <td>应用场景：</td>
    <td>某些网站或者应用特殊定制;</td>
  </tr>
  <tr>
    <td style="word-wrap:break-word;word-break:break-all;" width="110px";>关联问题：</td>
    <td>#377006</td>
  </tr>
  <tr>
    <td rowspan="5">socks5_proxy:"xxx"<br/>
    <td>使用方式：</td>
    <td>url -H socks5_proxy:"***"</td>
  </tr>
  <tr>
    <td>参数作用：</td>
    <td>设置socks5代理 </td>
  </tr>
  <tr>
    <td style="word-wrap:break-word;word-break:break-all;" width="110px";>范     围：</td>
    <td>字符串</td>
  </tr>
  <tr>
    <td>应用场景：</td>
    <td>某些地区下使用某些网站时需应用特殊定制;</td>
  </tr>
  <tr>
    <td style="word-wrap:break-word;word-break:break-all;" width="110px";>关联问题：</td>
    <td>#388865</td>
  </tr>
  <tr>
    <td rowspan="5">http_proxy:"xxx"<br/>
    <td>使用方式：</td>
    <td>url -H http_proxy:"***"</td>
  </tr>
  <tr>
    <td>参数作用：</td>
    <td>设置http代理 </td>
  </tr>
  <tr>
    <td style="word-wrap:break-word;word-break:break-all;" width="110px";>范     围：</td>
    <td>字符串</td>
  </tr>
  <tr>
    <td>应用场景：</td>
    <td>某些地区下使用某些网站时需应用特殊定制;</td>
  </tr>
  <tr>
    <td style="word-wrap:break-word;word-break:break-all;" width="110px";>关联问题：</td>
    <td>#392501</td>
  </tr>
  <tr>
    <td rowspan="5">stc_recover_mode:"xxx"<br/>
    <td>使用方式：</td>
    <td>url -H stc_recover_mode:"***"</td>
  </tr>
  <tr>
    <td>参数作用：</td>
    <td>设置同步方式 </td>
  </tr>
  <tr>
    <td style="word-wrap:break-word;word-break:break-all;" width="110px";>范     围：</td>
    <td>数值正数</td>
  </tr>
  <tr>
    <td>应用场景：</td>
    <td>某些应用或者应用场景下;</td>
  </tr>
  <tr>
    <td style="word-wrap:break-word;word-break:break-all;" width="110px";>关联问题：</td>
    <td>#384168</td>
  </tr>
  <tr>
    <td rowspan="5">rist_secret:"xxx"<br/>
    <td>使用方式：</td>
    <td>url -H rist_secret:"***"</td>
  </tr>
  <tr>
    <td>参数作用：</td>
    <td>启动rist协议加密功能，设置密钥,支持AES加密(AES-128/AES-256)</td>
  </tr>
  <tr>
    <td style="word-wrap:break-word;word-break:break-all;" width="110px";>范     围：</td>
    <td>字符串(最大支持128字节,16个字符)</td>
  </tr>
  <tr>
    <td>应用场景：</td>
    <td>rist协议上使用;</td>
  </tr>
  <tr>
    <td style="word-wrap:break-word;word-break:break-all;" width="110px";>关联问题：</td>
    <td>#326279</td>
  </tr>
  <tr>
    <td rowspan="5">rist_encryption_type:"xxx"<br/>
    <td>使用方式：</td>
    <td>url -H rist_encryption_type:"***"</td>
  </tr>
  <tr>
    <td>参数作用：</td>
    <td>启动rist协议加密密钥的长度 </td>
  </tr>
  <tr>
    <td style="word-wrap:break-word;word-break:break-all;" width="110px";>范     围：</td>
    <td>数值正数,目前支持二种模式，0(默认)/128(AES-128)/256(AES-256)</td>
  </tr>
  <tr>
    <td>应用场景：</td>
    <td>rist协议上使用;</td>
  </tr>
  <tr>
    <td style="word-wrap:break-word;word-break:break-all;" width="110px";>关联问题：</td>
    <td>#326279</td>
  </tr>
  <tr>
    <td rowspan="5">rist_profile:"xxx"<br/>
    <td>使用方式：</td>
    <td>url -H rist_profile:"***"</td>
  </tr>
  <tr>
    <td>参数作用：</td>
    <td>设置rist协议支持的配置文件 </td>
  </tr>
  <tr>
    <td style="word-wrap:break-word;word-break:break-all; width:110px;">范     围：</td>
    <td style="word-wrap:break-word;word-break:break-all;">
        数值正数,默认设置为1(main profile),支持如下几种模式:
        <br>0: simple profile;只支持基本的rist功能，不支持高级功能,使用于简单的点对点传输场景;
        <br>1: main profile;支持rist核心功能,包括NACK和简单的拥塞控制,使用于实时流媒体传输场景;
        <br>2: advanced profile;支持rist的所有高级功能，包裹加密、多路传输、动态码率调整等;
        <br>other:播放器强制为1，即main profile 模式;
    </td>
  </tr>
  <tr>
    <td>应用场景：</td>
    <td>rist协议上使用;</td>
  </tr>
  <tr>
    <td style="word-wrap:break-word;word-break:break-all;" width="110px";>关联问题：</td>
    <td>#326279</td>
  </tr>
  </table>
