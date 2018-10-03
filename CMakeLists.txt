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

include(TestBigEndian)
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


include_directories(include)

set(prplMesh_HEADERS
    1905_alme.h
    1905_cmdus.h
    1905_l2.h
    1905_tlvs.h
    bbf_tlvs.h
    datamodel.h
    dlist.h
    hlist.h
    lldp_payload.h
    lldp_tlvs.h
    media_specific_blobs.h
    platform.h
    ptrarray.h
    tlv.h
    utils.h
    )

add_subdirectory(src)

if (${CMAKE_SYSTEM_NAME} MATCHES Linux)
    enable_testing()
    add_subdirectory(tests)
endif (${CMAKE_SYSTEM_NAME} MATCHES Linux)
