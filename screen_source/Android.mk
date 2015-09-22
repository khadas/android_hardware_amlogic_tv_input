# Copyright (C) 2013 Amlogic
#
#

LOCAL_PATH := $(call my-dir)

# HAL module implemenation, not prelinked and stored in
# /system/lib/hw/screen_source.amlogic.so
include $(CLEAR_VARS)
LOCAL_MODULE_RELATIVE_PATH := hw
LOCAL_SRC_FILES := aml_screen.cpp v4l2_vdin.cpp

ifneq (,$(wildcard hardware/amlogic/gralloc))
	GRALLOC_DIR := hardware/amlogic/gralloc
else
	GRALLOC_DIR := hardware/libhardware/modules/gralloc
endif

LOCAL_C_INCLUDES += frameworks/native/include/utils \
					frameworks/native/include/android \
					system/core/include/utils \
					system/core/libion/include \
					system/core/libion/kernel-headers \
					$(GRALLOC_DIR)

LOCAL_SHARED_LIBRARIES:= libutils liblog libui

LOCAL_MODULE := screen_source.amlogic
LOCAL_CFLAGS:= -DLOG_TAG=\"screen_source\"
LOCAL_MODULE_TAGS := optional
include $(BUILD_SHARED_LIBRARY)
