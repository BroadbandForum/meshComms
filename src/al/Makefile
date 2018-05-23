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
#   LDGLAGS -------> Extra flags to use while linking
#
#   PLATFORM      -> Possible values: "linux"
#   OUTPUT_FOLDER -> Absolute path to the folder where binaries will be built
#
#   COMMON_LIB ----> Absolute path to the "common library" *.a file
#   COMMON_INC ----> Absolute path to the folder containing common (to all
#                    sub-projects) header files
#
#   FACTORY_LIB   -> Absolute path to the "factory library" *.a file
#   FACTORY_INC   -> Absolute path to the folder containing the "factory
#                    library" header files
#
#   AL_EXE --------> Absolute path to the final AL entity binary
#
#   MKDIR ---------> Tool to create a directory
#

EXE                        := $(AL_EXE)

AL_DIRECTORY               := al

SRC_INDEPENDENT            := $(wildcard src_independent/*.c)
SRC_INDEPENDENT            += $(wildcard src_independent/extensions/*/*.c)
SRC_PLATFORM               := $(wildcard src_$(PLATFORM)/*.c)

OBJ_INDEPENDENT            := $(patsubst %.c,$(OUTPUT_FOLDER)/tmp/$(AL_DIRECTORY)/%.o,$(SRC_INDEPENDENT))
OBJ_PLATFORM               := $(patsubst %.c,$(OUTPUT_FOLDER)/tmp/$(AL_DIRECTORY)/%.o,$(SRC_PLATFORM))

INTERNAL_INC_INDEPENDENT   := src_independent internal_interfaces
INTERNAL_INC_INDEPENDENT   += $(sort $(dir $(wildcard interfaces/extensions/*/))) $(sort $(dir $(wildcard  src_independent/extensions/*/)))
INTERNAL_INC_PLATFORM      := src_$(PLATFORM) internal_interfaces

EXTERNAL_INC               := $(COMMON_INC) $(FACTORY_INC)

HDR_INDEPENDENT            := $(shell find $(INTERNAL_INC_INDEPENDENT) $(EXTERNAL_INC) -name *.h)
HDR_PLATFORM               := $(shell find $(INTERNAL_INC_PLATFORM)    $(EXTERNAL_INC) -name *.h)


################################################################################
# Targets
################################################################################

.PHONY: all
all: $(EXE)


$(EXE):  $(OBJ_INDEPENDENT) $(OBJ_PLATFORM) $(FACTORY_LIB) $(COMMON_LIB)
	$(CC) $^ $(LDFLAGS) -o $@

$(OBJ_INDEPENDENT) : $(OUTPUT_FOLDER)/tmp/$(AL_DIRECTORY)/%.o : %.c $(HDR_INDEPENDENT)
	$(foreach directory, $(sort $(dir $(wildcard $(SRC_INDEPENDENT)))), $(MKDIR) $(OUTPUT_FOLDER)/tmp/$(AL_DIRECTORY)/$(directory);)
	$(CC) $(CCFLAGS) -c $(addprefix -I,$(INTERNAL_INC_INDEPENDENT) $(EXTERNAL_INC)) $< -o $@

$(OBJ_PLATFORM)    : $(OUTPUT_FOLDER)/tmp/$(AL_DIRECTORY)/%.o : %.c $(HDR_PLATFORM)
	$(foreach directory, $(sort $(dir $(wildcard $(SRC_PLATFORM)))), $(MKDIR) $(OUTPUT_FOLDER)/tmp/$(AL_DIRECTORY)/$(directory);)
	$(CC) $(CCFLAGS) -c $(addprefix -I,$(INTERNAL_INC_PLATFORM) $(EXTERNAL_INC)) $< -o $@


.PHONY: clean
clean:
	rm -rf $(EXE)
	rm -rf $(OUTPUT_FOLDER)/tmp/$(AL_DIRECTORY)
