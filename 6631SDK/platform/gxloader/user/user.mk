#########################################################

CPPFLAGS += -Iuser/include
LIBS     += -Luser/lib
MY_LIBS  +=

#########################################################

COBJS-y += user/user_config.o
COBJS-y += user/canopus_config.o
COBJS-y += user/vega_config.o
COBJS-y += user/scorpio_config.o
