/*
 * Copyright (c) 2021 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */

#ifndef _AML_MP_SIGNAL_HANDLER_H_
#define _AML_MP_SIGNAL_HANDLER_H_

#include <signal.h>
#include <ostream>
#include <map>

namespace aml_mp {
class AmlMpSignalHandler
{
public:
    static AmlMpSignalHandler& instance() {
        static AmlMpSignalHandler ins;
        return ins;
    }

    int installSignalHandlers();

    static void dumpstack(std::ostream& os);

private:
    AmlMpSignalHandler() = default;
    ~AmlMpSignalHandler() = default;

    static void unexpectedSignalHandler(int signal_number, siginfo_t* info, void*);
    static const char* getSignalName(int signal_number);
    static const char* getSignalCodeName(int signal_number, int signal_code);
    static std::string demangleSymbol(const char* symbol);

    std::map<int, void(*)(int)> mOldSignalHandlers;

private:
    AmlMpSignalHandler(const AmlMpSignalHandler&) = delete;
    AmlMpSignalHandler& operator=(const AmlMpSignalHandler&) = delete;
};



}









#endif
