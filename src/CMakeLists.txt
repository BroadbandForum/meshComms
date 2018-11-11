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

add_library(${libname} STATIC
    1905_alme.c
    1905_cmdus.c
    1905_tlvs.c
    al_datamodel.c
    al_entity.c
    al_extension.c
    al_extension_register.c
    al_recv.c
    al_send.c
    al_utils.c
    al_wsc.c
    bbf_recv.c
    bbf_send.c
    bbf_tlvs.c
    datamodel.c
    hlist.c
    lldp_payload.c
    lldp_tlvs.c
    mac_address.c
    media_specific_blobs.c
    tlv.c
    utils.c)

install(TARGETS ${libname} DESTINATION lib COMPONENT Devel)

if (${CMAKE_SYSTEM_NAME} MATCHES Linux)

    find_package(OpenSSL REQUIRED)
    find_package(Threads REQUIRED)

    find_package(PkgConfig)
    pkg_check_modules(NL3 libnl-3.0 libnl-genl-3.0 REQUIRED)

    find_library(LIBRT rt)

    target_include_directories(${libname} PRIVATE ${OPENSSL_INCLUDE_DIR} ${NL3_INCLUDE_DIRS})
    set_property(TARGET ${libname} APPEND PROPERTY COMPILE_OPTIONS ${NL3_CFLAGS_OTHER})

    target_sources(${libname} PRIVATE
         linux/netlink_collect.c
         linux/netlink_socks.c
         linux/netlink_utils.c
         linux/platform.c
         linux/platform_alme_server.c
         linux/platform_crypto.c
         linux/platform_interfaces.c
         # @todo make these configurable
         linux/platform_uci.c
         linux/platform_interfaces_ghnspirit.c
         linux/platform_interfaces_simulated.c
         linux/platform_interfaces_openwrt.c
         linux/platform_os.c)

    add_executable(al_entity linux/al_entity/al_entity_main.c)
    target_link_libraries(al_entity ${libname} OpenSSL::Crypto Threads::Threads ${NL3_LIBRARIES})
    if (LIBRT)
        target_link_libraries(al_entity rt)
    endif (LIBRT)
    install(TARGETS al_entity DESTINATION bin)

    add_executable(hl_entity linux/hl_entity/hl_entity_main.c)
    target_link_libraries(hl_entity ${libname} OpenSSL::Crypto Threads::Threads)
    install(TARGETS hl_entity DESTINATION bin)

endif (${CMAKE_SYSTEM_NAME} MATCHES Linux)

