CC = $(CROSS_COMPILE)

CSOURCE = $(shell find ./ -name "*.c")
#FILTER_SOURCE = ./driver/front_panel/drvFD650.c
FILTER_SOURCE += ./driver/front_panel/drvFD650_mini.c
CSOURCE := $(filter-out $(FILTER_SOURCE), $(CSOURCE))
COBJ = $(subst .c,.rel,$(CSOURCE))
CHIP_VAR = $(shell echo $(CHIP) | tr '[a-z]' '[A-Z]')
CPPFLAGS =
CPPFLAGS = -D$(CHIP_VAR)_LOWPOWER
CPPFLAGS += -Iarch/51 -Idriver/gpio -Idriver/irr -Idriver/cec -Iinclude \
		   -Idriver/timer -I./ -Idriver/front_panel
CFLAGS = $(CPPFLAGS) -c

all : gxlowpower.fw

gxlowpower.fw : main.bin
	@echo [create $@]
	@cp $^ $@

main.bin : main.hex
	@echo [create $@]
	@objcopy -I ihex -O binary $^ $@

main.hex : main.ihx
	@echo [create $@]
	@packihx $< > $@

main.ihx : $(COBJ)
	@echo [create $@]
	@$(CC) $^ --code-size 8192 -o $@

%.rel : %.c
	@echo [Compling $@]
	@$(CC) $(CFLAGS) $< -o $@

.PHONY : clean
clean:
	@echo [Clean all]
	@find -name "*.asm" | xargs rm -f
	@find -name "*.lst" | xargs rm -f
	@find -name "*.mem" | xargs rm -f
	@find -name "*.rst" | xargs rm -f
	@find -name "*.lnk" | xargs rm -f
	@find -name "*.rel" | xargs rm -f
	@find -name "*.sym" | xargs rm -f
	@find -name "*.ihx" | xargs rm -f
	@find -name "*.hex" | xargs rm -f
	@find -name "*.map" | xargs rm -f
	@find -name "*.lk" | xargs rm -f
	@find -name "*.bin" | xargs rm -f
	@find -name "*.fw" | xargs rm -f
	@find -name "*.d" | xargs rm -f
