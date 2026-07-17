# /bin/bash
# ******************************************************************************
#                                  CONFIDENTIAL
#               Hangzhou NationalChip Science and Technology Co.,Ltd.
#                        (C)2006-2008, All right reserved
# ******************************************************************************
# File Name :    config.sh
# Author    :    liuyx
# Project   :    lowpower
# Type      :    csky
# ******************************************************************************
# Purpose   :    Initial the configuration of the lowpower
# ******************************************************************************

LinkFile=./link.ld
MakeFile=./makefile

# ******************************************************************************
# Function      : modify_link
# Description   : Modify or create link.ld 
# Arguments     : [in]  $chip_type
# Returns       : void
# Other         : void
# ******************************************************************************
creat_link()
{
	case $1 in
		gx3211|gx6605s)
			echo "ENTRY( __reset );"                                                     >$LinkFile
			echo "MEMORY"                                                               >>$LinkFile
			echo "{"                                                                    >>$LinkFile
			echo "    sram : ORIGIN = 0x00100000+1024, LENGTH = (0x4000-256-1024-256)"  >>$LinkFile
			echo "}"                                                                    >>$LinkFile
			echo "__stack = 0x00100000+(0x4000-256);"                                   >>$LinkFile
			;;
		gx3201|gx3113c)
			echo "ENTRY( __reset );"                                                     >$LinkFile
			echo "MEMORY"                                                               >>$LinkFile
			echo "{"                                                                    >>$LinkFile
			echo "    sram : ORIGIN = 0x00100000+1024, LENGTH = (0x3000-256-1024-256)"  >>$LinkFile
			echo "}"                                                                    >>$LinkFile
			echo "__stack = 0x00100000+(0x3000-256);"                                   >>$LinkFile
			;;
	esac

	echo "SECTIONS"                                                                     >>$LinkFile
	echo "{"                                                                            >>$LinkFile
	echo "    .text : {"                                                                >>$LinkFile
	echo "        *(.text*)"                                                            >>$LinkFile
	echo "        *(.rodata*)"                                                          >>$LinkFile
	echo "        . = ALIGN(4);"                                                        >>$LinkFile
	echo "        _end_text = .;"                                                       >>$LinkFile
	echo "        _start_data = .;"                                                     >>$LinkFile
	echo "        *(.data*)"                                                            >>$LinkFile
	echo "        . = ALIGN(4);"                                                        >>$LinkFile
	echo "        _end_data = .;"                                                       >>$LinkFile
	echo "    } > sram"                                                                 >>$LinkFile
	echo ""                                                                             >>$LinkFile
	echo "    .bss : {"                                                                 >>$LinkFile
	echo "        _sbss = .;"                                                           >>$LinkFile
	echo "        *(.bss) *(COMMON)"                                                    >>$LinkFile
	echo "        . = ALIGN(4);"                                                        >>$LinkFile
	echo "        PROVIDE(_ebss = .);"                                                  >>$LinkFile
	echo "    } > sram"                                                                 >>$LinkFile
	echo "}"                                                                            >>$LinkFile
}

# ******************************************************************************
# Function      : modify_make
# Description   : Modify makefile 
# Arguments     : [in]  $chip_type
# Returns       : void
# Other         : void
# ******************************************************************************
modify_make()
{
	case $1 in
		gx3201|gx3211|gx6605s)
			sed -i -e 's/^TextBaseAddr =.*/TextBaseAddr = 0x00000000/g'   \
				$MakeFile
			;;
		gx3113c)
			sed -i -e 's/^TextBaseAddr =.*/TextBaseAddr = 0x00000000/g'   \
				$MakeFile
			;;
	esac
}

# ******************************************************************************
# Function      : creat_config
# Description   : Create config
# Arguments     : void
# Returns       : void
# Other         : void
# ******************************************************************************
creat_config()
{
	cp include/${1}.h include/config.h
}


# ******************************************************************************
# Function      : config_all
# Description   : Call some functions such as creat_config,creat_link
#                 and modify_make to config all.
# Arguments     : void
# Returns       : void
# Other         : void
# ******************************************************************************
config_all()
{
	creat_config $@
	creat_link $@
	modify_make $@ 
}

config_all $@

exit 0
