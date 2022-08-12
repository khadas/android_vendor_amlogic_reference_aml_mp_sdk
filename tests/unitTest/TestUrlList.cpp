/*
 * Copyright (c) 2020 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */

#define LOG_TAG "TestUrlList"
#include <utils/AmlMpLog.h>
#include "TestUrlList.h"
#include <dirent.h>
#include <unistd.h>
#include <fstream>

static const char* mName = LOG_TAG;

namespace aml_mp {
TestUrlList::TestUrlList()
{
    MLOG();
}

void TestUrlList::initSourceDir(const std::string& sourceDir)
{
    if (sourceDir.empty()) {
        return;
    }
    mSourceDir = sourceDir;
    if (*mSourceDir.rbegin() != '/') {
        mSourceDir.append("/");
    }
}

void TestUrlList::initSourceUrl(const std::string& sourceUrl)
{
    mSourceUrl = sourceUrl;
}

bool TestUrlList::getUrl(const std::string& testName __unused, std::string* url)
{
    if (mSourceUrl.empty()) {
        return false;
    }
    *url = mSourceUrl;
    return true;
}

bool TestUrlList::getUrls(const std::string& testName, std::list<std::string>* results)
{
    if (results == nullptr) return false;
    std::string subDir = mapToDirectoryName(testName);
    std::string dir = mSourceDir + subDir;

    auto it = mFileListCache.find(dir);
    if (it != mFileListCache.end()) {
        *results = it->second;
        return true;
    } else if (collectFileList(dir, results)) {
        mFileListCache.emplace(dir, *results);
        return true;
    }

    return false;
}

std::string TestUrlList::mapToDirectoryName(const std::string& testName)
{
    std::string subdir = "";
    const std::map<std::string, std::string> dirTable = {
        {"subtitleTest", "subtitle/"},
    };

    auto it = dirTable.find(testName);
    if (it != dirTable.end()) {
        subdir = it->second;
    }

    return subdir;
}

bool TestUrlList::collectFileList(const std::string& dir, std::list<std::string>* fileList)
{
    DIR* dirHandle;
    dirHandle = opendir(dir.c_str());
    if (dirHandle == nullptr) {
        printf("open %s failed!\n", dir.c_str());
        return false;
    }

    struct dirent* ent;
    while ((ent = readdir(dirHandle)) != nullptr) {
        std::string file = ent->d_name;
        if (file == "." || file == "..") {
            continue;
        }

        if (ent->d_type != DT_REG) {
            continue;
        }

        auto suffix = file.find_last_of(".");
        if (suffix == std::string::npos) {
            continue;
        }

        if (file.substr(suffix+1) != "ts") {
            continue;
        }

        std::string fullPath = dir + file;
        printf("file: %s\n", fullPath.c_str());
        fileList->push_back(std::move(fullPath));
    }

    closedir(dirHandle);

    return !fileList->empty();
}

}
