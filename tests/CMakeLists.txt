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

# Note that this directory is only included under Linux, so no need to
# check for it.

macro(unittest)
    get_filename_component(testname ${ARGV0} NAME_WE)
    add_executable(UNITTEST_${testname} ${ARGV})
    target_link_libraries(UNITTEST_${testname} prplMesh)
    add_test(NAME ${testname} COMMAND UNITTEST_${testname})
endmacro(unittest)

unittest(hlist_test.c)
unittest(dlist_test.c)
unittest(ptrarray_test.c)

foreach(factory_unit_test 1905_alme 1905_cmdu 1905_tlv lldp_payload lldp_tlv bbf_tlv)
    unittest(
        ${factory_unit_test}_parsing.c
        ${factory_unit_test}_test_vectors.c)
    unittest(
        ${factory_unit_test}_forging.c
        ${factory_unit_test}_test_vectors.c)
endforeach(factory_unit_test)

macro(aletest testname)
    add_executable(ALETEST_${testname} ${testname}.c aletest.c)
    target_link_libraries(ALETEST_${testname} prplMesh)
    add_test(NAME ${testname}
             COMMAND ./start_interfaces $<TARGET_FILE:al_entity> $<TARGET_FILE:ALETEST_${testname}>
             WORKING_DIRECTORY ${prplMesh_SOURCE_DIR}/tests)
endmacro(aletest)

aletest(ap_onboarding_controller)
aletest(topology_discovery)

