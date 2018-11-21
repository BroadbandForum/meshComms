# Broadband Forum IEEE 1905.1/1a stack
# 
# Copyright (c) 2017, Broadband Forum
# 
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are
# met:
# 
# 1. Redistributions of source code must retain the above copyright
#    notice, this list of conditions and the following disclaimer.
# 
# 2. Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in the
#    documentation and/or other materials provided with the distribution.
# 
# Subject to the terms and conditions of this license, each copyright
# holder and contributor hereby grants to those receiving rights under
# this license a perpetual, worldwide, non-exclusive, no-charge,
# royalty-free, irrevocable (except for failure to satisfy the
# conditions of this license) patent license to make, have made, use,
# offer to sell, sell, import, and otherwise transfer this software,
# where such license applies only to those patent claims, already
# acquired or hereafter acquired, licensable by such copyright holder or
# contributor that are necessarily infringed by:
# 
# (a) their Contribution(s) (the licensed copyrights of copyright holders
#     and non-copyrightable additions of contributors, in source or binary
#     form) alone; or
# 
# (b) combination of their Contribution(s) with the work of authorship to
#     which such Contribution(s) was added by such copyright holder or
#     contributor, if, at the time the Contribution is added, such addition
#     causes such combination to be necessarily infringed. The patent
#     license shall not apply to any other combinations which include the
#     Contribution.
# 
# Except as expressly stated above, no rights or licenses from any
# copyright holder or contributor is granted under this license, whether
# expressly, by implication, estoppel or otherwise.
# 
# DISCLAIMER
# 
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
# IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
# TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
# PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
# HOLDERS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
# INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
# BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
# OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
# ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR
# TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE
# USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
# DAMAGE.

################################################################################
# Configuration section. Tune the parameters below according to your needs
################################################################################

# Platform dependent CONFIGs
#
# You can either "export PLATFORM=..." (and "FLAVOUR", if needed) before
# calling this Makefile or let "PLATFORM" undefined to use the default value
# (which is "linux")

ifndef PLATFORM

PLATFORM := linux
 FLAVOUR  := x86_generic
 #FLAVOUR  := arm_wrt1900acx
 #FLAVOUR  := x86_windows_mingw

endif


ifeq ($(PLATFORM),linux)
    ifeq ($(FLAVOUR), x86_generic)
        CC        := gcc
       #CC        := clang

        CCFLAGS   := -D_FLAVOUR_X86_GENERIC_

        LDFLAGS       += -lrt -lpthread   # For threads
        LDFLAGS       += -lpcap           # For packet capture
        LDFLAGS       += -lcrypto         # For WPS crypto

        AL_SUPPORTED  := yes
        HLE_SUPPORTED := yes
    else ifeq ($(FLAVOUR), arm_wrt1900acx)
        CC        := $(WRT1900_CROSS)gcc
        AR        := $(WRT1900_CROSS)ar
        CCFLAGS   := -I $(WRT1900_SYSROOT)/include/
        LDFLAGS   := -L $(WRT1900_SYSROOT)/lib/

        CCFLAGS   += -D_FLAVOUR_ARM_WRT1900ACX_

        LDFLAGS       += -lrt -lpthread   # For threads
        LDFLAGS       += -lpcap           # For packet capture
        LDFLAGS       += -lcrypto         # For WPS crypto

        AL_SUPPORTED  := yes
        HLE_SUPPORTED := yes
    else ifeq ($(FLAVOUR), x86_windows_mingw)
        CC        := $(MINGW_CROSS)gcc
        LD        := $(MINGW_CROSS)ld
        AR        := $(MINGW_CROSS)ar
        CCFLAGS   += -I $(MINGW_SYSROOT)/include/
        LDFLAGS   := -L $(MINGW_SYSROOT)/lib/

        CCFLAGS   += -D_FLAVOUR_X86_WINDOWS_MINGW_

        LDFLAGS       += -lws2_32 # Network stuff

        AL_SUPPORTED  := no
        HLE_SUPPORTED := yes
    else
        $(error "Linux FLAVOUR unknown")
    endif

    CCFLAGS       += -g -O0 -Wall -Werror #-Wextra
    CCFLAGS       += -D_HOST_IS_LITTLE_ENDIAN_=1 -DMAX_NETWORK_SEGMENT_SIZE=1500
    CCFLAGS       += -DINT8U="unsigned char"
    CCFLAGS       += -DINT16U="unsigned short int"
    CCFLAGS       += -DINT32U="unsigned int"
    CCFLAGS       += -DINT8S="signed char"
    CCFLAGS       += -DINT16S="signed short int"
    CCFLAGS       += -DINT32S="signed int"
    CCFLAGS       += -D_GNU_SOURCE
