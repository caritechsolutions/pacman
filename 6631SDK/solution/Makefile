#  Makefile for buildroot2
#
# Copyright (C) 1999-2005 by Erik Andersen <andersen@codepoet.org>
# Copyright (C) 2006-2012 by the Buildroot developers <buildroot@uclibc.org>
# Copyright (C) 2013-2015 by Blacker <liujh@nationalchip.com>
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
# General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
#

#--------------------------------------------------------------
# Just run 'make menuconfig', configure stuff, then run 'make'.
# You shouldn't need to mess with anything beyond this point...
#--------------------------------------------------------------

# Set and export the version string
export BR2_VERSION:=2013.11.26_REL

# Check for minimal make version (note: this check will break at make 10.x)
MIN_MAKE_VERSION=3.81
ifneq ($(firstword $(sort $(MAKE_VERSION) $(MIN_MAKE_VERSION))),$(MIN_MAKE_VERSION))
$(error You have make '$(MAKE_VERSION)' installed. GNU make >= $(MIN_MAKE_VERSION) is required)
endif

export HOSTARCH := $(shell uname -m | \
	sed -e s/i.86/x86/ \
	    -e s/sun4u/sparc64/ \
	    -e s/arm.*/arm/ \
	    -e s/sa110/arm/ \
	    -e s/ppc64/powerpc/ \
	    -e s/ppc/powerpc/ \
	    -e s/macppc/powerpc/\
	    -e s/sh.*/sh/)

# This top-level Makefile can *not* be executed in parallel
.NOTPARALLEL:

# absolute path
TOPDIR:=$(shell pwd)
CONFIG_CONFIG_IN=Config.in
CONFIG_CONFIG_IN_PROTECT=Config.in.protect
CONFIG=tools/support/kconfig
DATE:=$(shell date +%Y%m%d)

# Compute the full local version string so packages can use it as-is
# Need to export it, so it can be got from environment in children (eg. mconf)
export BR2_VERSION_FULL:=$(BR2_VERSION)$(shell $(TOPDIR)/tools/support/scripts/setlocalversion)

noconfig_targets:=menuconfig nconfig config \
	defconfig %_defconfig release %_release\
	print_version

# Strip quotes and then whitespaces
#qstrip=$(strip $(subst ",,$(1)))#"))

# Variables for use in Make constructs
comma:=,
empty:=
space:=$(empty) $(empty)

J=-j$(shell echo `nproc` - 1 | bc)
ifeq ($(J), -j0)
J:=-j1
endif

ifneq ("$(origin O)", "command line")
O:=output
CONFIG_DIR:=$(TOPDIR)
else
override O:=$(O)
CONFIG_DIR:=$(O)
# we need to pass O= everywhere we call back into the toplevel makefile
EXTRAMAKEARGS = O=$(O)
endif

# Pull in the user's configuration file
ifeq ($(filter $(noconfig_targets),$(MAKECMDGOALS)),)
-include $(CONFIG_DIR)/.config
endif

# To put more focus on warnings, be less verbose as default
# Use 'make V=1' to see the full commands
ifdef V
  ifeq ("$(origin V)", "command line")
    KBUILD_VERBOSE=$(V)
  endif
endif

ifndef KBUILD_VERBOSE
  KBUILD_VERBOSE=0
endif

ifeq ($(KBUILD_VERBOSE),1)
  quiet=
  Q=
ifndef VERBOSE
  VERBOSE=1
endif
else
  quiet=quiet_
  Q=@
endif

export quiet Q KBUILD_VERBOSE VERBOSE

