all:generate_param

HOSTCC := gcc
HOSTCPPFLAGS := -Ioutput
HOSTCFLAGS := -O2 -g 

hostobj-y = generate_param.c
hostobj-y += output/board-param.c
hostobj-y += output/vega_clock_param.c
hostobj-y += output/vega_dram_param.c
hostobj-y += output/vega_dram_phase.c
hostobj-y += output/vega_dram_phase_backup.c

generate_param:$(hostobj-y)
	@$(HOSTCC) $(HOSTCPPFLAGS) $(HOSTCFLAGS) $^ -o $@
