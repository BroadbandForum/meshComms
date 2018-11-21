# Broadband Forum IEEE 1905.1/1a stack
# 
# Copyright (c) 2017, Broadband Forum
# 
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are
# met:
# 
# 1. Redistributions of source code must retain the above copyright
#    notice, this list of conditions and the following disclaimer.
# 
# 2. Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in the
#    documentation and/or other materials provided with the distribution.
# 
# Subject to the terms and conditions of this license, each copyright
# holder and contributor hereby grants to those receiving rights under
# this license a perpetual, worldwide, non-exclusive, no-charge,
# royalty-free, irrevocable (except for failure to satisfy the
# conditions of this license) patent license to make, have made, use,
# offer to sell, sell, import, and otherwise transfer this software,
# where such license applies only to those patent claims, already
# acquired or hereafter acquired, licensable by such copyright holder or
# contributor that are necessarily infringed by:
# 
# (a) their Contribution(s) (the licensed copyrights of copyright holders
#     and non-copyrightable additions of contributors, in source or binary
#     form) alone; or
# 
# (b) combination of their Contribution(s) with the work of authorship to
#     which such Contribution(s) was added by such copyright holder or
#     contributor, if, at the time the Contribution is added, such addition
#     causes such combination to be necessarily infringed. The patent
#     license shall not apply to any other combinations which include the
#     Contribution.
# 
# Except as expressly stated above, no rights or licenses from any
# copyright holder or contributor is granted under this license, whether
# expressly, by implication, estoppel or otherwise.
# 
# DISCLAIMER
# 
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
# IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
# TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
# PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
# HOLDERS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
# INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
# BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
# OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
# ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR
# TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE
# USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
# DAMAGE.

##############################################################################
#
# Description: This script is an example of how to start the 1905 stack in a 
#              Linksys 1900 AC device 
#
#              The following binaries/scripts must be in the same directory :
#                   - start_linksys.sh
#                   - topology_change.sh
#                   - al_entity
#                   - configlayer
#                    
#              This script will :
#                   - configure the WIFI radio 1 with default configuration
#                   - configure the ebtables to drop 1905 multicast MACs
#                   - start the topology_change.sh script to monitor topology
#                     changes and inform 1905 stack
#                   - start the 1905 stack with the default configuration
#
#              This script must be run with the following command :
#                   - ./start_linksys.sh
#
#                
###############################################################################

AL_MAC=00:50:43:22:22:22
GHN_INTERFACE_MAC=00139D00114C
GHN_INTERFACE=eth0
DEFAULT_DOMAIN_NAME=Demo1905_2
DEFAULT_WIFI_SSID=Marvell1905_2

PATH=$PATH:.

#Leave secure mode
./configlayer -i $GHN_INTERFACE -m $GHN_INTERFACE_MAC -o SETLEGACY -p PAIRING.GENERAL.LEAVE_SECURE_DOMAIN=yes -w paterna

#Default WIFI configuration
uci set wireless.@wifi-iface[1].ssid=$DEFAULT_WIFI_SSID
uci set wireless.@wifi-iface[1].encryption='psk2'
uci set wireless.@wifi-iface[1].key='12345678'
wifi
sleep 5

#Default GHN configuration
./configlayer -i $GHN_INTERFACE -m $GHN_INTERFACE_MAC -o SETLEGACY -p NODE.GENERAL.DOMAIN_NAME=$DEFAULT_DOMAIN_NAME -w paterna

#Avoid duplicate 1905 multicast messages because of bridge
ebtables -A FORWARD  -d 01:80:c2:00:00:13 -j DROP 

#Kill previous topology_change process if exsit
process_id=`ps | grep topology_change | grep exe | awk '{print $1}'`
if [ $? -eq "0" ]; then
   kill -9 $process_id
fi

#Monitor topology changes
./topology_change.sh $GHN_INTERFACE_MAC > /dev/null &

#Start 1905 entity
echo ./al_entity -m $AL_MAC -i eth0:ghnspirit:$GHN_INTERFACE_MAC:paterna,eth1,wlan1 -v
./al_entity -m $AL_MAC -i eth0:ghnspirit:$GHN_INTERFACE_MAC:paterna,eth1,wlan1 -v

