/*
 * Copyright (c) 2012-2013, Ari Suutari <ari@stonepile.fi>.
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

#if defined(POS_DEBUGHELP) && POSCFG_ARGCHECK > 1

static int ts(EshContext * ctx)
{
  eshCheckNamedArgsUsed(ctx);
  eshCheckArgsUsed(ctx);
  if (eshArgError(ctx) != EshOK)
    return -1;

  int taskCount = 0;
  int i;
  struct PICOTASK* task;
  struct PICOTASK* allTasks[POSCFG_MAX_TASKS];
  const char* name;
  int freeStack;
  unsigned char* sp;

  memset(allTasks, '\0', sizeof(allTasks));

  posTaskSchedLock();
  task = picodeb_tasklist;
  while (task != NULL) {

    allTasks[taskCount] = task;
    taskCount++;
    task = task->next;
  }

  posTaskSchedUnlock();

  for (i = 0; i < taskCount; i++) {

    task = allTasks[i];

    if (task->state == task_notExisting)
      continue;

    freeStack = 0;

    sp = task->handle->stack;
    while (*sp == PORT_STACK_MAGIC) {
      ++sp;
      ++freeStack;
    }

    name = (task->name != NULL) ? task->name : "?";
    eshPrintf(ctx, "%06X task %s %d\n", task->handle, name, freeStack);
  }


  eshPrintf(ctx, "%d tasks, %d conf max.\n", taskCount, POSCFG_MAX_TASKS);
  return 0;
}

static int es(EshContext * ctx)
{
  eshCheckNamedArgsUsed(ctx);
  eshCheckArgsUsed(ctx);
  if (eshArgError(ctx) != EshOK)
    return -1;

  int eventCount = 0;
  int i;
  struct PICOEVENT* event;
  struct PICOEVENT* allEvents[POSCFG_MAX_EVENTS];
  const char* name;

  memset(allEvents, '\0', sizeof(allEvents));

  posTaskSchedLock();
  event = picodeb_eventlist;
  while (event != NULL) {

    allEvents[eventCount] = event;
    eventCount++;
    event = event->next;
  }

  posTaskSchedUnlock();

  for (i = 0; i < eventCount; i++) {

    event = allEvents[i];

    name = (event->name != NULL) ? event->name : "?";
    eshPrintf(ctx, "%06X event %s 0x%X\n", event->handle, name, event->state);
  }


  eshPrintf(ctx, "%d events, %d conf max.\n", eventCount, POSCFG_MAX_EVENTS);
  return 0;
}

const EshCommand eshTsCommand = {
  .flags = 0,
  .name = "ts",
  .help = "show tasks",
  .handler = ts
};

const EshCommand eshEsCommand = {
  .flags = 0,
  .name = "es",
  .help = "show events",
  .handler = es
};

#endif
