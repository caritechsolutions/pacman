
CC= $(CROSS_COMPILE)gcc
AS= $(CROSS_COMPILE)as
AR= $(CROSS_COMPILE)ar
LD= $(CROSS_COMPILE)ld
CP= $(CROSS_COMPILE)objcopy
DP= $(CROSS_COMPILE)objdump
TextBaseAddr = 0x00000000

CSOURCE = $(shell find ./ -name "*.c")
SSOURCE = $(shell find ./ -name "*.S")
COBJ = $(subst .c,.o,$(CSOURCE))
SOBJ = $(subst .S,.o,$(SSOURCE))
CPPFLAGS = -I include

LDFLAGS = -EL --gc-sections -nostblib
CFLAGS  = $(CPPFLAGS) -mlittle-endian  -Wpointer-arith -Wundef -O2 -c -Wall -Wundef -Werror -Wstrict-prototypes -g -pipe -mcpu=ck610 -ffunction-sections 

ARFLAGS  = -rcs


all: main.elf gxlowpower.fw main.txt

gxlowpower.fw:gxlowpower.bin
	@echo [create $@]
	@mv $^ $@

main.txt:main.elf
	@echo [create $@]
	@$(DP) -S $^ > $@
	
gxlowpower.bin: main.elf
	@echo [change section .text=$(TextBaseAddr) and create $@]
	@$(CP)  $^ -O binary gxlowpower.bin --change-section-lma  .text=$(TextBaseAddr)

main.elf: $(SOBJ) $(COBJ)
	@echo [create $@]
	@$(LD) $(LDFLAGS) -T link.ld $^  -o $@
	@rm -f src/*.o src/*.d *.o *.d

%.o : %.S
	@echo [Compling $@]
	@$(CC) $(CFLAGS) $< -o $@
%.o : %.c
	@echo [Compling $@]
	@$(CC) $(CFLAGS) $< -o $@

ifneq ($(MAKECMDGOALS), clean)
-include $(subst .c,.d,$(CSOURCE))
-include $(subst .S,.d,$(SSOURCE))
endif

%.d : %.S
	@$(CC) $(CPPFLAGS) -M $< > $@.12345;
	@sed 's,\($*\)\.o[:]*,\1.o $@ : ,g' <$@.12345 > $@;
	@rm -f $@.12345

%.d : %.c
	@$(CC) $(CPPFLAGS) -M $< > $@.12345;
	@sed 's,\($*\)\.o[:]*,\1.o $@ : ,g' <$@.12345 > $@;
	@rm -f $@.12345

.PHONY:clean
clean:
	@echo [Clean all]
	@rm -f *.o *.elf *.d
	@rm -f src/*.o src/*.d
	@rm -f gxlowpower.fw main.txt