else
    $(error "'PLATFORM' variable was not set to a valid value")
endif

# Platform independent CONFIGs

#CCFLAGS += -DDO_NOT_ACCEPT_UNAUTHENTICATED_COMMANDS
CCFLAGS += -DSEND_EMPTY_TLVS
CCFLAGS += -DFIX_BROKEN_TLVS
CCFLAGS += -DSPEED_UP_DISCOVERY
  #
  # These are special flags that change the way the implementation behaves.
  # The README file contains more information regarding them.

CCFLAGS += -D_BUILD_NUMBER_=\"$(shell cat version.txt)\"
  #
  # Version flag to identify the binaries
CCFLAGS += -DREGISTER_EXTENSION_BBF
  #
  # These are special flags to enable Protocol extensions

################################################################################
# End of configuration section. Do not touch anything from this point on
################################################################################


# Shortcuts for later
#
SRC_FOLDER    := $(shell pwd)/src
OUTPUT_FOLDER := $(shell pwd)/output

# "Common" library. Includes platform specific code that is used by both the
# AL and HLE entities (example: "PLATFORM_MALLOC()", "PLATFORM_PRINTF()", ...)
# 
COMMON_LIB    := $(OUTPUT_FOLDER)/libcommon.a
COMMON_INC    := $(SRC_FOLDER)/common/interfaces

# "Factory" library. Includes "pure" 1905 related functions to parse/build 1905
# packets. This library is also used by both AL and HLE but most (if not all) of
# it is platform independent.
#
FACTORY_LIB   := $(OUTPUT_FOLDER)/libfactory.a
FACTORY_INC   := $(SRC_FOLDER)/factory/interfaces
FACTORY_INC   += $(sort $(dir $(wildcard $(SRC_FOLDER)/factory/interfaces/extensions/*/)))

# The actual binaries that we are going to build: one for the AL and another one
# for the HLE entity.
#
AL_EXE        := $(OUTPUT_FOLDER)/al_entity
HLE_EXE       := $(OUTPUT_FOLDER)/hle_entity

# Compilation will need to create new directories during the process
#
MKDIR         := mkdir -p

# Export all these variable to child Makefiles
#
export


################################################################################
# Targets
################################################################################

.PHONY: all
all:
ifeq ($(AL_SUPPORTED),yes)
all: $(AL_EXE)
endif
ifeq ($(HLE_SUPPORTED),yes)
all: $(HLE_EXE)
endif

.PHONY: $(COMMON_LIB)
$(COMMON_LIB):
	$(MAKE) -C src/common

.PHONY: $(FACTORY_LIB)
$(FACTORY_LIB):
	$(MAKE) -C src/factory 

$(AL_EXE): $(COMMON_LIB) $(FACTORY_LIB)
	$(MAKE) -C src/al

$(HLE_EXE): $(COMMON_LIB) $(FACTORY_LIB)
	$(MAKE) -C src/hle


.PHONY: unit_tests
unit_tests: all
	$(MAKE) -C src/factory unit_tests


.PHONY: clean
clean:
	$(MAKE) -C src/common  clean
	$(MAKE) -C src/factory clean
	$(MAKE) -C src/al      clean
	$(MAKE) -C src/hle     clean


.PHONY: static-analysis
static-analysis: clean
	scan-build $(MAKE) all
	cppcheck --enable=warning,style,performance,portability --inconclusive --xml-version=2 --error-exitcode=2 --suppress=memleakOnRealloc:* src

# See https://code.broadband-forum.org/scm/software/tools.git (needs username/password) for
# the licenses/add-license.py tool
.PHONY: update-license
update-license:
	add-license.py --exclude=version.txt --replace-summary
