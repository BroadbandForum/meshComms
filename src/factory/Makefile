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
#   OUTPUT_FOLDER -> Absolute path to the folder where binaries will be built
#
#   COMMON_INC ----> Absolute path to the folder containing common (to all
#                    sub-projects) header files
#
#   FACTORY_LIB ---> Absolute path to the "factory library" *.a file
#
#   MKDIR ---------> Tool to create a directory
#

FACTORY_DIRECTORY := factory

LIB               := $(FACTORY_LIB)
SRC               := $(wildcard src_independent/*.c)
SRC               += $(wildcard src_independent/extensions/*/*.c)
OBJ               := $(patsubst %.c,$(OUTPUT_FOLDER)/tmp/$(FACTORY_DIRECTORY)/%.o,$(SRC))

INTERNAL_INC      := src_independent interfaces
INTERNAL_INC      += $(sort $(dir $(wildcard interfaces/extensions/*/)))
EXTERNAL_INC      := $(COMMON_INC)

HDR               := $(shell find $(INTERNAL_INC) $(EXTERNAL_INC) -name *.h)


################################################################################
# Targets
################################################################################

.PHONY: all
all: $(LIB)


$(LIB) : $(OBJ)
	$(AR) -rc $@ $^


$(OBJ) : $(OUTPUT_FOLDER)/tmp/$(FACTORY_DIRECTORY)/%.o : %.c $(HDR)
	$(foreach directory, $(sort $(dir $(wildcard $(SRC)))), $(MKDIR) $(OUTPUT_FOLDER)/tmp/$(FACTORY_DIRECTORY)/$(directory);)
	$(CC) $(CCFLAGS) -c $(addprefix -I,$(INTERNAL_INC) $(EXTERNAL_INC)) $< -o $@


.PHONY: unit_tests
unit_tests:
	$(MAKE) -C unit_tests all


.PHONY: clean
clean:
	rm -rf $(LIB)
	rm -rf $(OUTPUT_FOLDER)/tmp/$(FACTORY_DIRECTORY)
	$(MAKE) -C unit_tests clean

