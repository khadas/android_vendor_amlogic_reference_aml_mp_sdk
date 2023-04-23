/*
 * Copyright (C) 2007 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#define LOG_TAG "AmlMpThread"
#include "AmlMpThread.h"
#include "AmlMpLog.h"
#include <unistd.h>
#include <sys/syscall.h>
#include <pthread.h>
#include <string.h>

static const char* mName = LOG_TAG;

namespace aml_mp {
AmlMpThread::AmlMpThread()
: mStatus(0)
, mExitPending(false)
, mRunning(false)
, mThread(-1)
, mTid(-1)
{

}

AmlMpThread::~AmlMpThread()
{

}

int AmlMpThread::readyToRun()
{
    return 0;
}

typedef int (*thread_func_t)(void*);

struct ThreadData {
    thread_func_t entryFunction;
    void* userData;
    char* threadName;

    static void* trampoline(void* t) {
        ThreadData* userData = (ThreadData*)t;
        thread_func_t f = userData->entryFunction;
        void* u = userData->userData;
        char* name = userData->threadName;
        delete userData;

        if (name) {
            pthread_setname_np(pthread_self(), name);
            free(name);
        }

        f(u);
        return nullptr;
    }
};

int AmlMpThread::run(const char* name)
{
    std::lock_guard<std::mutex> _l(mLock);
    if (mRunning) {
        return -EINVAL;
    }

    mStatus = 0;
    mExitPending = false;
    mRunning = true;
    mThread = -1;
    mTid = -1;

    mHoldSelf = this;


    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

    ThreadData* t = new ThreadData;
    t->threadName = strdup(name);
    t->entryFunction = _threadLoop;
    t->userData = this;

    pthread_t thread;
    int result = pthread_create(&thread, &attr, &ThreadData::trampoline, t);
    pthread_attr_destroy(&attr);
    if (result != 0) {
        MLOGE("pthread create failed %d", result);

        if (t->threadName) {
            free(t->threadName);
        }
        delete t;

        mStatus = -EINVAL;
        mRunning = false;
        mThread = -1;
        mHoldSelf.clear();

        return mStatus;
    }

    mThread = thread;

    return 0;
}

int AmlMpThread::_threadLoop(void* user)
{
    AmlMpThread* const self = static_cast<AmlMpThread*>(user);

    sptr<AmlMpThread> strong(self->mHoldSelf);
    wptr<AmlMpThread> weak(strong);
    self->mHoldSelf.clear();

    self->mTid = syscall(__NR_gettid);
    bool first = true;

    do {
        bool result;
        if (first) {
            first =false;
            self->mStatus = self->readyToRun();
            result = self->mStatus == 0;
            if (result && !self->exitPending()) {
                result = self->threadLoop();
            }
        } else {
            result = self->threadLoop();
        }

        {
            std::unique_lock<std::mutex> _l(self->mLock);
            if (result == false || self->mExitPending) {
                self->mExitPending = true;
                self->mRunning = false;
                self->mThread = -1;
                self->mThreadExitedCondition.notify_all();
                break;
            }
        }

        strong.clear();
        strong = weak.promote();
    } while (strong != nullptr);

    MLOGI("thread exited!!!");

    return 0;
}

void AmlMpThread::requestExit()
{
    std::lock_guard<std::mutex> _l(mLock);
    mExitPending = true;
}

int AmlMpThread::requestExitAndWait()
{
    std::unique_lock<std::mutex> _l(mLock);
    if (mThread == pthread_self()) {
        MLOGE("don't call waitForExit from this thread!");
        return -EWOULDBLOCK;
    }

    mExitPending = true;

    while (mRunning) {
        mThreadExitedCondition.wait(_l);
    }

    mExitPending = false;

    return mStatus;
}

int AmlMpThread::join()
{
    std::unique_lock<std::mutex> _l(mLock);
    if (mThread == pthread_self()) {
        MLOGE("don't call waitForExit from this thread!");
        return -EWOULDBLOCK;
    }

    while (mRunning) {
        mThreadExitedCondition.wait(_l);
    }

    return mStatus;
}

bool AmlMpThread::isRunning()
{
    std::lock_guard<std::mutex> _l(mLock);
    return mRunning;
}

int AmlMpThread::getTid() const
{
    std::lock_guard<std::mutex> _l(mLock);
    int tid;

    if (mRunning) {
        tid = mTid;
    } else {
        tid = -1;
    }

    return tid;
}

bool AmlMpThread::exitPending() const
{
    std::lock_guard<std::mutex> _l(mLock);
    return mExitPending;
}


}
