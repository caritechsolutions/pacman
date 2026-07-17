struct ddr_patch ddr2_qfn_533MHz_patch[] = {
	{DDR_KGD_WINBOND, Databahn_Reg(81),     0x00002424},
	{DDR_KGD_WINBOND, Databahn_Reg(256+02), 0x212100a0},
	{DDR_KGD_WINBOND, Databahn_Reg(256+04), 0x43038603},
	{DDR_KGD_WINBOND, Databahn_Reg(256+24), 0x00004305},
	{DDR_KGD_ETRON,   Databahn_Reg(81),     0x00002c2c},
	{DDR_KGD_ETRON,   Databahn_Reg(256+02), 0x212100a0},
	{DDR_KGD_ETRON,   Databahn_Reg(256+04), 0x43038a03},
	{DDR_KGD_ETRON,   Databahn_Reg(256+24), 0x00004305},
};
