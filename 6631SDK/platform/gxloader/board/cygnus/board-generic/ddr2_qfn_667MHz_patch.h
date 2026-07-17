struct ddr_patch ddr2_qfn_667MHz_patch[] = {
	{DDR_KGD_ETRON, Databahn_Reg(81),     0x00002828},
	{DDR_KGD_ETRON, Databahn_Reg(256+04), 0x43039e03},
	{DDR_KGD_ETRON, Databahn_Reg(256+24), 0x00004305},
};
