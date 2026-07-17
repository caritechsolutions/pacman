## URL 示例

### 网络 URL

- 示例：
  -  “http://192.168.0.100/sample.mkv”
  - "http://192.168.189.240:10080/vhls/EDK6_DuGg/EDK6_DuGg_live.m3u8"
  - "http://192.168.189.240:10080/flv/vlive/EDK6_DuGg.flv"
  - "rtmp://192.168.189.240:10035/vlive/EDK6_DuGg"
  - "https://192.168.189.240:443/vhls/EDK6_DuGg/EDK6_DuGg_live.m3u8"
  - "https://192.168.189.240:443/flv/vlive/EDK6_DuGg.flv"
  - "rtsp://192.168.189.240:10054/EDK6_DuGg"
  - “http://192.168.0.100/sample.mkv -H ffprobesize:65536”
- 其他说明
  - 网络 URL 根据客户服务器需求，可以按需追加一些特殊配置，详见：[IPTV option](../function/function.md#iptv-options)

### 本地 URL

- 示例：

  - /mnt/usb/sample.mkv
  - /mnt/usb0_1/xxx.ts.dvr

- 获取方法：

  1、Hotplug 相关接口获取到分区信息，例如:

    ```
    GxHotplugPartitionGet(HOTPLUG_TYPE_USB);
    ```

  2、GxCoreFS 相关接口获取到文件路径信息，例如：

    ```
    GxCore_GetDir(path, &(explorer->ents), suffix);
    ```

### DVB URL

- 示例：
   - dvbc://fre:770000&symbol:27500&qam:8&vpid:515&apid:653&pcrpid:515&vcodec:0&acodec:0&tuner:0&pmt:273
   - dvbs://fre:770000&symbol:27500&polar:0&22k:1&vpid:515&apid:653&pcrpid:515&vcodec:0&acodec:0&tuner:0&pmt:273
   - dvbt://fre:770000&bandwidth:8000&vpid:515&apid:653&pcrpid:515&vcodec:0&acodec:0&tuner:0&pmt:273

- 获取方法：

  1、PM 相关接口获取到基础的URL，例如:

    ```
    GxBus_PmProgUrlGet(&prog[i], prog_url, PLAYER_URL_LONG);
    ```

  2、URL 相关接口追加或修改特定配置，例如：

    ```
     GxUrl_SetItem((char *)prog_url, GX_URL_KEY_TSID, 0, PLAYER_URL_LONG);
    ```

- 其他说明

  - GxUrl_SetItem 系列接口支持的 “key” 参见头文件：gxplayer_url.h

### FM URL

- 示例：
   - fm://fre:109000

- 获取方法：

  1、PM 相关接口获取到基础的URL，例如:

    ```
    GxBus_PmProgUrlGet(&prog[i], prog_url, PLAYER_URL_LONG);
    ```

  2、URL 相关接口追加或修改特定配置，例如：

    ```
     GxUrl_SetItem((char *)prog_url, GX_URL_KEY_FRE, 108000, PLAYER_URL_LONG);
    ```

- 其他说明

  - GxUrl_SetItem 系列接口支持的 “key” 参见头文件：gxplayer_url.h
