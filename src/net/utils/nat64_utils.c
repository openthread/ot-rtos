/*
 *  Copyright (c) 2019, The OpenThread Authors.
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are met:
 *  1. Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *  2. Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *  3. Neither the name of the copyright holder nor the
 *     names of its contributors may be used to endorse or promote products
 *     derived from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 *  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 *  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 *  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 *  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 *  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 *  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 *  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 *  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 *  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 *  POSSIBILITY OF SUCH DAMAGE.
 */

#include "net/utils/nat64_utils.h"

#include "lwip/netdb.h"

static ip6_addr_t sNat64Prefix;

void setNat64Prefix(const ip6_addr_t *aNat64Prefix)
{
    sNat64Prefix = *aNat64Prefix;
}

ip6_addr_t getNat64Address(const ip4_addr_t *aIpv4Address)
{
    ip6_addr_t addr = sNat64Prefix;
    addr.addr[3]    = aIpv4Address->addr;
    addr.zone       = IP6_NO_ZONE;
    return addr;
}

int dnsNat64Address(const char *aHostName, ip6_addr_t *aAddrOut)
{
    ip4_addr_t      v4Addr;
    struct hostent *host = gethostbyname(aHostName);
    int             ret  = -1;
    if (host && host->h_addr_list && host->h_addrtype == AF_INET)
    {
        memcpy(&v4Addr.addr, host->h_addr_list[0], sizeof(v4Addr.addr));
        *aAddrOut = getNat64Address(&v4Addr);
        ret       = 0;
    }

    return ret;
}