ifndef HOSTAR
HOSTAR:=ar
endif
ifndef HOSTAS
HOSTAS:=as
endif
ifndef HOSTCC
HOSTCC:=gcc
HOSTCC:=$(shell which $(HOSTCC) || type -p $(HOSTCC) || echo gcc)
endif
HOSTCC_NOCCACHE:=$(HOSTCC)
ifndef HOSTCXX
HOSTCXX:=g++
HOSTCXX:=$(shell which $(HOSTCXX) || type -p $(HOSTCXX) || echo g++)
endif
HOSTCXX_NOCCACHE:=$(HOSTCXX)
ifndef HOSTFC
HOSTFC:=gfortran
endif
ifndef HOSTCPP
HOSTCPP:=cpp
endif
ifndef HOSTLD
HOSTLD:=ld
endif
ifndef HOSTLN
HOSTLN:=ln
endif
ifndef HOSTNM
HOSTNM:=nm
endif
HOSTAR:=$(shell which $(HOSTAR) || type -p $(HOSTAR) || echo ar)
HOSTAS:=$(shell which $(HOSTAS) || type -p $(HOSTAS) || echo as)
HOSTFC:=$(shell which $(HOSTLD) || type -p $(HOSTLD) || echo || which g77 || type -p g77 || echo gfortran)
HOSTCPP:=$(shell which $(HOSTCPP) || type -p $(HOSTCPP) || echo cpp)
HOSTLD:=$(shell which $(HOSTLD) || type -p $(HOSTLD) || echo ld)
HOSTLN:=$(shell which $(HOSTLN) || type -p $(HOSTLN) || echo ln)
HOSTNM:=$(shell which $(HOSTNM) || type -p $(HOSTNM) || echo nm)

export HOSTAR HOSTAS HOSTCC HOSTCXX HOSTFC HOSTLD
export HOSTCC_NOCCACHE HOSTCXX_NOCCACHE

# Make sure pkg-config doesn't look outside the buildroot tree
unexport PKG_CONFIG_PATH
unexport PKG_CONFIG_SYSROOT_DIR

# Having DESTDIR set in the environment confuses the installation
# steps of some packages.
unexport DESTDIR

# bash prints the name of the directory on 'cd <dir>' if CDPATH is
# set, so unset it here to not cause problems. Notice that the export
# line doesn't affect the environment of $(shell ..) calls, so
# explictly throw away any output from 'cd' here.
#export CDPATH:=
BASE_DIR := $(shell mkdir -p $(O) && cd $(O) >/dev/null && pwd)
$(if $(BASE_DIR),, $(error output directory "$(O)" does not exist))

BUILD_DIR:=$(BASE_DIR)
export BUILD_DIR
OUTPUT_DIR:=$(BASE_DIR)
export OUTPUT_DIR

ifeq ($(BR2_HAVE_DOT_CONFIG),y)

GNU_HOST_NAME:=$(shell tools/support/gnuconfig/config.guess)

##############################################################
#
# The list of stuff to build for the target toolchain
# along with the packages to build for the target.
#
##############################################################
TARGETS:=

# silent mode requested?
QUIET:=$(if $(findstring s,$(MAKEFLAGS)),-q)

# Strip off the annoying quoting
ARCH:=$(call qstrip,$(BR2_ARCH))

# packages compiled for the host go here
HOST_DIR:=$(call qstrip,$(BR2_HOST_DIR))

TARGET_DIR:=$(BASE_DIR)/

all: env build_elf
##############################################################################
#
# for DVB-S2 HD Application
#
#
##############################################################################

#
# Set Chip Arch
#
ifeq ($(BR2_CHIP_GX3211), y)
ARCH:=csky
CHIP:=GX3211
endif

ifeq ($(BR2_CHIP_SIRIUS), y)
ARCH:=arm
CHIP:=SIRIUS
endif

ifeq ($(BR2_CHIP_CANOPUS), y)
ARCH:=arm
CHIP:=CANOPUS
endif

ifeq ($(BR2_CHIP_VEGA), y)
ARCH:=arm
CHIP:=VEGA
endif

ifeq ($(BR2_CHIP_TAURUS), y)
ARCH:=csky
CHIP:=TAURUS
endif

ifeq ($(BR2_CHIP_SCORPIO), y)
ARCH:=csky
CHIP:=SCORPIO
endif

