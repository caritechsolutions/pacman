MEMORY
{
	sdram	:	ORIGIN = 0x07f50000,	LENGTH = 0xb0000
}
ENTRY(ResetEntry)
SECTIONS {
	. = ALIGN(4);
	.data :
	{
		cpu/arm/canopus/canopus_start.o(.text*)
		cpu/arm/canopus/canopus_pll_mini.o(.text*)
		cpu/arm/canopus/canopus_sdram.o(.text*)
		cpu/copy.o(.text*)
		cpu/arm/canopus/canopus_start.o(.rodata*)
		cpu/arm/canopus/canopus_pll_mini.o(.rodata*)
		cpu/arm/canopus/canopus_sdram.o(.rodata*)
		cpu/copy.o(.rodata*)
		cpu/arm/canopus/canopus_start.o(.data*)
		cpu/arm/canopus/canopus_pll_mini.o(.data*)
		cpu/arm/canopus/canopus_sdram.o(.data*)
		cpu/copy.o(.data*)
		*(.cache_section)
		*(.reset_patch)
		*(.sram_text)
		*(.sram)
		_stage2_start_ = .;
		. = ALIGN(4);
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

