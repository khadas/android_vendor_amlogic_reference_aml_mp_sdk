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

#ifndef AML_MP_THREAD_H_
#define AML_MP_THREAD_H_

#include <mutex>
#include <condition_variable>
#include "AmlMpRefBase.h"

namespace aml_mp {
class AmlMpThread : virtual public AmlMpRefBase
{
public:
    AmlMpThread();
    virtual ~AmlMpThread();
    virtual int run(const char* name);
    virtual void requestExit();
    virtual int readyToRun();
    int requestExitAndWait();
    int join();
    bool isRunning();
    int getTid() const;

protected:
    bool exitPending() const;

private:
    virtual bool threadLoop() = 0;
    static int _threadLoop(void* user);
    mutable std::mutex mLock;
    std::condition_variable mThreadExitedCondition;
    int mStatus;
    volatile bool mExitPending;
    volatile bool mRunning;
    sptr<AmlMpThread> mHoldSelf;
    pthread_t mThread;
    int mTid;

private:
    AmlMpThread(const AmlMpThread&) = delete;
    AmlMpThread& operator= (const AmlMpThread&) = delete;
};
}


#endif