ifeq ($(BR2_CHIP_SCORPIOTAURUS), y)
ARCH:=csky
CHIP:=SCORPIO-TAURUS
endif

ifeq ($(BR2_CHIP_GEMINI), y)
ARCH:=csky
CHIP:=GEMINI
endif

ifeq ($(BR2_CHIP_CYGNUS), y)
ARCH:=csky
CHIP:=CYGNUS
endif

ifeq ($(BR2_CHIP_GX6602), y)
ARCH:=csky
CHIP:=GX6602
endif

ifeq ($(BR2_CHIP_GX6601), y)
ARCH:=csky
CHIP:=GX6601
endif

ifeq ($(BR2_CHIP_GX3201), y)
ARCH:=csky
CHIP:=GX3201
endif

ifeq ($(BR2_CHIP_GX6605S), y)
ARCH:=csky
CHIP:=GX6605S
endif


ifeq ($(BR2_CHIP_GX3200), y)
ARCH:=arm
CHIP:=GX3200
endif

ifeq ($(BR2_CHIP_GX3113B), y)
ARCH:=arm
CHIP:=GX3113
endif

#
# Set Operating System
#
ifeq ($(BR2_OS_ECOS), y)
OS:=ecos
endif

ifeq ($(BR2_OS_LINUX), y)
OS:=linux
endif

CROSS_PATH:=$(ARCH)-$(OS)
GXSRC_PATH:=$(shell pwd)

check_lib_path=$(shell if [ -e $(GXSRC_PATH)/../library/goxceed ]; then echo "goxceed"; fi)
ifeq ($(check_lib_path),goxceed)
GXLIB_PATH:=$(GXSRC_PATH)/../library/goxceed/$(CROSS_PATH)
else
GXLIB_PATH:=$(OPT)/opt/goxceed/$(CROSS_PATH)
endif
#######################
export ARCH
export OS
export CHIP
export GXSRC_PATH
export GXLIB_PATH

export BR2_FAMILY_NAME
export BR2_PROJ_NAME
export BR2_MOD_APP_PLUGINSTORE
export BR2_MOD_APP_APPLETS
SUBDIRS=app

##############################################################################
-include  $(GXSRC_PATH)/scripts/inc.Makefile.conf.mak
##############################################################################

print_info:
	@echo -e "\e[032mProject infomation\e[0m"
	@echo "Arch:" $(ARCH)
	@echo "OS:" $(OS)
	@echo "CROSS:" $(CROSS_PATH)
	@echo "Verdor:" $(BR2_FAMILY_NAME)
	@echo "Project:" $(BR2_PROJ_NAME)
	@echo "SRC PATH:" $(GXSRC_PATH)
	@echo "LIB PATH:" $(GXLIB_PATH)

log_info:
	@echo "    [loglevel List] : "
	@echo "                  n=0    - not  print"
	@echo "                  n=1    - error print"
	@echo "                  n=2    - error info print"
	@echo "                  n=3    - error info warning print"
	@echo "                  n=4    - error info warning debug print"
	@echo "                  n=flow - error info flow print"
#
#build elf for ecos
#
ifeq ($(OS), ecos)
build_elf:post_config subdirs
	@echo "build $(OS) app ok ..."
	@echo "see output directory"

endif

#
#build elf for linux
#
ifeq ($(OS), linux)
USERDIR=dvb

build_elf:post_config subdirs
	@echo "build $(OS) app ok ... "
	@echo "see output directory"

endif

.PHONY: build_elf print_info log_info bin all dirs clean distclean \
	post_config protect_config prepare_config \
	$(BUILD_DIR)  $(TARGET_DIR) $(HOST_DIR)

else # ifeq ($(BR2_HAVE_DOT_CONFIG),y)

clean:
	@echo "Has been cleared"

all: envconfig
	make clean
	make $J build_elf

#############################################################
#
# Configurations
#
#############################################################

