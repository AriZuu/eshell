/*
 * Copyright (c) 2001, 2002 Swedish Institute of Computer Science.
 * Copyright (c) 2015, Ari Suutari <ari@stonepile.fi>.
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
#include <picoos-lwip.h>
#include <lwip/icmp.h>
#include <lwip/inet_chksum.h>
#include <lwip/ip4.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

#if LWIP_RAW
#include "eshell.h"
#include "eshell-commands.h"

#define PING_DATA_SIZE 50

typedef struct __attribute__((packed)) {

  struct icmp_echo_hdr hdr;
  char data[PING_DATA_SIZE];
} PingPacket;

static int pingSend(int s, struct sockaddr *to, int toLen, int seq)
{
  size_t i;
  PingPacket packet;

  packet.hdr.type = ICMP_ECHO;
  packet.hdr.code = 0;
  packet.hdr.chksum = 0;
  packet.hdr.id = 0x1942;
  packet.hdr.seqno = htons(seq);

  for (i = 0; i < PING_DATA_SIZE; i++)
  packet.data[i] = (char)i;

  packet.hdr.chksum = inet_chksum(&packet, sizeof(packet));

  return sendto(s, &packet, sizeof(packet), 0, to, toLen);
}

static bool pingReceive(int s, struct sockaddr *to, int toLen, int seq)
{
  int len;
  char buf[sizeof(struct ip_hdr) + sizeof(PingPacket)];
  PingPacket *packet;

  struct sockaddr_in from;
  int fromlen;
  struct ip_hdr *iphdr;

  fromlen = sizeof(struct sockaddr_in);

  while ((len = recvfrom(s, buf, sizeof(buf), 0, (struct sockaddr *)&from, (socklen_t *) &fromlen)) > 0) {

    if (len >= (int)(sizeof(struct ip_hdr) + sizeof(struct icmp_echo_hdr))) {

      if (from.sin_family == AF_INET && from.sin_addr.s_addr == ((struct sockaddr_in*)to)->sin_addr.s_addr) {

        iphdr = (struct ip_hdr *)buf;
        packet = (PingPacket*)(buf + (IPH_HL(iphdr) * 4));
        if ((packet->hdr.id == 0x1942) && (packet->hdr.seqno == htons(seq))) {

          return true;
        }
      }
    }

    fromlen = sizeof(struct sockaddr_in);
  }

  return false;
}

static int ping(EshContext * ctx)
{
  eshCheckNamedArgsUsed(ctx);
  char *host = eshNextArg(ctx, true);

  eshCheckArgsUsed(ctx);
  if (eshArgError(ctx) != EshOK)
    return -1;

  struct addrinfo hints;
  struct addrinfo *res;
  int error;

  memset(&hints, '\0', sizeof(hints));
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_RAW;
  hints.ai_protocol = IPPROTO_ICMP;

  error = getaddrinfo(host, NULL, &hints, &res);
  if (error) {

    eshPrintf(ctx, "ping: unknown host %s\n", host);
    return -1;
  }

  int s;
  struct timeval timeout;

  timeout.tv_sec = 1;
  timeout.tv_usec = 0;

  if ((s = socket(res->ai_family, res->ai_socktype, res->ai_protocol)) < 0) {

    freeaddrinfo(res);
    eshPrintf(ctx, "ping: cannot create socket.\n");
    return -1;
  }

  setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));

  int i;
  int okCount = 0;
  int okJiffies = 0;
  JIF_t start;
  int max = 10;
  bool ok;

  eshPrintf(ctx, "%s: ", host);
  for (i = 0; i < max; i++) {

    start = jiffies;
    if (pingSend(s, res->ai_addr, res->ai_addrlen, i) >= 0) {

      ok = pingReceive(s, res->ai_addr, res->ai_addrlen, i);
      if (ok) {

        okCount++;
        okJiffies += (jiffies - start);
      }

      eshPrintf(ctx, "%s", ok ? "." : "!");
    }
    else {

      eshPrintf(ctx, " send failed.");
      break;
    }

    posTaskSleep(MS(500));
  }

  closesocket(s);
  eshPrintf(ctx, " %d%% success", (int)((100.0 * okCount) / max));
  if (okCount)
    eshPrintf(ctx, ", avg delay %d ms", (int)(1000 * okJiffies / HZ / okCount));

  eshPrintf(ctx, ".\n");
  freeaddrinfo(res);
  return 0;
}

const EshCommand eshPingCommand = {
  .flags = 0,
  .name = "ping",
  .help = "n.n.n.n",
  .handler = ping
};

#endif
