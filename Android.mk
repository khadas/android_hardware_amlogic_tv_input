# Copyright (C) 2014 The Android Open Source Project
# Copyright (C) 2011 Amlogic
#
#

LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
DVB_PATH := $(wildcard vendor/amlogic/dvb)
TVAPI_PATH := vendor/amlogic/frameworks/libtvbinder/include
LIB_SQLITE_PATH := $(wildcard external/sqlite)
AM_LIBPLAYER_PATH := $(wildcard vendor/amlogic/frameworks/av/LibPlayer)


ifneq (,$(wildcard hardware/amlogic/gralloc))
	GRALLOC_DIR := hardware/amlogic/gralloc
else
	GRALLOC_DIR := hardware/libhardware/modules/gralloc
endif

LOCAL_MODULE_RELATIVE_PATH := hw
LOCAL_SHARED_LIBRARIES := libcutils libutils libtvbinder libbinder libui liblog libhardware
LOCAL_REQUIRED_MODULES := libtvbinder

LOCAL_SRC_FILES := tv_input.cpp \
		TvPlay.cpp

LOCAL_MODULE := tv_input.amlogic
LOCAL_MODULE_TAGS := optional
LOCAL_C_INCLUDES += \
	$(TVAPI_PATH)	\
	$(DVB_PATH)/android/ndk/include \
	$(DVB_PATH)/include/am_adp \
	$(DVB_PATH)/include/am_mw \
	$(DVB_PATH)/include/am_ver \
	$(DVB_PATH)/android/ndk/include \
	$(LIB_SQLITE_PATH)/dist \
	$(AM_LIBPLAYER_PATH)/amadec/include \
	$(AM_LIBPLAYER_PATH)/amcodec/include \
	$(AM_LIBPLAYER_PATH)/amffmpeg \
	$(AM_LIBPLAYER_PATH)/amplayer \
	system/media/audio_effects/include \
	system/core/libion/include \
	system/core/libion/kernel-headers \
	$(GRALLOC_DIR)

LOCAL_C_INCLUDES += hardware/amlogic/audio/libTVaudio

include $(BUILD_SHARED_LIBRARY)
