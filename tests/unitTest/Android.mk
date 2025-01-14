LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_MODULE := amlMpUnitTest
LOCAL_LICENSE_KINDS := SPDX-license-identifier-Apache-2.0 SPDX-license-identifier-MIT legacy_by_exception_only legacy_notice
LOCAL_LICENSE_CONDITIONS := by_exception_only notice restricted
LOCAL_NOTICE_FILE := $(LOCAL_PATH)/../../LICENSE
LOCAL_MODULE_TAGS := optional
LOCAL_SRC_FILES := \
    TestUrlList.cpp \
    AmlMpTest.cpp \
    AmlMpPlayerVideoTest.cpp \
    AmlMpPlayerAudioTest.cpp \
    AmlMpPlayerSubtitleTest.cpp\
    AmlMpPlayerTest.cpp \
    AmlMpPlayerProbeTest.cpp \
    AmlMpDvrRecorderTest.cpp \
    AmlMpDvrRecorderProbeTest.cpp \
    AmlMpDvrPlayerTest.cpp \
    AmlMpDvrPlayerVideoTest.cpp \
    AmlMpDvrPlayerAudioTest.cpp \
    AmlMpMultiThreadTest.cpp \

LOCAL_CFLAGS := -DANDROID_PLATFORM_SDK_VERSION=$(PLATFORM_SDK_VERSION) \
	-Werror -Wsign-compare
LOCAL_C_INCLUDES :=
LOCAL_SHARED_LIBRARIES := libutils \
    libcutils \
    liblog \
	libui \
	libstagefright_foundation \

LOCAL_STATIC_LIBRARIES := libamlMpTestSupporter libgtest

LOCAL_SHARED_LIBRARIES += \
    libaml_mp_sdk \
	libamdvr.system \
    libgui

ifeq (1, $(shell expr $(PLATFORM_SDK_VERSION) \>= 30))
LOCAL_SYSTEM_EXT_MODULE := true
endif

include $(BUILD_EXECUTABLE)