$(BUILD_DIR)/config/%onf:
	mkdir -p $(@D)/lxdialog
	$(MAKE) CC="$(HOSTCC_NOCCACHE)" HOSTCC="$(HOSTCC_NOCCACHE)" obj=$(@D) -C $(CONFIG) -f Makefile.br $(@F)

COMMON_CONFIG_ENV = \
	KCONFIG_AUTOCONFIG=$(BUILD_DIR)/config/auto.conf \
	KCONFIG_AUTOHEADER=$(BUILD_DIR)/config/autoconf.h \
	KCONFIG_TRISTATE=$(BUILD_DIR)/config/tristate.config \
	BUILDROOT_CONFIG=$(CONFIG_DIR)/.config


menuconfig :$(BUILD_DIR)/config/mconf prepare_config
	@mkdir -p $(BUILD_DIR)/config
	@$(COMMON_CONFIG_ENV) $< $(CONFIG_CONFIG_IN)

nconfig : $(BUILD_DIR)/config/nconf prepare_config
	@mkdir -p $(BUILD_DIR)/config
	@$(COMMON_CONFIG_ENV) $< $(CONFIG_CONFIG_IN)

config : $(BUILD_DIR)/config/conf prepare_config
	@mkdir -p $(BUILD_DIR)/config
	@$(COMMON_CONFIG_ENV) $< $(CONFIG_CONFIG_IN)

oldconfig: $(BUILD_DIR)/config/conf prepare_config
	mkdir -p $(BUILD_DIR)/config
	@$(COMMON_CONFIG_ENV) $< --oldconfig $(CONFIG_CONFIG_IN)

randconfig: $(BUILD_DIR)/config/conf prepare_config
	@mkdir -p $(BUILD_DIR)/config
	@$(COMMON_CONFIG_ENV) $< --randconfig $(CONFIG_CONFIG_IN)

allyesconfig: $(BUILD_DIR)/config/conf prepare_config
	@mkdir -p $(BUILD_DIR)/config
	@$(COMMON_CONFIG_ENV) $< --allyesconfig $(CONFIG_CONFIG_IN)

allnoconfig: $(BUILD_DIR)/config/conf prepare_config
	@mkdir -p $(BUILD_DIR)/config
	@$(COMMON_CONFIG_ENV) $< --allnoconfig $(CONFIG_CONFIG_IN)

defconfig: $(BUILD_DIR)/config/conf prepare_config
	@mkdir -p $(BUILD_DIR)/config
	@$(COMMON_CONFIG_ENV) $< --defconfig$(if $(BR2_DEFCONFIG),=$(BR2_DEFCONFIG)) $(CONFIG_CONFIG_IN)

%_defconfig: $(BUILD_DIR)/config/conf prepare_config $(TOPDIR)/configs/%_defconfig
	@mkdir -p $(BUILD_DIR)/config
	@$(COMMON_CONFIG_ENV) $< --defconfig=$(TOPDIR)/configs/$@ $(CONFIG_CONFIG_IN)

savedefconfig: $(BUILD_DIR)/config/conf prepare_config
	@mkdir -p $(BUILD_DIR)/config
	@$(COMMON_CONFIG_ENV) $< --savedefconfig=$(CONFIG_DIR)/defconfig $(CONFIG_CONFIG_IN)

endif # ifeq ($(BR2_HAVE_DOT_CONFIG),y)

#############################################################
#
# Cleanup and misc junk
#
#############################################################

envconfig:
	@scripts/env_config.sh

prepare_config:
	@chmod +x scripts/factory_config_proj.sh
	@scripts/factory_config_proj.sh
	@chmod +x scripts/ca_choose.sh
	@scripts/ca_choose.sh
	@chmod +x scripts/factory_config_theme.sh
	@scripts/factory_config_theme.sh
	@chmod +x scripts/factory_config_area.sh
	@scripts/factory_config_area.sh
	@chmod +x $(TOPDIR)/scripts/parse_netapps_configin.py
	@$(TOPDIR)/scripts/parse_netapps_configin.py  $(TOPDIR)

