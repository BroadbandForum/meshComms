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

cmake_minimum_required(VERSION 3.4)
project(prplMesh LANGUAGES C VERSION 0.9)

set_property(DIRECTORY APPEND PROPERTY COMPILE_DEFINITIONS _BUILD_NUMBER_=\"${prplMesh_VERSION}\")

INCLUDE(TestBigEndian)
TEST_BIG_ENDIAN(BIGENDIAN)
if(BIGENDIAN)
    set_property(DIRECTORY APPEND PROPERTY COMPILE_DEFINITIONS _HOST_IS_BIG_ENDIAN_=1)
else()
    set_property(DIRECTORY APPEND PROPERTY COMPILE_DEFINITIONS _HOST_IS_LITTLE_ENDIAN_=1)
endif(BIGENDIAN)

# @todo make these configurable
set_property(DIRECTORY APPEND PROPERTY COMPILE_DEFINITIONS
    MAX_NETWORK_SEGMENT_SIZE=1500
    SEND_EMPTY_TLVS
    FIX_BROKEN_TLVS
    SPEED_UP_DISCOVERY
    REGISTER_EXTENSION_BBF
    )


include_directories(src/common/interfaces)
# Wildcards are frowned upon, but they make our life so much easier :-)
file (GLOB commonSrcs src/common/src_independent/*.c)
if (${CMAKE_SYSTEM_NAME} MATCHES Linux)
    file (GLOB commonPlatformSrcs src/common/src_linux/*.c)
endif (${CMAKE_SYSTEM_NAME} MATCHES Linux)
add_library(common STATIC ${commonSrcs} ${commonPlatformSrcs})

include_directories(src/factory/interfaces)
include_directories(src/factory/interfaces/extensions/bbf)
file (GLOB factorySrcs src/factory/src_independent/*.c)
file (GLOB factoryBbfSrcs src/factory/src_independent/extensions/bbf/*.c)
add_library(factory STATIC ${factorySrcs} ${factoryBbfSrcs})

include_directories(src/al/internal_interfaces)
include_directories(src/al/src_independent)
include_directories(src/al/src_independent/extensions/bbf)
file (GLOB alSrcs src/al/src_independent/*.c)
file (GLOB alBbfSrcs src/al/src_independent/extensions/bbf/*.c)
add_library(al STATIC ${alSrcs} ${alBbfSrcs})

if (${CMAKE_SYSTEM_NAME} MATCHES Linux)
    find_package(OpenSSL REQUIRED)
    find_package(Threads REQUIRED)
    file (GLOB alExeSrcs src/al/src_linux/*.c)
    add_executable(al_entity ${alExeSrcs})
    target_link_libraries(al_entity al factory common OpenSSL::Crypto Threads::Threads)

    find_library(LIBRT rt)
    if (LIBRT)
        target_link_libraries(al_entity rt)
    endif (LIBRT)

    file (GLOB hleExeSrcs src/hle/src_linux/*.c)
    add_executable(hle_entity ${hleExeSrcs})
    target_link_libraries(hle_entity factory common OpenSSL::Crypto Threads::Threads)

    enable_testing()
    macro(unittest)
        get_filename_component(testname ${ARGV0} NAME_WE)
        add_executable(UNITTEST_${testname} ${ARGV})
        target_link_libraries(UNITTEST_${testname} factory common)
        add_test(NAME ${testname} COMMAND UNITTEST_${testname})
    endmacro(unittest)

    unittest(src/factory/unit_tests/hlist_test.c)
    unittest(src/common/unit_tests/dlist_test.c)
    unittest(src/common/unit_tests/ptrarray_test.c)

    foreach(factory_unit_test 1905_alme 1905_cmdu 1905_tlv lldp_payload lldp_tlv extensions/bbf/bbf_tlv)
        unittest(
            src/factory/unit_tests/${factory_unit_test}_parsing.c
            src/factory/unit_tests/${factory_unit_test}_test_vectors.c)
        unittest(
            src/factory/unit_tests/${factory_unit_test}_forging.c
            src/factory/unit_tests/${factory_unit_test}_test_vectors.c)
    endforeach(factory_unit_test)

    macro(aletest testname)
        add_executable(ALETEST_${testname} src/al/ale_tests/${testname}.c src/al/ale_tests/aletest.c)
        target_link_libraries(ALETEST_${testname} al factory common)
        add_test(NAME ${testname}
            COMMAND ./start_interfaces $<TARGET_FILE:al_entity> $<TARGET_FILE:ALETEST_${testname}>
            WORKING_DIRECTORY ${prplMesh_SOURCE_DIR}/src/al/ale_tests)
    endmacro(aletest)

    aletest(ap_onboarding_controller)
    aletest(topology_discovery)
endif (${CMAKE_SYSTEM_NAME} MATCHES Linux)
