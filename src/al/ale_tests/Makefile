# prplMesh Wi-Fi Multi-AP
#
# Copyright (c) 2017, Broadband Forum
# Copyright (c) 2018, prpl Foundation
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
#   LDFLAGS -------> Extra flags to use while linking
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

ALETEST_DIRECTORY               := al/test

UNITS := topology_discovery

EXE  := $(addprefix $(OUTPUT_FOLDER)/ALETEST_, $(UNITS))
SRC  := $(addsuffix .c, $(UNITS))
OBJ  := $(addprefix $(OUTPUT_FOLDER)/tmp/$(ALETEST_DIRECTORY)/, $(addsuffix .o, $(UNITS)))

INTERNAL_INC := .
EXTERNAL_INC := $(COMMON_INC) $(FACTORY_INC)

HDR := $(shell find $(INTERNAL_INC) $(EXTERNAL_INC) -name "*.h")

SHARED_SRC := aletest.c
SHARED_OBJ := $(addprefix $(OUTPUT_FOLDER)/tmp/$(ALETEST_DIRECTORY)/,$(patsubst %.c,%.o,$(SHARED_SRC)))
SHARED_HDR := $(patsubst %.c,%.h,$(SHARED_SRC))

#LDFLAGS += -fdata-sections -ffunction-sections -Wl,--gc-sections

TESTS      := $(addprefix ALETEST_,$(basename $(UNITS)))

################################################################################
# Targets
################################################################################

.PHONY: all
all: $(TESTS)


.PHONY: $(TESTS)
$(TESTS) : ALETEST_% : $(OUTPUT_FOLDER)/ALETEST_%
	./start_interfaces $(AL_EXE) $<


$(EXE) : $(OUTPUT_FOLDER)/ALETEST_% : $(OUTPUT_FOLDER)/tmp/$(ALETEST_DIRECTORY)/%.o $(SHARED_OBJ) $(COMMON_LIB) $(FACTORY_LIB)
	$(foreach directory, $(UNITS), $(MKDIR) $(OUTPUT_FOLDER)/ALETEST_$(shell dirname $(directory);))
	$(CC) $(LDFLAGS) $^ -o $@

$(OBJ) : $(OUTPUT_FOLDER)/tmp/$(ALETEST_DIRECTORY)/%.o : %.c $(HDR)
	$(foreach directory, $(sort $(dir $(wildcard $(SRC)))), $(MKDIR) $(OUTPUT_FOLDER)/tmp/$(ALETEST_DIRECTORY)/$(directory);)
	$(CC)  $(CCFLAGS) -c $(addprefix -I,$(INTERNAL_INC) $(EXTERNAL_INC)) $< -o $@;


$(SHARED_OBJ) : $(OUTPUT_FOLDER)/tmp/$(ALETEST_DIRECTORY)/%.o : %.c $(SHARED_HDR)
	$(foreach directory, $(sort $(dir $(wildcard $(SHARED_SRC)))), $(MKDIR) $(OUTPUT_FOLDER)/tmp/$(ALETEST_DIRECTORY)/$(directory);)
	$(CC)  $(CCFLAGS) -c $(addprefix -I,$(INTERNAL_INC) $(EXTERNAL_INC)) $< -o $@;


.PHONY: clean
clean:
	rm -f $(EXE)
	rm -rf $(OUTPUT_FOLDER)/ALETEST_*
	rm -rf $(OUTPUT_FOLDER)/tmp/ALETEST_*
	rm -rf $(OUTPUT_FOLDER)/tmp/$(ALETEST_DIRECTORY)
