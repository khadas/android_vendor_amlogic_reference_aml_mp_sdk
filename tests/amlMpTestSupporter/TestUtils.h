/*
 * Copyright (c) 2020 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */

#ifndef _AML_MP_TEST_UTILS_H_
#define _AML_MP_TEST_UTILS_H_

#include <gui/Surface.h>
#include <gui/SurfaceComposerClient.h>
#include <ui/DisplayInfo.h>
#include <string>
#include <utils/RefBase.h>

namespace aml_mp {
using android::sp;

struct NativeUI : android::RefBase
{
    NativeUI();
    ~NativeUI();
    sp<ANativeWindow> getNativeWindow() const;
    void controlSurface(int zorder);
    void controlSurface(int left, int top, int right, int bottom);
    int getSurfaceWidth();
    int getSurfaceHeight();

private:
    sp<android::SurfaceComposerClient> mComposerClient;
    sp<android::SurfaceControl> mSurfaceControl;
    sp<android::Surface> mSurface;

    sp<android::SurfaceControl> mSurfaceControlUi;
    sp<android::Surface> mSurfaceUi;

    int mDisplayWidth = 1920;
    int mDisplayHeight = 1080;

    int mSurfaceWidth = 1920;
    int mSurfaceHeight = 1080;

    static const int UI_LAYER = 1;

    NativeUI(const NativeUI&) = delete;
    NativeUI& operator=(const NativeUI&);
};

struct CommandProcessor : android::RefBase
{
    using Visitor = bool(const std::vector<std::string>& args);
    using Interrupter = bool();

    CommandProcessor(const std::string& prompt = ">");
    ~CommandProcessor();
    int setCommandVisitor(const std::function<Visitor>& visitor);
    int setInterrupter(const std::function<Interrupter>& interrupter);
    int fetchAndProcessCommands();

private:
    bool isDelimiter(const std::string& delimiters, char c) {
        return delimiters.find(c) != std::string::npos;
    }

    std::vector<std::string> split(const std::string& str);

private:
    std::function<Visitor> mCommandVisitor;
    std::function<Interrupter> mInterrupter;
    std::string mPrompt;

    CommandProcessor(const CommandProcessor&) = delete;
    CommandProcessor& operator=(const CommandProcessor&) = delete;
};


}













#endif