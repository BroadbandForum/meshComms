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
# Description: This script is in charge of monitoring network topology changes
#              and inform the 1905 stack that a new device has been connected
#              or a device has been disconnected.
#
#              This script is automatically started by the 'start_linksys.sh'
#              script.
#
#              In this example, the script only monitors GHN and WIFI
#              interfaces. If something occurs in these two interfaces,
#              a "touch" will be done to the /tmp/topology_change file to
#              inform the 1905 stack that the topology must be refreshed.
#
###############################################################################

GHN_INTERFACE_MAC=$1
GHN_INTERFACE=eth0
WIFI_INTERFACE=wlan1

wifi_device_list_old=""
ghn_device_list_old=""
while true;
do
  echo "Refreshing device list...."
  wifi_device_list=`iw dev $WIFI_INTERFACE station dump | grep Station | cut -f2 -d' '`
  ghn_device_list=`./configlayer -i $GHN_INTERFACE -m $GHN_INTERFACE_MAC -o GETLEGACY -p DIDMNG.GENERAL.MACS -w paterna`
  if [ "$wifi_device_list" != "$wifi_device_list_old" ] || [ "$ghn_device_list" != "$ghn_device_list_old" ]
  then
      echo "Topology has changed"
      touch /tmp/topology_change
      echo "Old list :"
      echo $wifi_device_list_old
      echo $ghn_device_list_old
      echo "New list :"
      echo $wifi_device_list
      echo $ghn_device_list
      wifi_device_list_old=$wifi_device_list
      ghn_device_list_old=$ghn_device_list
  fi
  sleep 5
done

