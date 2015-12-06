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

#define STATE_NORMAL 0
#define STATE_IAC    1
#define STATE_WILL   2
#define STATE_WONT   3
#define STATE_DO     4
#define STATE_DONT   5
#define STATE_CLOSE  6
#define STATE_CR     7

#define TELNET_IAC   255
#define TELNET_WILL  251
#define TELNET_WONT  252
#define TELNET_DO    253
#define TELNET_DONT  254


static void telnetFunc(EshContext* ctx, const char* buf)
{
  const unsigned char* ptr = (const unsigned char*)buf;

  while (*ptr) {

    if (ctx->state) { // CR state

      if (*ptr != '\n')
        fputc(0, ctx->outputStream);

       ctx->state = 0;
    }

    if (*ptr == TELNET_IAC)
      fputc(TELNET_IAC, ctx->outputStream);

    fputc(*ptr, ctx->outputStream);
    if (*ptr == '\r')
      ctx->state = 1;

    ++ptr;
  }

  fflush(ctx->outputStream);
}

static void sendOpt(EshContext* ctx, uint8_t option, uint8_t value)
{
  fputc(TELNET_IAC, ctx->outputStream);
  fputc(option, ctx->outputStream);
  fputc(value, ctx->outputStream);
  fflush(ctx->outputStream);
}

static bool inputFunc(EshContext* ctx, char* data, int max)
{
  int c;
  int len = 0;
  bool gotLine = false;
  max = max - 1;

  do {
    
    c = fgetc(ctx->inputStream);
    if (c == EOF)
      return false;

    switch (ctx->state)
    {
    case STATE_IAC:
      if (c == TELNET_IAC) {

        *data++ = c;
        ++len;

        ctx->state = STATE_NORMAL;
      }
      else {

        switch (c)
        {
        case TELNET_WILL:
          ctx->state = STATE_WILL;
          break;

        case TELNET_WONT:
          ctx->state = STATE_WONT;
          break;

        case TELNET_DO:
          ctx->state = STATE_DO;
          break;

        case TELNET_DONT:
          ctx->state = STATE_DONT;
          break;

        default:
          ctx->state = STATE_NORMAL;
          break;
        }
      }
      break;

    case STATE_WILL:
      /* Reply with a DONT */
      sendOpt(ctx, TELNET_DONT, c);
      ctx->state = STATE_NORMAL;
      break;

    case STATE_WONT:
      /* Reply with a DONT */
      sendOpt(ctx, TELNET_DONT, c);
      ctx->state = STATE_NORMAL;
      break;

    case STATE_DO:
      /* Reply with a WONT */
      sendOpt(ctx, TELNET_WONT, c);
      ctx->state = STATE_NORMAL;
      break;

    case STATE_DONT:
      /* Reply with a WONT */
      sendOpt(ctx, TELNET_WONT, c);
      ctx->state = STATE_NORMAL;
      break;

    case STATE_CR:
      ctx->state = STATE_NORMAL;
      gotLine = true;
      break;

    case STATE_NORMAL:
      if (c == TELNET_IAC) {

        ctx->state = STATE_IAC;
      }
      else if (c == '\r') {

        ctx->state = STATE_CR;
      }
      else {

        if (c == '\n') {

          gotLine = true;
        }
        else {

          *data++ = c;
          ++len;
        }
      }
      break;
    }

  } while(!gotLine && len < max - 1);

  *data = '\0';
  fputc('\n', ctx->outputStream);

  return true;
}

static void tcpClientThread(void* arg)
{
  int sock = (int)arg;
  char buf[80];
  EshContext ctx;
  struct timeval tv;

  tv.tv_sec = 15;
  tv.tv_usec = 0;

  setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

  ctx.outputStream = fdopen(sock, "w+");
  ctx.inputStream = ctx.outputStream;
  ctx.output = telnetFunc;
  ctx.input = inputFunc;
  ctx.state  = STATE_NORMAL;

  eshPrintf(&ctx, "Hello!\n");
  while (true) {

    if (eshPrompt(&ctx, "#> ", buf, sizeof(buf))) {

      if (eshParse(&ctx, buf) == 0)
        break;
    }
    else
      break;
  }
  
  fclose(ctx.outputStream);
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
