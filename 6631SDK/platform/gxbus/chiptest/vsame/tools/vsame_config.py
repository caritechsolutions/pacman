

# streamjson: 测试流对应的json文件,当路径为相对路径时，相对于命令执行位置的路径
# avtype: 指定测试流是音频还是视频，默认是视频,值为audio或者video
# streamtype: 测试流数据格式，是ES还是TS，默认是ES
client_config = {
        '1':{'streamjson':'./all.json', 'avtype':'video', 'streamtype':'ES'},
        '2':{'streamjson':'./264.json', 'avtype':'video', 'streamtype':'ES'},
        }
