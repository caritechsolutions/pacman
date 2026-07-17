/*
 * MultiThreading.h
 *****************************************************************************
 * Copyright (C) 2012, bitmovin Softwareentwicklung OG, All Rights Reserved
 *
 * Email: libdash-dev@vicky.bitmovin.net
 *
 * This source code and its use and distribution, is subject to the terms
 * and conditions of the applicable license agreement.
 *****************************************************************************/

#ifndef LIBDASH_FRAMEWORK_PORTABLE_MULTITHREADING_H_
#define LIBDASH_FRAMEWORK_PORTABLE_MULTITHREADING_H_

#include <string.h>
#include <pthread.h>
#include <errno.h>
#include <stdlib.h>
#include <iostream>
#include <unistd.h>
#include "gxos/gxcore_os_core.h"
#define CRITICAL_SECTION    handle_t
#define CONDITION_VARIABLE  handle_t
#define SEM_VARIABLE        handle_t

#define PortableSleep(ms)                                   GxCore_ThreadDelay(ms)
#define JoinThread(handle)                                  GxCore_ThreadJoin(handle)
#define InitializeCriticalSection(mutex_p)                  GxCore_MutexCreate(mutex_p)
#define DeleteCriticalSection(mutex_p)                      GxCore_MutexDelete(*(mutex_p))
#define EnterCriticalSection(mutex_p)                       GxCore_MutexLock(*(mutex_p))
#define LeaveCriticalSection(mutex_p)                       GxCore_MutexUnlock(*(mutex_p))
#define InitializeConditionVariable(cond_p)                 GxCore_CondInit(cond_p)
#define DeleteConditionVariable(cond_p)                     GxCore_CondDelete(*(cond_p))
#define SleepConditionVariableCS(cond_p, mutex_p, infinite) GxCore_CondWait(*(cond_p), *(mutex_p), 0) // INFINITE should be handled mor properly
#define WakeConditionVariable(cond_p)                       GxCore_CondSignal(*(cond_p))
#define WakeAllConditionVariable(cond_p)                    GxCore_CondBroadcast(*(cond_p))
#define InitializeSem(sem_p, sem_v)                         GxCore_SemCreate(sem_p, sem_v)
#define WakeSem(sem_p)                                      GxCore_SemPost(*(sem_p))
#define WaitSem(sem_p)                                      GxCore_SemWait(*(sem_p))
#define DeleteSem(sem_p)                                    GxCore_SemDelete(*(sem_p))

typedef handle_t THREAD_HANDLE;

THREAD_HANDLE   CreateThread    (void (*start_routine) (void *), void *arg);

#endif  // LIBDASH_FRAMEWORK_PORTABLE_MULTITHREADING_H_
