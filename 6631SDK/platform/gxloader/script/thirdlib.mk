LIBS+= -L$(GX_PREFIX)/$(ARCH)-nos/lib --start-group -ldownloader -lgxfrontend -lgxcore -lui -lgxav -lgxsecure -lgxosal --end-group
ifeq ($(ENABLE_THIRDLIB_PANEL), y)
LIBS += -lgxpanel
endif

CPPFLAGS += -I$(GX_PREFIX)/$(ARCH)-nos/include
