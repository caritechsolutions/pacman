#!/bin/bash

if [ -e ${GXSRC_PATH}/.config ]; then
    source ${GXSRC_PATH}/.config
else
    echo -e "\033[031mCannot find .config file, config first\033[0m"
    echo -e "\033[032mRun make menuconfig please\033[0m"
    exit 1
fi

function build_gxloader()
{
    CHIP=${BR2_CHIP_NAME//\"/}
    BOARD_NAME=${BR2_BOARD_NAME//\"/}
    GXLOADER_PATH=${GXSRC_PATH}/../platform/gxloader

    if [[ ${BR2_MOD_CHIP_NNE} = y ]]; then
        CHIP_BOARD=${BOARD_NAME}-NNE
    elif [[ ${BR2_MOD_CHIP_NNM} = y ]]; then
        CHIP_BOARD=${BOARD_NAME}-NNM
    else
        CHIP_BOARD=${BOARD_NAME}
    fi

    if [[ ${BR2_MOD_FUNC_CI} = y && -d ${GXLOADER_PATH}/conf/${CHIP}/${CHIP_BOARD}_dvbci ]]; then
        CHIP_BOARD=${CHIP_BOARD}_dvbci
    fi

    if [[ ! -e ${GXLOADER_PATH}/conf/${CHIP}/${CHIP_BOARD}/release.config ]]; then
        echo -e "\033[031mCannot find gxloader corresponding ${CHIP} ${CHIP_BOARD} file\033[0m"
        exit 1
    fi

    cd ${GXLOADER_PATH}

    cp conf/${CHIP}/${CHIP_BOARD}/release.config .config

    if [[ -n `grep -w "ENABLE_ROOTFS_CRAMFS" .config` ]]; then
        sed -i 's/^ENABLE_ROOTFS_CRAMFS[[:space:]]*=[[:space:]]*y/ENABLE_ROOTFS_CRAMFS = n/' .config
    else
        sed -i '$a\ENABLE_ROOTFS_CRAMFS = n' .config
    fi

    if [[ -n `grep -w "ENABLE_DECOMPRESS_GZIP" .config` ]]; then
        sed -i 's/^ENABLE_DECOMPRESS_GZIP[[:space:]]*=[[:space:]]*y/ENABLE_DECOMPRESS_GZIP = n/' .config
    else
        sed -i '$a\ENABLE_DECOMPRESS_GZIP = n' .config
    fi

    if [[ -n `grep -w "ENABLE_DECOMPRESS_ZLIB" .config` ]]; then
        sed -i 's/^ENABLE_DECOMPRESS_ZLIB[[:space:]]*=[[:space:]]*y/ENABLE_DECOMPRESS_ZLIB = n/' .config
    else
        sed -i '$a\ENABLE_DECOMPRESS_ZLIB = n' .config
    fi

    if [[ -n `grep -w "ENABLE_DECOMPRESS_LZMA" .config` ]]; then
        sed -i 's/^ENABLE_DECOMPRESS_LZMA[[:space:]]*=[[:space:]]*n/ENABLE_DECOMPRESS_LZMA = y/' .config
    else
        sed -i '$a\ENABLE_DECOMPRESS_LZMA = y' .config
    fi

    if [[ ${BR2_OS_NAME//\"/} = "ecos" ]]; then
        if [[ -n `grep -w "ENABLE_KERNEL_DTB" .config` ]]; then
            sed -i 's/^ENABLE_KERNEL_DTB[[:space:]]*=[[:space:]]*y/ENABLE_KERNEL_DTB = n/' .config
        else
            sed -i '$a\ENABLE_KERNEL_DTB = n' .config
        fi

        if [[ -n `grep -w "ENABLE_UIMAGE" .config` ]]; then
            sed -i 's/^ENABLE_UIMAGE[[:space:]]*=[[:space:]]*y/ENABLE_UIMAGE = n/' .config
        else
            sed -i '$a\ENABLE_UIMAGE = n' .config
        fi

        if [[ -n `grep -w "ENABLE_ZIMAGE" .config` ]]; then
            sed -i 's/^ENABLE_ZIMAGE[[:space:]]*=[[:space:]]*y/ENABLE_ZIMAGE = n/' .config
        else
            sed -i '$a\ENABLE_ZIMAGE = n' .config
        fi

        if [[ -n `grep -w "ENABLE_ROOTFS_ROMFS" .config` ]]; then
            sed -i 's/^ENABLE_ROOTFS_ROMFS[[:space:]]*=[[:space:]]*n/ENABLE_ROOTFS_ROMFS = y/' .config
        else
            sed -i '$a\ENABLE_ROOTFS_ROMFS = y' .config
        fi
    else
        if [[ -n `grep -w "ENABLE_KERNEL_DTB" .config` ]]; then
            sed -i 's/^ENABLE_KERNEL_DTB[[:space:]]*=[[:space:]]*n/ENABLE_KERNEL_DTB = y/' .config
        else
            sed -i '$a\ENABLE_KERNEL_DTB = y' .config
        fi

        if [[ -n `grep -w "ENABLE_UIMAGE" .config` ]]; then
            sed -i 's/^ENABLE_UIMAGE[[:space:]]*=[[:space:]]*n/ENABLE_UIMAGE = y/' .config
        else
            sed -i '$a\ENABLE_UIMAGE = y' .config
        fi

        if [[ ${BR2_MOD_APP_NET_UPGRADE} = y ]]; then
            if [[ -n `grep -w "ENABLE_ROOTFS_ROMFS" .config` ]]; then
                sed -i 's/^ENABLE_ROOTFS_ROMFS[[:space:]]*=[[:space:]]*n/ENABLE_ROOTFS_ROMFS = y/' .config
            else
                sed -i '$a\ENABLE_ROOTFS_ROMFS = y' .config
            fi
        else
            if [[ -n `grep -w "ENABLE_ROOTFS_ROMFS" .config` ]]; then
                sed -i 's/^ENABLE_ROOTFS_ROMFS[[:space:]]*=[[:space:]]*y/ENABLE_ROOTFS_ROMFS = n/' .config
            else
                sed -i '$a\ENABLE_ROOTFS_ROMFS = n' .config
            fi
        fi

        if [[ "${CHIP}" == "gx6605s" ]]; then
            if [[ -n `grep -w "^CMDLINE_VALUE" .config` ]]; then
                sed -i '/^CMDLINE_VALUE/d' .config
            fi
            sed -i '$a\CMDLINE_VALUE = "mem=34M videomem=30M mem_end console=ttyS0,115200 init=/init"' .config
        elif [[ "${CHIP}" == "gemini" ]]; then
            if [[ -n `grep -w "^CMDLINE_VALUE" .config` ]]; then
                sed -i '/^CMDLINE_VALUE/d' .config
            fi
            sed -i '$a\CMDLINE_VALUE = "mem=67224K videomem=60M femem=2408K mem_end console=ttyS0,115200 init=/init"' .config
        fi
    fi

    if [[ ${BR2_MOD_APP_NET_UPGRADE} = y ]]; then
        if [[ -n `grep -w "CONFIG_LOAD_GX_RECOVERY_OS" .config` ]]; then
            sed -i 's/^CONFIG_LOAD_GX_RECOVERY_OS[[:space:]]*=[[:space:]]*n/CONFIG_LOAD_GX_RECOVERY_OS = y/' .config
        else
            sed -i '$a\CONFIG_LOAD_GX_RECOVERY_OS = y' .config
        fi
    else
        if [[ -n `grep -w "CONFIG_LOAD_GX_RECOVERY_OS" .config` ]]; then
            sed -i 's/^CONFIG_LOAD_GX_RECOVERY_OS[[:space:]]*=[[:space:]]*y/CONFIG_LOAD_GX_RECOVERY_OS = n/' .config
        else
            sed -i '$a\CONFIG_LOAD_GX_RECOVERY_OS = n' .config
        fi
    fi

    if [[ ${BR2_MOD_FUNC_OTA_LOADER} = y  ]]; then
        if [[ -n `grep -w "ENABLE_LIB" .config` ]]; then
            sed -i 's/^ENABLE_LIB[[:space:]]*=[[:space:]]*y/ENABLE_LIB = n/' .config
        else
            sed -i '$a\ENABLE_LIB = n' .config
        fi

        if [[ -n `grep -w "ENABLE_ROOTFS_ROMFS" .config` ]]; then
            sed -i 's/^ENABLE_ROOTFS_ROMFS[[:space:]]*=[[:space:]]*n/ENABLE_ROOTFS_ROMFS = y/' .config
        else
            sed -i '$a\ENABLE_ROOTFS_ROMFS = y' .config
        fi

        if [[ -n `grep -w "ENABLE_OTA" .config` ]]; then
            sed -i 's/^ENABLE_OTA[[:space:]]*=[[:space:]]*n/ENABLE_OTA = y/' .config
        else
            sed -i '$a\ENABLE_OTA = y' .config
        fi

        if [[ -n `grep -w "ENABLE_OTA_FORCE_DRAM_ADDR" .config` ]]; then
            sed -i 's/^ENABLE_OTA_FORCE_DRAM_ADDR[[:space:]]*=[[:space:]]*n/ENABLE_OTA_FORCE_DRAM_ADDR = y/' .config
        else
            sed -i '$a\ENABLE_OTA_FORCE_DRAM_ADDR = y' .config
        fi

        flash_config_ota=${GXSRC_PATH}/output/image/bin_${OS}/flash.conf
        ota_patition_start_addr=`grep "ota.img" ${flash_config_ota} | awk '{print $8}'`
        count=$(sed -n  '/ota.img/=' $flash_config_ota)
        let count++
        line=$(sed -n "${count}p" $flash_config_ota)
        ota_patition_end_addr=`echo $line | awk '{print $8}'`
        sed -i "s|.*OTA_FORCE_FLASH_ADDR.*|OTA_FORCE_FLASH_ADDR = ${ota_patition_start_addr}|" script/ota.config
        ota_flash_size=`printf "0x%06x\n" $(($ota_patition_end_addr-$ota_patition_start_addr))`
        sed -i "s|.*OTA_FORCE_FLASH_SIZE.*|OTA_FORCE_FLASH_SIZE = ${ota_flash_size}|" script/ota.config
    else
        if [[ -n `grep -w "ENABLE_LIB" .config` ]]; then
            sed -i 's/^ENABLE_LIB[[:space:]]*=[[:space:]]*y/ENABLE_LIB = n/' .config
        else
            sed -i '$a\ENABLE_LIB = n' .config
        fi

        if [[ -n `grep -w "ENABLE_OTA" .config` ]]; then
            sed -i 's/^ENABLE_OTA[[:space:]]*=[[:space:]]*y/ENABLE_OTA = n/' .config
        else
            sed -i '$a\ENABLE_OTA = n' .config
        fi

        if [[ -n `grep -w "ENABLE_OTA_FORCE_DRAM_ADDR" .config` ]]; then
            sed -i 's/^ENABLE_OTA_FORCE_DRAM_ADDR[[:space:]]*=[[:space:]]*y/ENABLE_OTA_FORCE_DRAM_ADDR = n/' .config
        else
            sed -i '$a\ENABLE_OTA_FORCE_DRAM_ADDR = n' .config
        fi
    fi

    if [[ ${BR2_MOD_USB_RECOVER} = y ]]; then
        FLASH_SIZE=`grep -w "^flash_size" ${GXSRC_PATH}/output/image/bin_${OS}/flash.conf | awk '{print $2}'`
        if [[ -n `grep -w "ENABLE_USB_RECOVER" .config` ]]; then
            sed -i 's/^ENABLE_USB_RECOVER[[:space:]]*=[[:space:]]*n/ENABLE_USB_RECOVER = y/' .config
        else
            sed -i '$a\ENABLE_USB_RECOVER = y' .config
        fi

        if [[ -n `grep -w "ENABLE_USB" .config` ]]; then
            sed -i 's/^ENABLE_USB[[:space:]]*=[[:space:]]*n/ENABLE_USB = y/' .config
        else
            sed -i '$a\ENABLE_USB = y' .config
        fi

        if [[ -n `grep -w "ENABLE_IRR" .config` ]]; then
            sed -i 's/^ENABLE_IRR[[:space:]]*=[[:space:]]*n/ENABLE_IRR = y/' .config
        else
            sed -i '$a\ENABLE_IRR = y' .config
        fi

        if [[ -n `grep -w "ENABLE_FLASH_FULLFUNCTION" .config` ]]; then
            sed -i 's/^ENABLE_FLASH_FULLFUNCTION[[:space:]]*=[[:space:]]*n/ENABLE_FLASH_FULLFUNCTION = y/' .config
        else
            sed -i '$a\ENABLE_FLASH_FULLFUNCTION = y' .config
        fi

        if [[ -n `grep -w "ENABLE_GDI" .config` ]]; then
            sed -i 's/^ENABLE_GDI[[:space:]]*=[[:space:]]*n/ENABLE_GDI = y/' .config
        else
            sed -i '$a\ENABLE_GDI = y' .config
        fi

        if [[ -n `grep -w "CONFIG_RECOVERY_IMAGE_SIZE" .config` ]]; then
            sed -i "s/^CONFIG_RECOVERY_IMAGE_SIZE[[:space:]]*=[[:space:]]*$/CONFIG_RECOVERY_IMAGE_SIZE = ${FLASH_SIZE}/" .config
        else
            sed -i "\$a\CONFIG_RECOVERY_IMAGE_SIZE = ${FLASH_SIZE}" .config
        fi

        if [[ -n `grep -w "ENABLE_WDT" .config` ]]; then
            sed -i 's/^ENABLE_WDT[[:space:]]*=[[:space:]]*n/ENABLE_WDT = y/' .config
        else
            sed -i '$a\ENABLE_WDT = y' .config
        fi
    else
        if [[ -n `grep -w "ENABLE_USB_RECOVER" .config` ]]; then
            sed -i 's/^ENABLE_USB_RECOVER[[:space:]]*=[[:space:]]*y/ENABLE_USB_RECOVER = n/' .config
        else
            sed -i '$a\ENABLE_USB_RECOVER = n' .config
        fi

        if [[ -n `grep -w "ENABLE_USB" .config` ]]; then
            sed -i 's/^ENABLE_USB[[:space:]]*=[[:space:]]*y/ENABLE_USB = n/' .config
        else
            sed -i '$a\ENABLE_USB = n' .config
        fi

        if [[ -n `grep -w "ENABLE_IRR" .config` ]]; then
            sed -i 's/^ENABLE_IRR[[:space:]]*=[[:space:]]*y/ENABLE_IRR = n/' .config
        else
            sed -i '$a\ENABLE_IRR = n' .config
        fi

        if [[ -n `grep -w "ENABLE_FLASH_FULLFUNCTION" .config` ]]; then
            sed -i 's/^ENABLE_FLASH_FULLFUNCTION[[:space:]]*=[[:space:]]*y/ENABLE_FLASH_FULLFUNCTION = n/' .config
        else
            sed -i '$a\ENABLE_FLASH_FULLFUNCTION = n' .config
        fi

        if [[ -n `grep -w "ENABLE_GDI" .config` ]]; then
            sed -i 's/^ENABLE_GDI[[:space:]]*=[[:space:]]*y/ENABLE_GDI = n/' .config
        else
            sed -i '$a\ENABLE_GDI = n' .config
        fi
    fi

    if [[ ${BR2_MOD_NAND_FLASH} = y ]]; then
        if [[ -n `grep -w "ENABLE_SPIFLASH" .config` ]]; then
            sed -i 's/^ENABLE_SPIFLASH[[:space:]]*=[[:space:]]*y/ENABLE_SPIFLASH = n/' .config
        else
            sed -i '$a\ENABLE_SPIFLASH = n' .config
        fi
        if [[ ${BR2_MOD_SPI_NAND} = y ]]; then
            if [[ -n `grep -w "ENABLE_SPINAND" .config` ]]; then
                sed -i 's/^ENABLE_SPINAND[[:space:]]*=[[:space:]]*n/ENABLE_SPINAND = y/' .config
            else
                sed -i '$a\ENABLE_SPINAND = y' .config
            fi
            if [[ -n `grep -w "ENABLE_NANDFLASH" .config` ]]; then
                sed -i 's/^ENABLE_NANDFLASH[[:space:]]*=[[:space:]]*y/ENABLE_NANDFLASH = n/' .config
            else
                sed -i '$a\ENABLE_NANDFLASH = n' .config
            fi
        elif [[ ${BR2_MOD_P_NAND} = y ]]; then
            if [[ -n `grep -w "ENABLE_NANDFLASH" .config` ]]; then
                sed -i 's/^ENABLE_NANDFLASH[[:space:]]*=[[:space:]]*n/ENABLE_NANDFLASH = y/' .config
            else
                sed -i '$a\ENABLE_NANDFLASH = y' .config
            fi
            if [[ -n `grep -w "ENABLE_SPINAND" .config` ]]; then
                sed -i 's/^ENABLE_SPINAND[[:space:]]*=[[:space:]]*y/ENABLE_SPINAND = n/' .config
            else
                sed -i '$a\ENABLE_SPINAND = n' .config
            fi
        fi
        if [[ ${BR2_MOD_NAND_UBIFS} = y ]]; then
            if [[ -n `grep -w "ENABLE_ROOTFS_UBIFS" .config` ]]; then
                sed -i 's/^ENABLE_ROOTFS_UBIFS[[:space:]]*=[[:space:]]*n/ENABLE_ROOTFS_UBIFS = y/' .config
            else
                sed -i '$a\ENABLE_ROOTFS_UBIFS= y' .config
            fi
        fi
    else
        if [[ -n `grep -w "ENABLE_SPIFLASH" .config` ]]; then
            sed -i 's/^ENABLE_SPIFLASH[[:space:]]*=[[:space:]]*n/ENABLE_SPIFLASH = y/' .config
        else
            sed -i '$a\ENABLE_SPIFLASH = y' .config
        fi
        if [[ -n `grep -w "ENABLE_SPINAND" .config` ]]; then
            sed -i 's/^ENABLE_SPINAND[[:space:]]*=[[:space:]]*y/ENABLE_SPINAND = n/' .config
        else
            sed -i '$a\ENABLE_SPINAND = n' .config
        fi
        if [[ -n `grep -w "ENABLE_NANDFLASH" .config` ]]; then
            sed -i 's/^ENABLE_NANDFLASH[[:space:]]*=[[:space:]]*y/ENABLE_NANDFLASH = n/' .config
        else
            sed -i '$a\ENABLE_NANDFLASH = n' .config
        fi
        if [[ -n `grep -w "ENABLE_ROOTFS_UBIFS" .config` ]]; then
            sed -i 's/^ENABLE_ROOTFS_UBIFS[[:space:]]*=[[:space:]]*y/ENABLE_ROOTFS_UBIFS = n/' .config
        else
            sed -i '$a\ENABLE_ROOTFS_UBIFS= n' .config
        fi
    fi

    if [[ ${BR2_MOD_CHIP_NNE} != y && ${BR2_MOD_CHIP_NNM} != y ]]; then
        if [[ "${CHIP}" != "vega" ]]; then
            if [[ -n `grep -w "CONFIG_STAGE2_COMPRESSED" .config` ]]; then
                sed -i 's/^CONFIG_STAGE2_COMPRESSED[[:space:]]*=[[:space:]]*n/CONFIG_STAGE2_COMPRESSED = y/' .config
            else
                sed -i '$a\CONFIG_STAGE2_COMPRESSED = y' .config
            fi
            if [[ -n `grep -w "CONFIG_STAGE2_COMPRESSED_LZMA" .config` ]]; then
                sed -i 's/^CONFIG_STAGE2_COMPRESSED_LZMA[[:space:]]*=[[:space:]]*n/CONFIG_STAGE2_COMPRESSED_LZMA = y/' .config
            else
                sed -i '$a\CONFIG_STAGE2_COMPRESSED_LZMA = y' .config
            fi
        fi
    fi

    make clean && make || exit 1

    if [[ ${BR2_MOD_NAND_FLASH} = y ]]; then
        if [[ ${BR2_MOD_SPI_NAND} = y ]]; then
            cp output/loader-spinand.bin ${GXSRC_PATH}/output/image/bin_${OS}/loader-sflash.bin || exit 1
        elif [[ ${BR2_MOD_P_NAND} = y ]]; then
            cp output/loader-nand.bin ${GXSRC_PATH}/output/image/bin_${OS}/loader-sflash.bin || exit 1
	fi
    else
        cp output/loader-*.bin ${GXSRC_PATH}/output/image/bin_${OS}/loader-sflash.bin || exit 1
    fi
    cd -
}

if [ -e ${GXSRC_PATH}/.config.old ]; then
    cmp_result=`diff -q ${GXSRC_PATH}/.config ${GXSRC_PATH}/.config.old`

    if [[ ! -z ${cmp_result} ]] || [ ! -f ${GXSRC_PATH}/output/image/bin_${OS}/loader-sflash.bin ]; then
        build_gxloader
        cp ${GXSRC_PATH}/.config ${GXSRC_PATH}/.config.old
    fi
else
    build_gxloader
    cp ${GXSRC_PATH}/.config ${GXSRC_PATH}/.config.old
fi
