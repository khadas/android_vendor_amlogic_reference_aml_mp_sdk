/*
 * Copyright (c) 2020 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */



#define LOG_TAG "AmlMpSysfsUtil"

#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <strings.h>
#include <utils/AmlMpLog.h>
#include <sys/ioctl.h>
#include "Amlsysfsutils.h"
#include <unistd.h>
#include <cutils/properties.h>
#include <string.h>

static const char* mName = LOG_TAG;

namespace aml_mp {

int amsysfs_set_sysfs_str(const char *path, const char *val) {
    int fd;
    int bytes;
    fd = open(path, O_CREAT | O_RDWR | O_TRUNC, 0644);
    if (fd >= 0) {
        bytes = write(fd, val, strlen(val));
        close(fd);
        return 0;
    } else {
        MLOGW("[%s] %s failed!",__FUNCTION__,path);
        return -1;
    }
}
int amsysfs_get_sysfs_str(const char *path, char *valstr, unsigned size) {
    int fd;
    int bytes;
    fd = open(path, O_RDONLY);
    if (fd >= 0) {
        memset(valstr, 0, size);
        bytes = read(fd, valstr, size - 1);
        valstr[size - 1] = '\0';
        close(fd);
        return 0;
    } else {
        MLOGW("[%s] %s failed!",__FUNCTION__,path);
        sprintf(valstr, "%s", "fail");
        return -1;
    }
}

int amsysfs_set_sysfs_int(const char *path, int val) {
    int fd;
    int bytes;
    char bcmd[16];
    fd = open(path, O_CREAT | O_RDWR | O_TRUNC, 0644);
    if (fd >= 0) {
        sprintf(bcmd, "%d", val);
        bytes = write(fd, bcmd, strlen(bcmd));
        close(fd);
        return 0;
    } else {
        sprintf(bcmd, "%d", val);
        MLOGW("[%s] %s failed!",__FUNCTION__,path);
        return -1;
    }
}

int amsysfs_get_sysfs_int(const char *path) {
    int fd;
    int bytes;
    int val = 0;
    char bcmd[16];
    fd = open(path, O_RDONLY);
    if (fd >= 0) {
        bytes = read(fd, bcmd, sizeof(bcmd));
        val = strtol(bcmd, NULL, 10);
        close(fd);
    } else {
        MLOGW("[%s] %s failed!",__FUNCTION__,path);
    }
    return val;
}

int amsysfs_set_sysfs_int16(const char *path, int val) {
    int fd;
    int bytes;
    char bcmd[16];
    fd = open(path, O_CREAT | O_RDWR | O_TRUNC, 0644);
    if (fd >= 0) {
        sprintf(bcmd, "0x%x", val);
        bytes = write(fd, bcmd, strlen(bcmd));
        close(fd);
        return 0;
    } else {
        MLOGW("[%s] %s failed!",__FUNCTION__,path);
        return -1;
    }
}

int amsysfs_get_sysfs_int16(const char *path) {
    int fd;
    int bytes;
    int val = 0;
    char bcmd[16];
    fd = open(path, O_RDONLY);
    if (fd >= 0) {
        bytes = read(fd, bcmd, sizeof(bcmd));
        val = strtol(bcmd, NULL, 16);
        close(fd);
    } else {
        MLOGW("[%s] %s failed!",__FUNCTION__,path);
    }
    return val;
}

unsigned long amsysfs_get_sysfs_ulong(const char *path) {
    int fd;
    int bytes;
    char bcmd[24] = "";
    unsigned long num = 0;
    if ((fd = open(path, O_RDONLY)) >= 0) {
        bytes = read(fd, bcmd, sizeof(bcmd));
        num = strtoul(bcmd, NULL, 0);
        close(fd);
    } else {
        MLOGW("[%s] %s failed!",__FUNCTION__,path);
    }
    return num;
}

}
