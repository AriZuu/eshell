/*
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

#include <picoos.h>
#include <picoos-lwip.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdarg.h>
#include <unistd.h>

#include "eshell.h"

static void telnetd(void*);

#define IAC 255

static void telnetFunc(EshContext* ctx, const char* buf)
{
  const unsigned char* ptr = (const unsigned char*)buf;

  while (*ptr) {

    if (ctx->state) { // CR state

      if (*ptr != '\n')
        fputc(0, ctx->stream);

       ctx->state = 0;
    }

    if (*ptr == IAC)
      fputc(IAC, ctx->stream);

    fputc(*ptr, ctx->stream);
    if (*ptr == '\r')
      ctx->state = 1;

    ++ptr;
  }

  fflush(ctx->stream);
}

static void tcpClientThread(void* arg)
{
  int sock = (int)arg;
  char buf[80];
  EshContext ctx;

  ctx.stream = fdopen(sock, "w+");
  ctx.output = telnetFunc;
  ctx.state  = 0;

  eshPrintf(&ctx, "Hello!\n");
  while (true) {

    eshPrintf(&ctx, "#> ");
    if (fgets(buf, sizeof(buf) - 1, ctx.stream) != NULL) {

      buf[strlen(buf) - 2] = '\0';
      eshParse(&ctx, buf);
    }
    else
      break;
  }
  
  fclose(ctx.stream);
}

static void telnetd(void* arg)
{
  int listenSock = (int)arg;
  int sock;
  struct sockaddr_in peerAddr;
  socklen_t addrlen;

  for(;;) {

/*
 * Wait for new connection. 
 */
    addrlen = sizeof(peerAddr);
    sock = accept(listenSock, (struct sockaddr*)&peerAddr, &addrlen);
    if (sock == -1) {

      printf("telnetd: failed to accept incoming connection.\n");
      posTaskSleep(MS(30000));
      continue;
    }
/*
 * Create thread to serve connection.
 */
    if (posTaskCreate(tcpClientThread, (void*)sock, 2, 3500) == NULL)
       close(sock);
  }
}

void eshStartTelnetd()
{
  int sock;
  struct sockaddr_in myAddr;
  int status;

  sock = socket(AF_INET, SOCK_STREAM, 0);
  if (sock == -1) {

    printf("telnetd: Socket creation error.\n");
    return;
  }

  myAddr.sin_family = AF_INET;
  myAddr.sin_addr.s_addr = INADDR_ANY;
  myAddr.sin_port = htons(23);

  status = bind(sock, (struct sockaddr*)&myAddr, sizeof(myAddr));
  if (status == -1) {

    printf("telnetd: socket bind error.\n");
    return;
  }

  status = listen(sock, 5);
  if (status == -1) {

    printf("telnetd: listen error.\n");
    return;
  }

  if (posTaskCreate(telnetd, (void*)sock, 2, 1500) == NULL) {

    close(sock);
    fprintf(stderr, "telnetd: failed to create thread.\n");
  }
}
