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

#include <picoos.h>
#include <picoos-u.h>
#include <string.h>

#include "eshell.h"

#if ESHELLCFG_ONEWIRE

#include <picoos-ow.h>
#include <temp10.h>

static int onewire(EshContext * ctx)
{
  eshCheckNamedArgsUsed(ctx);
  eshCheckArgsUsed(ctx);
  if (eshArgError(ctx) != EshOK)
    return -1;

  int   rslt;
  uint8_t serialNum[8];
  float value;
  int i;

  if (!owAcquire(0, NULL)) {

    eshPrintf(ctx, "owAcquire failed.\n");
    return -1;
  }

  rslt = owFirst(0, TRUE, FALSE);

  while (rslt) {

    owSerialNum(0, serialNum, TRUE);

    for (i = 7; i >= 0; i--) {

      eshPrintf(ctx, "%02X", (int)serialNum[i]);
      if (i > 0)
        eshPrintf(ctx, "-");
     }

    ReadTemperature(0, serialNum, &value);
    eshPrintf(ctx, "=%1.1f\n", value);
    rslt = owNext(0, TRUE, FALSE);
  }

  owRelease(0);
  return 0;
}

const EshCommand eshOnewireCommand = {
  .flags = 0,
  .name = "onewire",
  .help = "list onewire bus",
  .handler = onewire
};

#endif
