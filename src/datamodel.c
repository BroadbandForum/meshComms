/*
 *  prplMesh Wi-Fi Multi-AP
 *
 *  Copyright (c) 2018, prpl Foundation
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 */

#include <datamodel.h>
#include <platform.h>

#include <assert.h>
#include <string.h> // memcpy

#define EMPTY_MAC_ADDRESS {0, 0, 0, 0, 0, 0}

struct alDevice *local_device = NULL;

struct registrar registrar = {
    .d = NULL,
    .is_map = false,
    .wsc = {&registrar.wsc, &registrar.wsc},
};

DEFINE_DLIST_HEAD(network);

void datamodelInit(void)
{
}

/* 'alDevice' related functions
 */
struct alDevice *alDeviceAlloc(const mac_address al_mac_addr)
{
    struct alDevice *ret = zmemalloc(sizeof(struct alDevice));
    dlist_add_tail(&network, &ret->l);
    memcpy(ret->al_mac_addr, al_mac_addr, sizeof(mac_address));
    dlist_head_init(&ret->interfaces);
    dlist_head_init(&ret->radios);
    ret->is_map_agent = false;
    ret->is_map_controller = false;
    return ret;
}

void alDeviceDelete(struct alDevice *alDevice)
{
    while (!dlist_empty(&alDevice->interfaces)) {
        struct interface *interface = container_of(dlist_get_first(&alDevice->interfaces), struct interface, l);
        interfaceDelete(interface);
    }
    while (!dlist_empty(&alDevice->radios)) {
        struct radio *radio = container_of(dlist_get_first(&alDevice->radios), struct radio, l);
        radioDelete(radio);
    }
    free(alDevice);
}

/* 'radio' related functions
 */
struct radio*   radioAlloc(struct alDevice *dev, const mac_address mac)
{
    struct radio *r = zmemalloc(sizeof(struct radio));
    memcpy(&r->uid, &mac, sizeof(mac_address));
    r->index = -1;
    dlist_add_tail(&dev->radios, &r->l);
    return r;
}

struct radio*   radioAllocLocal(const mac_address mac, const char *name, int index)
{
    struct radio *r = radioAlloc(local_device, mac);
    strcpy(r->name, name);
    r->index = index;
    return r;
}

void    radioDelete(struct radio *radio)
{
    unsigned i;
    dlist_remove(&radio->l);
    for (i = 0; i < radio->configured_bsses.length; i++)
    {
        /* The interfaceWifi is deleted automatically when we delete the interface itself. */
        interfaceDelete(&radio->configured_bsses.data[i]->i);
    }
    PTRARRAY_CLEAR(radio->configured_bsses);
    for ( i=0 ; i < radio->bands.length ; i++ ) {
        PTRARRAY_CLEAR(radio->bands.data[i]->channels);
        free(radio->bands.data[i]);
    }
    PTRARRAY_CLEAR(radio->bands);
    free(radio);
}

struct radio *findDeviceRadio(const struct alDevice *device, const mac_address uid)
{
    struct radio *radio;
    dlist_for_each(radio, device->radios, l)
    {
        if (memcmp(radio->uid, uid, 6) == 0)
        {
            return radio;
        }
    }
    return NULL;
}

struct radio *findLocalRadio(const char *name)
{
    struct radio *radio;
    dlist_for_each(radio, local_device->radios, l)
    {
        if (strncmp(radio->name, name, sizeof(radio->name)) == 0)
        {
            return radio;
        }
    }
    return NULL;
}

int radioAddInterfaceWifi(struct radio *radio, struct interfaceWifi *ifw)
{
    PTRARRAY_ADD(radio->configured_bsses, ifw);
    ifw->radio = radio;
    return 0;
}

void radioAddAp(struct radio *radio, struct bssInfo bssInfo)
{
    if (radio->addAP == NULL)
    {
        PLATFORM_PRINTF_DEBUG_WARNING("No addAP callback for radio " MACSTR " to be configured with ssid %.*s\n",
                                      MAC2STR(radio->uid), bssInfo.ssid.length, bssInfo.ssid.ssid);
        return;
    }
    radio->addAP(radio, bssInfo);
}


/* 'interface' related functions
 */
