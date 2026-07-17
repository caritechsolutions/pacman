MEMORY
{
 stage1 : ORIGIN = 0x80400000, LENGTH = 0x4000 - 32
 stage2 : ORIGIN = 0x07f40000, LENGTH = 0xc0000
}
ENTRY(ResetEntry)
SECTIONS {
 . = ALIGN(4);
 .text :
 {
  _stage1_start_ = .;
  cpu/arm/canopus/canopus_start.o(.text*)
  cpu/arm/canopus/canopus_pll_mini.o(.text*)
  cpu/arm/canopus/canopus_pll_full.o(.text*)
  cpu/arm/canopus/canopus_sdram.o(.text*)
  cpu/arm/canopus/ddr_soft_training.o(.text*)
  cpu/copy.o(.text*)
  common/secure/common.o(.text*)
  cpu/arm/canopus/canopus_start.o(.rodata*)
  cpu/arm/canopus/canopus_pll_mini.o(.rodata*)
  cpu/arm/canopus/canopus_pll_full.o(.rodata*)
  cpu/arm/canopus/canopus_sdram.o(.rodata*)
  cpu/arm/canopus/ddr_soft_training.o(.rodata*)
  cpu/copy.o(.rodata*)
  common/secure/common.o(.rodata*)
  cpu/arm/canopus/canopus_start.o(.data*)
  cpu/arm/canopus/canopus_pll_mini.o(.data*)
  cpu/arm/canopus/canopus_pll_full.o(.data*)
  cpu/arm/canopus/canopus_sdram.o(.data*)
  cpu/arm/canopus/ddr_soft_training.o(.data*)
  cpu/copy.o(.data*)
  common/secure/common.o(.data*)
  *(.cache_section)
  *(.reset_patch)
  *(.sram_text)
  *(.sram)
  _stage1_end_ = .;
 } > stage1
 . = ALIGN(2048);
 .data :
 {
  _stage2_start_ = .;
  _stage2_text_start_ = .;
  *(.irq_text)
  . = ALIGN(512);
  *(.fiq_text)
  . = ALIGN(512);
  *(.entry_text)
  . = ALIGN(4);
  *(.text*)
  . = ALIGN(4);
  _stage2_text_end_ = .;
  __commands_begin = .;
  KEEP(*(.data.commands))
  . = ALIGN(4);
  __commands_end = .;
  _stage2_data_start_ = .;
  *(.data*)
  *(.rodata*)
  . = ALIGN(16);
  _stage2_data_end_ = .;
  _stage2_end_ = .;
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
