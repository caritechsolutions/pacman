MEMORY
{
	sdram	:	ORIGIN = 0x9217e800,	LENGTH = 0x400000
}
ENTRY(ResetEntry)

SECTIONS {

	. = ALIGN(4);
	.text :
	{
		. = ALIGN(4);
		__text_start__ = .;
		*(.text*)
		*(.irq_text)
		*(.fiq_text)
		*(.entry_text)
		*(.cache_section)
		. = ALIGN(4);
		__text_end__ = .;
		__u_boot_cmd_begin = .;
		KEEP(*(SORT(.u_boot_list*)));
		. = ALIGN(4);
		__u_boot_cmd_end = .;
		__data_start__ = .;
		*(.data*)
		. = ALIGN(4);
		__data_end__ = .;
		__rodata_start__ = .;
		*(.rodata*)
		. = ALIGN(4);
		__rodata_end__ = .;
	} > sdram

	.mmu_tables ALIGN (0x4000) : { . = .; *(.mmu_tables) } > sdram

	. = ALIGN(4);
	.bss :
	{
		. = ALIGN(4);
		__bss_start__ = .;
		*(.bss*)
		*(COMMON)
		. = ALIGN(4);
		__bss_end__ = .;
	} > sdram
}
