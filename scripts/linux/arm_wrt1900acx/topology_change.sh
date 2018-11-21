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

