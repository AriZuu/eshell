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

#if defined(POS_DEBUGHELP)

/*
 * Versions of 'show tasks' and 'show events' commands that dig
 * information out from pico]OS debug help structures.
 */

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

    name = (task->name != NULL) ? task->name : "?";

#if POSCFG_ARGCHECK > 1

    int freeStack;
    unsigned char* sp;

    freeStack = 0;

    sp = task->handle->stack;
    while (*sp == PORT_STACK_MAGIC) {
      ++sp;
      ++freeStack;
    }

    eshPrintf(ctx, "%08X %s %d\n", task->handle, name, freeStack);
#else
    eshPrintf(ctx, "%08X %s\n", task->handle, name);
#endif
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
  const char* typeName;

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

    switch (event->type) {
    case event_semaphore:
      typeName = "sem";
      break;

    case event_mutex:
      typeName = "mutex";
      break;

    case event_flags:
      typeName = "flag";
      break;

    }

    name = (event->name != NULL) ? event->name : "?";
    eshPrintf(ctx, "%06X %-5s %s 0x%X\n", event->handle, typeName, name, event->counter);
  }


  eshPrintf(ctx, "%d events, %d conf max.\n", eventCount, POSCFG_MAX_EVENTS);
  return 0;
}

#elif NOSCFG_FEATURE_REGISTRY

/*
 * Alternative versions of 'show tasks' and 'show events' commands that dig
 * information out from pico]OS registry. This has less runtime
 * performance impacts than the version using debug help stuff.
 * The downside is that only nano-layer objects are reported.
 */
static int ts(EshContext * ctx)
{
  eshCheckNamedArgsUsed(ctx);
  eshCheckArgsUsed(ctx);
  if (eshArgError(ctx) != EshOK)
    return -1;

  int taskCount = 0;

  NOSREGQHANDLE_t q;
  NOSGENERICHANDLE_t h;
  POSTASK_t task;
  char name[80];

  q = nosRegQueryBegin(REGTYPE_TASK);
  while (nosRegQueryElem(q, &h, name, sizeof(name)) == E_OK) {

    task = (POSTASK_t)h;
    taskCount++;

#if POSCFG_ARGCHECK > 1

    int freeStack;
    unsigned char* sp;
    freeStack = 0;

    sp = task->stack;
    while (*sp == PORT_STACK_MAGIC) {
      ++sp;
      ++freeStack;
    }

    eshPrintf(ctx, "%08X %s %d\n", task, name, freeStack);
#else
    eshPrintf(ctx, "%08X %s\n", task, name);
#endif
  }

  nosRegQueryEnd(q);

  eshPrintf(ctx, "%d nano tasks + idle task, %d conf max.\n", taskCount, POSCFG_MAX_TASKS);
  return 0;
}

static int listEvents(EshContext* ctx, int type, const char* typeName)
{
  NOSREGQHANDLE_t q;
  NOSGENERICHANDLE_t h;
  char name[80];
  int eventCount = 0;

  q = nosRegQueryBegin(type);
  while (nosRegQueryElem(q, &h, name, sizeof(name)) == E_OK) {

    eventCount++;
    eshPrintf(ctx, "%06X %-5s %s\n", h, typeName, name);
  }

  nosRegQueryEnd(q);
  return eventCount;
}

static int es(EshContext * ctx)
{
  eshCheckNamedArgsUsed(ctx);
  eshCheckArgsUsed(ctx);
  if (eshArgError(ctx) != EshOK)
    return -1;

  int eventCount = 0;

#if NOSCFG_FEATURE_SEMAPHORES
  eventCount += listEvents(ctx, REGTYPE_SEMAPHORE, "sem");
#endif

#if NOSCFG_FEATURE_FLAGS
  eventCount += listEvents(ctx, REGTYPE_FLAG, "flag");
#endif

#if NOSCFG_FEATURE_MUTEXES
  eventCount += listEvents(ctx, REGTYPE_MUTEX, "mutex");
#endif

  eshPrintf(ctx, "%d nano events, %d conf max.\n", eventCount, POSCFG_MAX_EVENTS);
  return 0;
}

#endif

#if defined(POS_DEBUGHELP) || NOSCFG_FEATURE_REGISTRY

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
