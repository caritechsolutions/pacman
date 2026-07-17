echo "Enter into MTD Partition"

source .config

if [[ $BR2_BOARD_FLASH_SPI_4MB = y || $BR2_BOARD_FLASH_SPI_8MB = y || $BR2_BOARD_FLASH_SPI_16MB = y ]];then
	echo "we use spi flash"
	echo "SPI Flash Size:" $BR2_BOARD_FLASH_SPI_SIZE
fi

if [[ $BR2_MTDPART_SPI_BOOT_SELECTED = y ]];then
	echo "BOOT:" $BR2_MTDPART_SPI_BOOT_SIZE
fi
