MEMORY
{
	stage1	:	ORIGIN = 0x80400000,	LENGTH = 0x2000 - 512
	stage2	:	ORIGIN = 0x7f60000,	LENGTH = 0x100000
}
ENTRY(ResetEntry)
SECTIONS {
	. = ALIGN(4);
	.text :
	{
		cpu/arm/sirius/sirius_start.o(.text*)
		cpu/arm/sirius/sirius_pll_mini.o(.text*)
		cpu/arm/sirius/sirius_pll_full.o(.text*)
		cpu/arm/sirius/sirius_sdram.o(.text*)
		drivers/gx_otp/sirius_otp.o(.text*)
		cpu/copy.o(.text*)
		cpu/arm/sirius/sirius_start.o(.rodata*)
		cpu/arm/sirius/sirius_pll_mini.o(.rodata*)
		cpu/arm/sirius/sirius_pll_full.o(.rodata*)
		cpu/arm/sirius/sirius_sdram.o(.rodata*)
		drivers/gx_otp/sirius_otp.o(.rodata*)
		cpu/copy.o(.rodata*)
		cpu/arm/sirius/sirius_start.o(.data*)
		cpu/arm/sirius/sirius_pll_mini.o(.data*)
		cpu/arm/sirius/sirius_pll_full.o(.data*)
		cpu/arm/sirius/sirius_sdram.o(.data*)
		drivers/gx_otp/sirius_otp.o(.data*)
		cpu/copy.o(.data*)
		*(.cache_section)
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

