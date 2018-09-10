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
#   COMMON_LIB ----> Absolute path to the "common library" *.a file
#   COMMON_INC ----> Absolute path to the folder containing common (to all
#                    sub-projects) header files

#   FACTORY_LIB   -> Absolute path to the "factory library" *.a file
#   FACTORY_INC   -> Absolute path to the folder containing the "factory
#                    library" header files
#
#   MKDIR ---------> Tool to create a directory
#
UNIT_TESTS_DIRECTORY := factory/unit_tests

UNITS := hlist_test
UNITS += 1905_tlv_forging
UNITS += 1905_tlv_parsing
UNITS += 1905_cmdu_forging
UNITS += 1905_cmdu_parsing
UNITS += 1905_alme_forging
UNITS += 1905_alme_parsing
UNITS += lldp_tlv_parsing
UNITS += lldp_payload_forging
UNITS += lldp_payload_parsing

UNITS += extensions/bbf/bbf_tlv_forging
UNITS += extensions/bbf/bbf_tlv_parsing

EXE  := $(addprefix $(OUTPUT_FOLDER)/UNITTEST_, $(UNITS))
SRC  := $(addsuffix .c, $(UNITS))
OBJ  := $(addprefix $(OUTPUT_FOLDER)/tmp/$(UNIT_TESTS_DIRECTORY)/, $(addsuffix .o, $(UNITS)))


INTERNAL_INC := . $(sort $(dir $(wildcard extensions/*/)))
EXTERNAL_INC := $(COMMON_INC) $(FACTORY_INC)

HDR := $(shell find $(INTERNAL_INC) $(EXTERNAL_INC) -name "*.h")

SHARED_SRC := $(wildcard *test_vectors.c)
SHARED_SRC += $(wildcard extensions/*/*test_vectors.c)
SHARED_OBJ := $(addprefix $(OUTPUT_FOLDER)/tmp/$(UNIT_TESTS_DIRECTORY)/,$(patsubst %.c,%.o,$(SHARED_SRC)))
SHARED_HDR := $(patsubst %.c,%.h,$(SHARED_SRC))

#LDFLAGS += -fdata-sections -ffunction-sections -Wl,--gc-sections

TESTS      := $(addprefix UNITTEST_,$(basename $(UNITS)))

################################################################################
# Targets
################################################################################

.PHONY: all
all: $(TESTS)


.PHONY: $(TESTS)
$(TESTS) : UNITTEST_% : $(OUTPUT_FOLDER)/UNITTEST_%
	$<


$(EXE) : $(OUTPUT_FOLDER)/UNITTEST_% : $(OUTPUT_FOLDER)/tmp/$(UNIT_TESTS_DIRECTORY)/%.o $(SHARED_OBJ) $(FACTORY_LIB) $(COMMON_LIB)
	$(foreach directory, $(UNITS), $(MKDIR) $(OUTPUT_FOLDER)/UNITTEST_$(shell dirname $(directory);))
	$(CC) $(LDFLAGS) $^ -o $@

$(OBJ) : $(OUTPUT_FOLDER)/tmp/$(UNIT_TESTS_DIRECTORY)/%.o : %.c
	$(foreach directory, $(sort $(dir $(wildcard $(SRC)))), $(MKDIR) $(OUTPUT_FOLDER)/tmp/$(UNIT_TESTS_DIRECTORY)/$(directory);)
	$(CC)  $(CCFLAGS) -c $(addprefix -I,$(INTERNAL_INC) $(EXTERNAL_INC)) $< -o $@;


$(SHARED_OBJ) : $(OUTPUT_FOLDER)/tmp/$(UNIT_TESTS_DIRECTORY)/%.o : %.c $(SHARED_HDR)
	$(foreach directory, $(sort $(dir $(wildcard $(SHARED_SRC)))), $(MKDIR) $(OUTPUT_FOLDER)/tmp/$(UNIT_TESTS_DIRECTORY)/$(directory);)
	$(CC)  $(CCFLAGS) -c $(addprefix -I,$(INTERNAL_INC) $(EXTERNAL_INC)) $< -o $@;


.PHONY: clean
clean:
	rm -f $(EXE)
	rm -rf $(OUTPUT_FOLDER)/UNITTEST_*
	rm -rf $(OUTPUT_FOLDER)/tmp/UNITTEST_*
	rm -rf $(OUTPUT_FOLDER)/tmp/$(UNIT_TESTS_DIRECTORY)
