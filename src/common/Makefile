# Broadband Forum IEEE 1905.1/1a stack
# 
# Copyright (c) 2017, Broadband Forum
# 
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
# 
#     http://www.apache.org/licenses/LICENSE-2.0
# 
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

# When calling this Makefile, the following environment variables must be set:
#
#   CC ------------> Path to the compiler
#   CCFLAGS -------> Extra flags to use while compiling
#
#   PLATFORM      -> Possible values: "linux"
#   OUTPUT_FOLDER -> Absolute path to the folder where binaries will be built
#
#   COMMON_LIB ----> Absolute path to the "common library" *.a file
#
#   MKDIR ---------> Tool to create a directory
#

LIB := $(COMMON_LIB)

COMMON_DIRECTORY := common

SRC_INDEPENDENT  := $(wildcard src_independent/*.c)
SRC_PLATFORM     := $(wildcard src_$(PLATFORM)/*.c)

OBJ_INDEPENDENT  := $(patsubst %.c,$(OUTPUT_FOLDER)/tmp/$(COMMON_DIRECTORY)/%.o,$(SRC_INDEPENDENT))
OBJ_PLATFORM     := $(patsubst %.c,$(OUTPUT_FOLDER)/tmp/$(COMMON_DIRECTORY)/%.o,$(SRC_PLATFORM))

INTERNAL_INC_INDEPENDENT := src_independent interfaces
INTERNAL_INC_PLATFORM    := src_$(PLATFORM) interfaces

EXTERNAL_INC     :=

HDR_INDEPENDENT  := $(shell find $(INTERNAL_INC_INDEPENDENT) $(EXTERNAL_INC) -name *.h)
HDR_PLATFORM     := $(shell find $(INTERNAL_INC_PLATFORM)    $(EXTERNAL_INC) -name *.h)


################################################################################
# Targets
################################################################################

.PHONY: all
all: $(LIB)


$(LIB) : $(OBJ_INDEPENDENT) $(OBJ_PLATFORM)
	$(AR) -rc $@ $^


$(OBJ_INDEPENDENT) : $(OUTPUT_FOLDER)/tmp/$(COMMON_DIRECTORY)/%.o : %.c $(HDR_INDEPENDENT)
	$(foreach directory, $(sort $(dir $(wildcard $(SRC_INDEPENDENT)))), $(MKDIR) $(OUTPUT_FOLDER)/tmp/$(COMMON_DIRECTORY)/$(directory);)
	$(CC) $(CCFLAGS) -c $(addprefix -I,$(INTERNAL_INC_INDEPENDENT) $(EXTERNAL_INC)) $< -o $@

$(OBJ_PLATFORM)    : $(OUTPUT_FOLDER)/tmp/$(COMMON_DIRECTORY)/%.o : %.c $(HDR_PLATFORM)
	$(foreach directory, $(sort $(dir $(wildcard $(SRC_PLATFORM)))), $(MKDIR) $(OUTPUT_FOLDER)/tmp/$(COMMON_DIRECTORY)/$(directory);)
	$(CC) $(CCFLAGS) -c $(addprefix -I,$(INTERNAL_INC_PLATFORM) $(EXTERNAL_INC)) $< -o $@


.PHONY: clean
clean:
	rm -rf $(LIB)
	rm -rf $(OUTPUT_FOLDER)/tmp/$(COMMON_DIRECTORY)

