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

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdarg.h>

#include "eshell.h"
#include "eshell-commands.h"

static int help(EshContext* ctx);
static int exitShell(EshContext* ctx);

const EshCommand eshHelpCommand = {

  .name = "help",
  .help = "displays help",
  .handler = help
};

const EshCommand eshExitCommand = {

  .name = "exit",
  .help = "exit shell",
  .handler = exitShell
};

bool eshPrompt(EshContext*ctx, const char* prompt, char* buf, int max)
{
  ctx->output(ctx, prompt);
  return ctx->input(ctx, buf, max - 1);
}

void eshPrintf(EshContext*ctx, const char* fmt, ...)
{
  va_list ap;
  char buf[80];

  va_start(ap, fmt);
  vsnprintf(buf, sizeof(buf), fmt, ap);
  ctx->output(ctx, buf);
  va_end(ap);
}

EshStatus eshArgError(EshContext* ctx)
{
  return ctx->error;
}

void eshCheckArgsUsed(EshContext* ctx)
{
  int i;

  if (ctx->error != EshOK)
    return;

  if (ctx->argc == 1)
    return;

  eshPrintf(ctx, "%s: unknown parameter(s):", ctx->argv[0]);
  for (i = 1; i < ctx->argc; i++)
    eshPrintf(ctx, " %s", ctx->argv[i]);

  eshPrintf(ctx, "\n");
  ctx->error = EshUnknownArg;
}

void eshCheckNamedArgsUsed(EshContext* ctx)
{
  int i;

  if (ctx->error != EshOK)
    return;

  for (i = 1; i < ctx->argc; i++)
    if (!strncmp(ctx->argv[i], "--", 2))
      break;

  if (i >= ctx->argc)
    return;

  eshPrintf(ctx, "%s: unknown parameter(s):", ctx->argv[0]);
  for (i = 1; i < ctx->argc; i++)
    if (!strncmp(ctx->argv[i], "--", 2))
      eshPrintf(ctx, " %s", ctx->argv[i]);

  eshPrintf(ctx, "\n");
  ctx->error = EshUnknownArg;
}

char* eshNextArg(EshContext* ctx, bool must)
{
  int i;
  char* arg;

  if (ctx->error != EshOK)
    return NULL;

/*
 * Search for argument.
 */
  for (i = 1; i < ctx->argc; i++) {

    arg = ctx->argv[i];
    if (strncmp(arg, "--", 2)) {

      if (i < ctx->argc - 1)
        memmove(ctx->argv + i, ctx->argv + i + 1, sizeof(char*) * (ctx->argc - i - 1));

      ctx->argc--;
      return arg;
    }
  }

  if (must)
    ctx->error = EshMissingArg;

  return NULL;
}

char* eshNamedArg(EshContext* ctx, const char* name, bool must)
{
  int i;
  char* sep;
  int   len;
  char* arg;

  if (ctx->error != EshOK)
    return NULL;

/*
 * Search for argument.
 */

  for (i = 1; i < ctx->argc; i++) {

    arg = ctx->argv[i];
    if (!strncmp(arg, "--", 2)) {

      arg = arg + 2;
      sep = strchr(arg, '=');
      if (sep != NULL)
        len = sep - arg;
      else
        len = strlen(arg);

      if (!strncmp(name, arg, len)) {

        if (i < ctx->argc - 1)
          memmove(ctx->argv + i, ctx->argv + 2, sizeof(char*) * (ctx->argc - i - 1));

        ctx->argc--;
        if (sep != NULL) {

          arg = sep + 1;
          if (strlen(arg) == 0) {

            eshPrintf(ctx, "%s: --%s: missing argument value .\n", ctx->argv[0], name);
            ctx->error = EshBadArg;
            return NULL;
          }
        }
        else
          arg = "";

        if (eshNamedArg(ctx, name, false) != NULL) {

          eshPrintf(ctx, "%s: --%s specified multiple times.\n", ctx->argv[0], name);
          ctx->error = EshDuplicateArg;
          return NULL;
        }

        return arg;
      }
    }
  }

  if (must) {

    ctx->error = EshMissingArg;
  }

  return NULL;
}

static void usage(EshContext* ctx, const EshCommand* cmd)
{
  eshPrintf(ctx, "%s", cmd->name);
  if (cmd->help != NULL)
    eshPrintf(ctx, " %s", cmd->help);

  eshPrintf(ctx, "\n");
}

static int help(EshContext* ctx)
{
  const EshCommand** cmd;

  for (cmd = eshCommandList; *cmd != NULL; cmd++)
    usage(ctx, *cmd);

  return 0;
}

static int exitShell(EshContext* ctx)
{
  ctx->error = EshQuit;
  return 0;
}

int eshParse(EshContext* ctx, char* buf)
{
  char* cmdName;
  char* argStr;

  ctx->error = EshOK;
  ctx->argc = 0;
  ctx->command = NULL;

  do {

    cmdName = strsep(&buf, " \t");
    if (cmdName == NULL)
       return 1;

  } while (*cmdName == '\0');

  const EshCommand** cmd;

  for (cmd = eshCommandList; *cmd != NULL; cmd++) {

    if (!strcmp((*cmd)->name, cmdName))
      break;
  }

  if (*cmd == NULL) {

    eshPrintf(ctx, "%s: Unknown command, try help.\n", cmdName);
    return -1;
  }

  ctx->command = *cmd;
  ctx->argv[ctx->argc++] = cmdName;
  bool positionalArgSeen = false;

  while ((argStr = strsep(&buf, " \t")) != NULL) {

    if (*argStr == '\0')
      continue;

    if (positionalArgSeen && !strncmp(argStr, "--", 2)) {

      eshPrintf(ctx, "%s: %s: named arguments must be before positional ones.\n", cmdName, argStr);
      return -1;
    }

    if (strncmp(argStr, "--", 2))
      positionalArgSeen = true;

    if (ctx->argc >= MAX_ARGS) {

      eshPrintf(ctx, "%s: Too many arguments.\n", cmdName);
      return -1;
    }

    ctx->argv[ctx->argc++] = argStr;
  }
  
  char* help = eshNamedArg(ctx, "help", false);
  if (help != NULL && strlen(help) == 0) {

    usage(ctx, *cmd);
    return 1;
  }
  else {

    (*cmd)->handler(ctx);
    switch (ctx->error) {
    case EshQuit:
      return 0;

    case EshOK:
    case EshUnknownCmd:
    case EshBadArg:
    case EshDuplicateArg:
      break;

    case EshUnknownArg:
      eshPrintf(ctx, "try %s --help\n", ctx->argv[0]);
      break;

    case EshMissingArg:
      eshPrintf(ctx, "%s: missing parameters.\n", ctx->argv[0]);
      eshPrintf(ctx, "try %s --help\n", ctx->argv[0]);
      break;
    }

    return 1;
  }
}