post_config:
	@chmod +x scripts/config_parse.sh
	@scripts/config_parse.sh $(loglevel) $(CROSS_COMPILE)
	@chmod +x scripts/factory_makefile_lib.sh
	@scripts/factory_makefile_lib.sh $(CROSS_PATH)
	@chmod +x scripts/create_signal_connect.sh
	@scripts/create_signal_connect.sh $(CROSS_COMPILE)

protect_config: $(BUILD_DIR)/config/mconf
	@mkdir -p $(BUILD_DIR)/config
	@echo "projects/"$(BR2_FAMILY_NAME)"/"$(BR2_PROJ_NAME)
	@$(MAKE) -C $(GXSRC_PATH)/system/configin protect_config

help:
	@echo 'Cleaning:'
	@echo '  clean                  - delete all files created by build'
	@echo '  distclean              - delete all non-source files (including .config)'
	@echo
	@echo
	@echo 'Configuration:'
	@echo '  menuconfig             - interactive curses-based configurator'
	@echo '  nconfig                - interactive ncurses-based configurator'
	@echo '  config                 - interactive configurator'
	@echo '  protect_config         - interactive protect-based configurator'
	@echo '  post_config            - interactive protect-based configurator'
	@echo '  envconfig              - Choose one from below for solution config'
	@echo
	@echo
	@echo 'Info:'
	@echo '  print_version          -  Print scheme version information'
	@echo '  list_family            -  Family list information'
	@echo '  list_theme             -  Theme list information'
	@echo '  print_info             -  Current scheme configuration information' 
	@echo
	@echo
	@echo 'Project:'
	@echo '  all                    -  Compile the entire project'
	@echo '  bin                    -  Compile to project file '
	@echo '  build_elf              -  Generate out ELF file'
	@echo '  lowpower               -  Compile and generate low-power module gxlowpower fw' 
	@echo
	@echo "loglevel:"
	@echo '  loglevel=0             - not  print'
	@echo '  loglevel=1             - error print'
	@echo '  loglevel=2             - error info print'
	@echo '  loglevel=3             - error info warning print'
	@echo '  loglevel=4             - error info warning debug print'
	@echo '  loglevel=flow          - error info flow print'
	@echo
	@echo 'Miscellaneous:'
	@echo '  make V=0|1             - 0 => quiet build (default), 1 => verbose build'
	@echo '  make O=dir             - Locate all output files in "dir", including .config'
	@echo
	@echo
	@echo 'Preconfiged:'
	@$(foreach b, $(sort $(notdir $(wildcard $(TOPDIR)/configs/*_defconfig))), \
	  printf "  %-35s - Configure for %s\\n" $(b) $(b:_defconfig=);)
	@echo
	@echo

# Create release tarballs. We need to fiddle a bit to add the generated
# documentation to the git output

print_version:
	@echo $(BR2_VERSION_FULL)

list_family:
	@echo "Family List:"
	@ls -l projects |awk '/^d/{print "    "$$NF}'


list_theme:
	@echo "Theme List:"
	@ls -l theme |awk '/^d/{print "    "$$NF}'

distclean:
	@echo "Distcleaning ..."
	@echo "remove "$(OUTPUT_DIR)
	@rm -rf $(OUTPUT_DIR)
	@echo "remove .config .config.old"
	@rm -f .config .config.old
	@rm -f app/deps  app/signal_connect.c app/include/app_config.h app/app_config.c
	@rm -f system/configin/.config system/configin/.config.old
	@rm -f system/configin/Config.in.proj
	@rm -f system/configin/Config.in.theme
	@rm -f system/configin/Config.in.lang*
	@rm -f system/configin/Config.in.netapps

lowpower:
	@cd ../platform/gxlowpower/;./build clean;./build `echo ${CHIP} | tr A-Z a-z`
	@cp `find ../platform/gxlowpower/ -name gxlowpower.fw` $(TOPDIR)/projects/$(BR2_FAMILY_NAME)/$(BR2_PROJ_NAME)/firmware/

.PHONY: $(noconfig_targets)

