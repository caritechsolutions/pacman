MEMORY
{
	sdram	:	ORIGIN = 0x92000000,	LENGTH = 0x100000
}
ENTRY(ResetEntry)
SECTIONS {
	. = ALIGN(4);
	.data :
	{
		cpu/ck/scorpio_taurus/scorpio_taurus_start.o(.text*)
		cpu/ck/scorpio_taurus/scorpio_taurus_pll_mini.o(.text*)
		cpu/ck/scorpio_taurus/scorpio_taurus_sdram.o(.text*)
		cpu/copy.o(.text*)
		cpu/ck/scorpio_taurus/scorpio_taurus_start.o(.rodata*)
		cpu/ck/scorpio_taurus/scorpio_taurus_pll_mini.o(.rodata*)
		cpu/ck/scorpio_taurus/scorpio_taurus_sdram.o(.rodata*)
		cpu/copy.o(.rodata*)
		cpu/ck/scorpio_taurus/scorpio_taurus_start.o(.data*)
		cpu/ck/scorpio_taurus/scorpio_taurus_pll_mini.o(.data*)
		cpu/ck/scorpio_taurus/scorpio_taurus_sdram.o(.data*)
		cpu/copy.o(.data*)
		*(.reset_patch)
		*(.sram_text)
		*(.sram)
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
		. = ALIGN(4);
		__bss_start__ = .;
		*(.bss*)
		*(COMMON)
		. = ALIGN(4);
		__bss_end__ = .;
	} > sdram
}

