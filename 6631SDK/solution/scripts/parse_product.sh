# Set chip status in production
if [[ $BR2_CHIP_GX3200 = y ]];then
	config_chip_product="nre"
    config_arch_product="arm"
fi

if [[ $BR2_CHIP_GX3201 = y ]];then
	config_chip_product="nre"
    config_arch_product="csky"
fi

if [[ $BR2_CHIP_GX6601 = y ]];then
	config_chip_product="nre"
    config_arch_product="csky"
fi

if [[ $BR2_CHIP_GX6602 = y ]];then
	config_chip_product="nre"
    config_arch_product="csky"
fi

if [[ $BR2_CHIP_GX3211 = y ]];then
	config_chip_product="nre"
    config_arch_product="csky"
fi

if [[ $BR2_CHIP_GX6605S = y ]];then
	config_chip_product="nre"
    config_arch_product="csky"
fi

if [[ $BR2_CHIP_SIRIUS = y ]];then
	config_chip_product="nre"
    config_arch_product="arm"
fi

if [[ $BR2_CHIP_TAURUS = y ]];then
	config_chip_product="nre"
    config_arch_product="csky"
fi

if [[ $BR2_CHIP_SCORPIO = y ]];then
	config_chip_product="nre"
    config_arch_product="csky"
fi

if [[ $BR2_CHIP_GEMINI = y ]];then
	config_chip_product="nre"
    config_arch_product="csky"
fi

if [[ $BR2_CHIP_CYGNUS = y ]];then
	config_chip_product="nre"
    config_arch_product="csky"
fi

if [[ $BR2_CHIP_CANOPUS = y ]];then
	config_chip_product="nre"
    config_arch_product="arm"
fi

if [[ $BR2_CHIP_VEGA = y ]];then
    config_chip_product="nre"
    config_arch_product="arm"
fi
