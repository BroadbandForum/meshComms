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

