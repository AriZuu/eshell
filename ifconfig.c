/*
 * Copyright (c) 2016, Ari Suutari <ari@stonepile.fi>.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 *  1. Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *  2. Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *  3. The name of the author may not be used to endorse or promote
 *     products derived from this software without specific prior written
 *     permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT,
 * INDIRECT,  INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * This loosely is based on LwIP ping app.
 */

#include <picoos.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>


#include "eshell.h"

#if ESHELLCFG_LWIP

#include <picoos-lwip.h>
#include "lwip/netif.h"
#include "lwip/netifapi.h"
#include "eshell-commands.h"

static int ifconfig(EshContext * ctx)
{
  eshCheckNamedArgsUsed(ctx);
  eshCheckArgsUsed(ctx);
  if (eshArgError(ctx) != EshOK)
    return -1;

  const struct netif* ifPtr = netif_list;
  while (ifPtr) {

    char buf[50];

    inet_ntoa_r(*netif_ip4_addr(ifPtr), buf, sizeof(buf));
    eshPrintf(ctx, "%.2s%d: inet4 %s", ifPtr->name, ifPtr->num, buf);

    inet_ntoa_r(*netif_ip4_netmask(ifPtr), buf, sizeof(buf));
    eshPrintf(ctx, " netmask %s\n", buf);

#if  LWIP_IPV6
   
    int i;

    for (i = 0; i < LWIP_IPV6_NUM_ADDRESSES; i++) {

      if (netif_ip6_addr_state(ifPtr, i) != 0) {

        inet6_ntoa_r(*netif_ip6_addr(ifPtr, i), buf, sizeof(buf));
        eshPrintf(ctx, "     inet6 %s\n", buf);
      }
    }
#endif

    ifPtr = ifPtr->next;
  }

  return 0;
}

const EshCommand eshIfconfigCommand = {
  .flags = 0,
  .name = "ifconfig",
  .help = "show interface settings",
  .handler = ifconfig
};

#endif
