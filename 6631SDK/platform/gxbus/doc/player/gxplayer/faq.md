
# faq.md

## 常见问题

### 问题1: **IPTV 的 V1、V2 是什么?**

  <table width="858" border="0" cellpadding="0" cellspacing="0" style='width:641.00pt;border-collapse:collapse;table-layout:fixed;'>
   <col width="55" style='mso-width-source:userset;mso-width-alt:2012;'/>
   <col width="58" span="4" style='width:54.00pt;'/>
   <col width="58" style='mso-width-source:userset;mso-width-alt:2172;'/>
   <col width="58" span="2" style='width:54.00pt;'/>
   <col width="58" style='mso-width-source:userset;mso-width-alt:2364;'/>
   <col width="68" span="4" style='width:54.00pt;'/>
   <col width="48" style='mso-width-source:userset;mso-width-alt:1628;'/>
   <tr height="19" style='height:14.25pt;'>
    <td class="xl65" height="19" width="50" style='height:14.25pt;width:42.50pt;' x:str>版本</td>
    <td class="xl66" width="249" colspan="5" style='width:119.25pt;border-right:.5pt solid windowtext;border-bottom:.5pt solid windowtext;' x:str>区别</td>
    <td class="xl66" width="211" colspan="3" style='width:135.75pt;border-right:.5pt solid windowtext;border-bottom:.5pt solid windowtext;' x:str>Release 版本中是V1V2?</td>
    <td class="xl66" width="322" colspan="5" style='width:236.50pt;border-right:.5pt solid windowtext;border-bottom:.5pt solid windowtext;' x:str>优缺点</td>
   </tr>
   <tr height="81.33" style='height:61.00pt;mso-height-source:userset;mso-height-alt:1220;'>
    <td class="xl68" height="81.33" style='height:61.00pt;' x:str>V1</td>
    <td class="xl69" colspan="5" style='border-right:.5pt solid windowtext;border-bottom:.5pt solid windowtext;' x:str>旧版本，协议实现在:(access/stream_***.c中,<br/>demux可能在demux_***.c or ffmpeg中实现)</td>
    <td class="xl70" colspan="3" style='border-right:.5pt solid windowtext;border-bottom:.5pt solid windowtext;' x:str>&lt;= V1.9.8-7</td>
    <td class="xl69" colspan="5" style='border-right:.5pt solid windowtext;border-bottom:.5pt solid windowtext;' x:str>1.功能比较稳定;经过终端客户校验;<br/>2.不支持mpegdash、local dash、<br/>compose、concat、rtp、rtpsdp 单路播放流功能;<br/>3.扩展性较差,<br>新加功能需移植第三方库及改动较大,代码重复;<br/>4.V1目前已经舍弃,已切换到V2版本当作主版本;</td>
   </tr>
   <tr height="80" style='height:60.00pt;mso-height-source:userset;mso-height-alt:1200;'>
    <td class="xl68" height="80" style='height:60.00pt;' x:str>V2</td>
    <td class="xl71" colspan="5" style='border-right:.5pt solid windowtext;border-bottom:.5pt solid windowtext;' x:str>新版本，协议跟demux的实现均在:<br/>avformat/****(ffmpeg code)中实现,<br/>access/***除传入url判断限制外,其他不做任何处理;</td>
    <td class="xl70" colspan="3" style='border-right:.5pt solid windowtext;border-bottom:.5pt solid windowtext;' x:str>&gt;= V1.9.8-8</td>
    <td class="xl69" colspan="5" style='border-right:.5pt solid windowtext;border-bottom:.5pt solid windowtext;' x:str>1.功能比较多;扩展性好,改动不会太大;<br/>2.与ffmpeg 4.0.0版本对齐的(协议部分);<br/>3.支持的功能比较完善,<br/>后期新功能直接从ffmpeg最新版本上移植;</td>
  </table>


