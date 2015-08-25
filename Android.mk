# Copyright (C) 2014 The Android Open Source Project
# Copyright (C) 2011 Amlogic
#
#

LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE_RELATIVE_PATH := hw
LOCAL_SHARED_LIBRARIES := libcutils liblog
LOCAL_SRC_FILES := tv_input.cpp
LOCAL_MODULE := tv_input.amlogic
LOCAL_MODULE_TAGS := optional
include $(BUILD_SHARED_LIBRARY)
