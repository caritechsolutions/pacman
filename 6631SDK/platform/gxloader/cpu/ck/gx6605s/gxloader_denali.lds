MEMORY
{
	stage1	:	ORIGIN = 0x00100000,	LENGTH = 0x2000 - 512
	stage2	:	ORIGIN = 0x92880000,	LENGTH = 0x100000
}
ENTRY(ResetEntry)
SECTIONS {
	. = ALIGN(4);
	.text :
	{
		cpu/ck/gx6605s/gx6605s_start.o(.text*)
		cpu/ck/gx6605s/gx6605s_pll_mini.o(.text*)
		cpu/ck/gx6605s/gx6605s_pll_full.o(.text*)
		cpu/ck/gx6605s/gx6605s_sdram.o(.text*)
		cpu/copy.o(.text*)
		common/sec_rw.o(.text*)
		cpu/ck/gx6605s/gx6605s_start.o(.rodata*)
		cpu/ck/gx6605s/gx6605s_pll_mini.o(.rodata*)
		cpu/ck/gx6605s/gx6605s_pll_full.o(.rodata*)
		cpu/ck/gx6605s/gx6605s_sdram.o(.rodata*)
		cpu/copy.o(.rodata*)
		common/sec_rw.o(.rodata*)
		cpu/ck/gx6605s/gx6605s_start.o(.data*)
		cpu/ck/gx6605s/gx6605s_pll_mini.o(.data*)
		cpu/ck/gx6605s/gx6605s_pll_full.o(.data*)
		cpu/ck/gx6605s/gx6605s_sdram.o(.data*)
		cpu/copy.o(.data*)
		common/sec_rw.o(.data*)
		*(.reset_patch)
		*(.sram_text)
		*(.sram)
	} > stage1

	. = ALIGN(4);
	.data :
	{
		. = ALIGN(4);
		_stage2_start_ = .;
		__commands_begin = .;
		KEEP(*(.data.commands))
		. = ALIGN(4);
		__commands_end = .;
		_stage2_text_start_ = .;
		*(.entry_text)
		*(.irq_text)
		*(.fiq_text)
		*(.text*)
		. = ALIGN(4);
		_stage2_text_end_ = .;
		_stage2_data_start_ = .;
		*(.data*)
		*(.rodata*)
		. = ALIGN(4);
		_stage2_data_end_ = .;
	} > stage2

	. = ALIGN(4);
	.bss :
	{
		. = ALIGN(4);
		__bss_start__ = .;
		*(.bss*)
		*(COMMON)
		. = ALIGN(4);
		__bss_end__ = .;
	} > stage2
}

