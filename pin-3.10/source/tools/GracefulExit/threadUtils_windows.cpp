/*
 * Copyright 2002-2019 Intel Corporation.
 * 
 * This software is provided to you as Sample Source Code as defined in the accompanying
 * End User License Agreement for the Intel(R) Software Development Products ("Agreement")
 * section 1.L.
 * 
 * This software and the related documents are provided as is, with no express or implied
 * warranties, other than those that are expressly stated in the License.
 */

#include <map>
#include <cstdio>
#include <cstdlib>
#include "threadUtils.h"

using std::map;

HANDLE cancellationPoint;


/**************************************************
 * Global variables                               *
 **************************************************/
volatile int numOfThreads = 0;
map<TidType, HANDLE> handles;


/**************************************************
 * Global locks                                   *
 **************************************************/
CRITICAL_SECTION printLock;          // This lock is used for synchronizing prints.
CRITICAL_SECTION numThreadsLock;     // This lock is used for synchronizing access to numOfThreads.


/**************************************************
 * Static functions declaration                   *
 **************************************************/
static void GetLock(CRITICAL_SECTION* thelock);
static void ReleaseLock(CRITICAL_SECTION* thelock);


/**************************************************
 * External functions implementation              *
 **************************************************/
unsigned int GetTid() {
    return (unsigned int)GetCurrentThreadId();
}

void InitLocks() {
    InitializeCriticalSection(&printLock);
    InitializeCriticalSection(&numThreadsLock);
    cancellationPoint = CreateEvent(NULL, TRUE, FALSE, NULL); // manual reset event
}

bool CreateNewThread(TidType* tid, void* func, void* info) {
    HANDLE hThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)func, info, 0, tid);
    if (hThread == NULL) return false;
    handles[*tid] = hThread;
    return true;
}

void CancelThread(TidType tid) {
    if (TerminateThread(handles[tid], 0) == 0) {
        ErrorExit(RES_CANCEL_FAILED);
    }
}

void WaitForThread(TidType tid) {
    if (WaitForSingleObject(handles[tid], INFINITE) == WAIT_FAILED) {
        ErrorExit(RES_JOIN_FAILED);
    }
}

void TimeoutThreadFunc(void* info){
    DoSleep(TIMEOUT);

    // Should never get here, only if timeout occurred
    Print("Got timeout, exiting test with an error");
    ErrorExit(RES_EXIT_TIMEOUT);
}

void SetTimeout(){
   TidType tid;
   if (!CreateNewThread(&tid, (void *)TimeoutThreadFunc, NULL)) {
       Print("Could not open a timeout thread");
       ErrorExit(RES_CREATE_FAILED);
   }
}


void ThreadExit() {
    ExitThread(0);
}

void IncThreads() {
    GetLock(&numThreadsLock);
    ++numOfThreads;
    ReleaseLock(&numThreadsLock);
}

void DecThreads() {
    GetLock(&numThreadsLock);
    --numOfThreads;
    ReleaseLock(&numThreadsLock);
}

int NumOfThreads() {
    return numOfThreads;
}

void Print(const string& str) {
    GetLock(&printLock);
    fprintf(stderr, "APP:  <%d> %s\n", GetTid(), str.c_str());
    fflush(stderr);
    ReleaseLock(&printLock);
}

void ErrorExit(Results res) {
    GetLock(&printLock);
    fprintf(stderr, "APP ERROR <%d>: %s\n", GetTid(), errorStrings[res].c_str());
    fflush(stderr);
    ReleaseLock(&printLock);
    exit(res);
}

void DoSleep(unsigned int seconds) {
    Sleep(seconds*1000);
}

void DoYield() {
    Yield();
}

void EnterSafeCancellationPoint() {
    WaitForSingleObject(cancellationPoint, INFINITE);
}


/**************************************************
 * Static functions implementation                *
 **************************************************/
void GetLock(CRITICAL_SECTION* thelock) {
    EnterCriticalSection(thelock);
}

void ReleaseLock(CRITICAL_SECTION* thelock) {
    LeaveCriticalSection(thelock);
}
