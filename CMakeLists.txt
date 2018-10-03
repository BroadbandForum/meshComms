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

set(libname "prplMesh" CACHE STRING
    "Name of the generated library. Includes are installed in a subdirectory of the same name.\
     This allows to install it as e.g. 'lib1905.a' if only 1905 functionality is needed.")
set(INCLUDE_INSTALL_DIR include/ CACHE PATH
    "Installation directory for includes (relative to CMAKE_INSTALL_PREFIX)")
set(LIB_INSTALL_DIR lib/ CACHE PATH
    "Installation directory for the library (relative to CMAKE_INSTALL_PREFIX)")
set(CMAKE_INSTALL_DIR ${LIB_INSTALL_DIR}/cmake CACHE PATH
    "Installation directory for CMake files (relative to CMAKE_INSTALL_PREFIX)")

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

add_subdirectory(src)

if (${CMAKE_SYSTEM_NAME} MATCHES Linux)
    enable_testing()
    add_subdirectory(tests)
endif (${CMAKE_SYSTEM_NAME} MATCHES Linux)


# Installation

include(CMakePackageConfigHelpers)

install(DIRECTORY include/ DESTINATION ${INCLUDE_INSTALL_DIR}/${libname} COMPONENT Devel FILES_MATCHING PATTERN "*.h")

configure_package_config_file(prplMeshConfig.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/${libname}Config.cmake
    INSTALL_DESTINATION ${CMAKE_INSTALL_DIR}
    NO_CHECK_REQUIRED_COMPONENTS_MACRO
    PATH_VARS INCLUDE_INSTALL_DIR LIB_INSTALL_DIR)
write_basic_package_version_file(
  "${CMAKE_CURRENT_BINARY_DIR}/${libname}ConfigVersion.cmake"
  VERSION ${prplMesh_VERSION}
  COMPATIBILITY AnyNewerVersion)
configure_file(prplMesh.pc.in
    ${CMAKE_CURRENT_BINARY_DIR}/${libname}.pc
    @ONLY)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/${libname}Config.cmake
              ${CMAKE_CURRENT_BINARY_DIR}/${libname}ConfigVersion.cmake
              ${CMAKE_CURRENT_BINARY_DIR}/${libname}.pc
        DESTINATION ${CMAKE_INSTALL_DIR}
        COMPONENT Devel)