static struct interface * interfaceInit(struct interface *i, const mac_address addr, struct alDevice *owner)
{
    i->type = interface_type_unknown;
    memcpy(i->addr, addr, 6);
    if (owner != NULL) {
        alDeviceAddInterface(owner, i);
    }
    return i;
}

struct interface *interfaceAlloc(const mac_address addr, struct alDevice *owner)
{
    return interfaceInit(zmemalloc(sizeof(struct interface)), addr, owner);
}

void interfaceDelete(struct interface *interface)
{
    unsigned i;
    for (i = 0; i < interface->neighbors.length; i++)
    {
        interfaceRemoveNeighbor(interface, interface->neighbors.data[i]);
    }
    /* Even if the interface doesn't have an owner, removing it from the empty list doesn't hurt. */
    dlist_remove(&interface->l);
    free(interface);
}

void interfaceAddNeighbor(struct interface *interface, struct interface *neighbor)
{
    PTRARRAY_ADD(interface->neighbors, neighbor);
    PTRARRAY_ADD(neighbor->neighbors, interface);
}

void interfaceRemoveNeighbor(struct interface *interface, struct interface *neighbor)
{
    PTRARRAY_REMOVE_ELEMENT(interface->neighbors, neighbor);
    PTRARRAY_REMOVE_ELEMENT(neighbor->neighbors, interface);
    if (neighbor->owner == NULL && neighbor->neighbors.length == 0)
    {
        /* No more references to the neighbor interface. */
        free(neighbor);
    }
}

/* 'interfaceWifi' related functions
 */
struct interfaceWifi *interfaceWifiAlloc(const mac_address addr, struct alDevice *owner)
{
    struct interfaceWifi *ifw = zmemalloc(sizeof(*ifw));
    interfaceInit(&ifw->i, addr, owner);
    ifw->i.type = interface_type_wifi;
    return ifw;
}

void    interfaceWifiRemove(struct interfaceWifi *ifw)
{
    PTRARRAY_REMOVE_ELEMENT(ifw->radio->configured_bsses, ifw);
    /* Clients don't need to be deleted; they are also in the interface neighbour list, so they will be deleted or unlinked
     * together with the interface. */
    interfaceDelete(&ifw->i); /* This also frees interfaceWifi itself. */
}

/* 'alDevice' related functions
 */
void alDeviceAddInterface(struct alDevice *device, struct interface *interface)
{
    assert(interface->owner == NULL);
    dlist_add_tail(&device->interfaces, &interface->l);
    interface->owner = device;
}

struct alDevice *alDeviceFind(const mac_address al_mac_addr)
{
    struct alDevice *ret;
    dlist_for_each(ret, network, l)
    {
        if (memcmp(ret->al_mac_addr, al_mac_addr, 6) == 0)
            return ret;
    }
    return NULL;
}

struct alDevice *alDeviceFindFromAnyAddress(const mac_address sender_addr)
{
    struct alDevice *sender_device = alDeviceFind(sender_addr);
    if (sender_device == NULL)
    {
        struct interface *sender_interface = findDeviceInterface(sender_addr);
        if (sender_interface != NULL)
        {
            sender_device = sender_interface->owner;
        }
    }
    return sender_device;
}

struct interface *alDeviceFindInterface(const struct alDevice *device, const mac_address addr)
{
    struct interface *ret;
    dlist_for_each(ret, device->interfaces, l)
    {
        if (memcmp(ret->addr, addr, 6) == 0)
        {
            return ret;
        }
    }
    return NULL;
}

struct interface *findDeviceInterface(const mac_address addr)
{
    struct alDevice *alDevice;
    struct interface *ret = NULL;

    dlist_for_each(alDevice, network, l)
    {
        ret = alDeviceFindInterface(alDevice, addr);
        if (ret != NULL)
        {
            return ret;
        }
    }
    return NULL;
}

struct interface *findLocalInterface(const char *name)
{
    struct interface *ret;
    if (local_device == NULL)
    {
        return NULL;
    }
    dlist_for_each(ret, local_device->interfaces, l)
    {
        if (ret->name && strcmp(ret->name, name) == 0)
        {
            return ret;
        }
    }
    return NULL;
}

void registrarAddWsc(struct wscRegistrarInfo *wsc)
{
    dlist_add_head(&registrar.wsc, &wsc->l);
}
