/*
 *  Broadband Forum IEEE 1905.1/1a stack
 *
 *  Copyright (c) 2017, Broadband Forum
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

#include "platform.h"

#include "lldp_payload.h"
#include "lldp_tlvs.h"
#include "packet_tools.h"


////////////////////////////////////////////////////////////////////////////////
// Actual API functions
////////////////////////////////////////////////////////////////////////////////

struct PAYLOAD *parse_lldp_PAYLOAD_from_packet(INT8U *packet_stream)
{
    struct PAYLOAD *ret;

    INT8U *p;
    INT8U i, j;

    if (NULL == packet_stream)
    {
        return NULL;
    }

    ret = (struct PAYLOAD *)PLATFORM_MALLOC(sizeof(struct PAYLOAD));
    for (i=0; i<MAX_LLDP_TLVS; i++)
    {
        ret->list_of_TLVs[i] = NULL;
    }

    p = packet_stream;
    i = 0;

    while (1)
    {
        INT8U *tlv;

        INT8U byte1, byte2;
        INT16U len;

        tlv = parse_lldp_TLV_from_packet(p);

        if (NULL == tlv || MAX_LLDP_TLVS == i)
        {
            // Parsing error or too many TLVs
            //
            for (j=0; j<i; j++)
            {
                free_lldp_TLV_structure(ret->list_of_TLVs[j]);
            }
            PLATFORM_FREE(ret);
            return NULL;
        }

        // The first byte of the TLV structure always contains the TLV type.
        // We need to check if we have reach the "end of LLPDPDU" TLV (ie. the
        // last one)
        //
        if (TLV_TYPE_END_OF_LLDPPDU == *tlv)
        {
            free_lldp_TLV_structure(tlv);
            break;
        }
        else
        {
            ret->list_of_TLVs[i] = tlv;
        }

        // All LLDP TLVs start with the same two bytes:
        //
        //   |byte #1 |byte #2 |
        //   |--------|--------|
        //   |TTTTTTTL|LLLLLLLL|
        //   |--------|--------|
        //    <-----><-------->
        //    7 bits   9 bits
        //    (type)   (lenght)
        //
        // We are interested in the length to find out how much we should
        // "advance" the stream pointer each time

        _E1B(&p, &byte1);
        _E1B(&p, &byte2);

        len  = ((byte1 & 0x1) << 8) + byte2;

        p += len;
        i++;
    }

    // Before returning, we must make sure that this packet contained all the
    // needed TLVs (ie. "chassis ID", "port ID" and "time to live")
    //
    {
        INT8U chassis_id    = 0;
        INT8U port_id       = 0;
        INT8U time_to_live  = 0;

        for (j=0; j<i; j++)
        {
            if      (TLV_TYPE_CHASSIS_ID    == *(ret->list_of_TLVs[j]))
            {
                chassis_id++;
            }
            else if (TLV_TYPE_PORT_ID       == *(ret->list_of_TLVs[j]))
            {
                port_id++;
            }
            else if (TLV_TYPE_TIME_TO_LIVE  == *(ret->list_of_TLVs[j]))
            {
                time_to_live++;
            }
        }

        if (1 != chassis_id || 1 != port_id || 1 != time_to_live)
        {
            // There are too many (or too few) TLVs of one of the required types
            //
            for (j=0; j<i; j++)
            {
                free_lldp_TLV_structure(ret->list_of_TLVs[j]);
            }
            PLATFORM_FREE(ret);
            return NULL;
        }
    }

    return ret;
}


INT8U *forge_lldp_PAYLOAD_from_structure(struct PAYLOAD *memory_structure, INT16U *len)
{
    INT8U i;

    struct chassisIdTLV       *x;
    struct portIdTLV          *y;
    struct timeToLiveTypeTLV  *z;

    INT8U  *stream;
    INT16U  stream_len;

    INT8U  *buffer;
    INT16U  total_len;

    struct endOfLldppduTLV  end_of_lldppdu_tlv = { .tlv_type = TLV_TYPE_END_OF_LLDPPDU };

    // First of all, make sure that the provided PAYLOAD structure contains one
    // (and only one) of the required TLVs (ie. "chassis ID", "port ID" and
    // "time to live") and nothing else.
    //
    {
        INT8U chassis_id    = 0;
        INT8U port_id       = 0;
        INT8U time_to_live  = 0;

        for (i=0; i<MAX_LLDP_TLVS; i++)
        {
            if (NULL == memory_structure->list_of_TLVs[i])
            {
                // No more TLVs
                //
                break;
            }

            if      (TLV_TYPE_CHASSIS_ID    == *(memory_structure->list_of_TLVs[i]))
            {
                chassis_id++;
                x = (struct chassisIdTLV *)memory_structure->list_of_TLVs[i];
            }
            else if (TLV_TYPE_PORT_ID       == *(memory_structure->list_of_TLVs[i]))
            {
                port_id++;
                y = (struct portIdTLV *)memory_structure->list_of_TLVs[i];
            }
            else if (TLV_TYPE_TIME_TO_LIVE  == *(memory_structure->list_of_TLVs[i]))
            {
                time_to_live++;
                z = (struct timeToLiveTypeTLV *)memory_structure->list_of_TLVs[i];
            }
            else
            {
                // Unexpected TLV!
                //
                return NULL;
            }
        }

        if (1 != chassis_id || 1 != port_id || 1 != time_to_live)
        {
            // There are too many (or too few) TLVs of one of the required types
            //
            return NULL;
        }
    }

    // Prepare the output buffer
    //
    buffer = (INT8U *)PLATFORM_MALLOC(MAX_NETWORK_SEGMENT_SIZE);

    // Next, from each of the just filled structures, obtain its packet
    // representation (ie. bit stream layout) and fill 'buffer' with them in
    // order
    //
    total_len = 0;

    stream = forge_lldp_TLV_from_structure((INT8U *)x, &stream_len);
    if (NULL == stream)
    {
        // Could not forge the packet. Error?
        //
        PLATFORM_PRINTF_DEBUG_WARNING("forge_lldp_TLV_from_structure(\"chassis ID\") failed!\n");
        PLATFORM_FREE(buffer);
        return NULL;
    }
    PLATFORM_MEMCPY(buffer + total_len, stream, stream_len);
    PLATFORM_FREE(stream);
    total_len += stream_len;

    stream = forge_lldp_TLV_from_structure((INT8U *)y, &stream_len);
    if (NULL == stream)
    {
        // Could not forge the packet. Error?
        //
        PLATFORM_PRINTF_DEBUG_WARNING("forge_lldp_TLV_from_structure(\"port ID\") failed!\n");
        PLATFORM_FREE(buffer);
        return NULL;
    }
    PLATFORM_MEMCPY(buffer + total_len, stream, stream_len);
    PLATFORM_FREE(stream);
    total_len += stream_len;

    stream = forge_lldp_TLV_from_structure((INT8U *)z, &stream_len);
    if (NULL == stream)
    {
        // Could not forge the packet. Error?
        //
        PLATFORM_PRINTF_DEBUG_WARNING("forge_lldp_TLV_from_structure(\"time to live\") failed!\n");
        PLATFORM_FREE(buffer);
        return NULL;
    }
    PLATFORM_MEMCPY(buffer + total_len, stream, stream_len);
    PLATFORM_FREE(stream);
    total_len += stream_len;

    stream = forge_lldp_TLV_from_structure((INT8U *)&end_of_lldppdu_tlv, &stream_len);
    if (NULL == stream)
    {
        // Could not forge the packet. Error?
        //
        PLATFORM_PRINTF_DEBUG_WARNING("forge_lldp_TLV_from_structure() failed!\n");
        PLATFORM_FREE(buffer);
        return NULL;
    }
    PLATFORM_MEMCPY(buffer + total_len, stream, stream_len);
    PLATFORM_FREE(stream);
    total_len += stream_len;

    *len = total_len;

    return buffer;
}



void free_lldp_PAYLOAD_structure(struct PAYLOAD *memory_structure)
{
    INT8U i;

    i = 0;
    while (memory_structure->list_of_TLVs[i])
    {
        free_lldp_TLV_structure(memory_structure->list_of_TLVs[i]);
        i++;
    }

    PLATFORM_FREE(memory_structure);

    return;
}


INT8U compare_lldp_PAYLOAD_structures(struct PAYLOAD *memory_structure_1, struct PAYLOAD *memory_structure_2)
{
    INT8U i;

    if (NULL == memory_structure_1 || NULL == memory_structure_2)
    {
        return 1;
    }

    i = 0;
    while (1)
    {
        if (NULL == memory_structure_1->list_of_TLVs[i] && NULL == memory_structure_2->list_of_TLVs[i])
        {
            // No more TLVs to compare! Return '0' (structures are equal)
            //
            return 0;
        }

        if (0 != compare_lldp_TLV_structures(memory_structure_1->list_of_TLVs[i], memory_structure_2->list_of_TLVs[i]))
        {
            // TLVs are not the same
            //
            return 1;
        }

        i++;
    }

    // This point should never be reached
    //
    return 1;
}

void visit_lldp_PAYLOAD_structure(struct PAYLOAD *memory_structure, void (*callback)(void (*write_function)(const char *fmt, ...), const char *prefix, INT8U size, const char *name, const char *fmt, void *p), void (*write_function)(const char *fmt, ...), const char *prefix)
{
    // Buffer size to store a prefix string that will be used to show each
    // element of a structure on screen
    //
    #define MAX_PREFIX  100

    INT8U i;

    if (NULL == memory_structure)
    {
        return;
    }

    i = 0;
    while (NULL != memory_structure->list_of_TLVs[i])
    {
        // In order to make it easier for the callback() function to present
        // useful information, append the type of the TLV to the prefix
        //
        char new_prefix[MAX_PREFIX];

        switch(*(memory_structure->list_of_TLVs[i]))
        {
            case TLV_TYPE_END_OF_LLDPPDU:
            {
                PLATFORM_SNPRINTF(new_prefix, MAX_PREFIX-1, "%sTLV(END_OF_LLDPPDU)", prefix, i);
                new_prefix[MAX_PREFIX-1] = 0x0;
                break;
            }

            case TLV_TYPE_CHASSIS_ID:
            {
                PLATFORM_SNPRINTF(new_prefix, MAX_PREFIX-1, "%sTLV(CHASSIS_ID)", prefix, i);
                new_prefix[MAX_PREFIX-1] = 0x0;
                break;
            }

            case TLV_TYPE_PORT_ID:
            {
                PLATFORM_SNPRINTF(new_prefix, MAX_PREFIX-1, "%sTLV(PORT_ID)", prefix, i);
                new_prefix[MAX_PREFIX-1] = 0x0;
                break;
            }

            case TLV_TYPE_TIME_TO_LIVE:
            {
                PLATFORM_SNPRINTF(new_prefix, MAX_PREFIX-1, "%sTLV(TIME_TO_LIVE)", prefix, i);
                new_prefix[MAX_PREFIX-1] = 0x0;
                break;
            }

            default:
            {
                // Unknown TLV. Ignore.
                break;
            }
        }

        visit_lldp_TLV_structure(memory_structure->list_of_TLVs[i], callback, write_function, new_prefix);
        i++;
    }

    return;
}
