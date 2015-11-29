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

#define MAX_ARGS	10

typedef enum {

  EshOK,
  EshUnknownArg,
  EshUnknownCmd,
  EshBadArg,
  EshDuplicateArg,
  EshMissingArg
} EshStatus;

struct _eshContext;

typedef struct {

  const char* name;
  const char* help;
  int (*handler)(struct _eshContext* ctx);
  
} EshCommand;

typedef struct _eshContext {

  void  (*output)(struct _eshContext* ctx, const char*);
  FILE* stream;
  int   argc;
  char* argv[MAX_ARGS];
  EshStatus error;
  const EshCommand* command;
  int   state;

} EshContext;  

void  eshPrintf(EshContext*ctx, const char* fmt, ...);
char* eshNextArg(EshContext* ctx, bool must);
char* eshNamedArg(EshContext* ctx, const char* name, bool must);
EshStatus eshArgError(EshContext* ctx);
void  eshCheckNamedArgsUsed(EshContext* ctx);
void  eshCheckArgsUsed(EshContext* ctx);
int   eshParse(EshContext* ctx, char* buf);
void  eshConsole(void);
