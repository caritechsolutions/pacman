1. 在boot.c里调用key_register(key_type)注册所需的按键，可选的按键数组定义见key_api.h
	eg: key_register(key_gongban_nationalchip_new)
2. 编译时自动生成key_api.h以及key目录，其中key目录存放按键的物理键值
	cd /gx/theme/key && sh key_merge_py.sh && cd -
3. xml里调用描述按键的先后顺序有要求
	eg:
		GUIK_0
		GUIK_1
		GUIk_2
		.......
		GUIk_9
		GUIK_RETURN
		GUIK_UP
		GUIK_DOWN
		GUIK_LEFT
		GUIK_RIGHT
		..........
4. 添加新的按键只需要将相应的xml描述放到key_xml/目录下，然后注册对应的按键数组即可
