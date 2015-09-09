# Copyright (C) 2014 The Android Open Source Project
# Copyright (C) 2011 Amlogic
#
#

LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
DVB_PATH := $(wildcard vendor/amlogic/dvb)
TVAPI_PATH := $(LOCAL_PATH)/tvapi
LIB_SQLITE_PATH := $(wildcard external/sqlite)
AM_LIBPLAYER_PATH := $(wildcard vendor/amlogic/frameworks/av/LibPlayer)

LOCAL_MODULE_RELATIVE_PATH := hw
LOCAL_SHARED_LIBRARIES := libcutils liblog
LOCAL_SRC_FILES := tv_input.cpp
LOCAL_MODULE := tv_input.amlogic
LOCAL_MODULE_TAGS := optional
LOCAL_C_INCLUDES += \
	$(TVAPI_PATH)/libtv \
	$(TVAPI_PATH)/android	\
    $(DVB_PATH)/android/ndk/include \
    $(DVB_PATH)/include/am_adp \
    $(DVB_PATH)/include/am_mw \
    $(DVB_PATH)/include/am_ver \
    $(DVB_PATH)/android/ndk/include \
	$(LIB_SQLITE_PATH)/dist \
	 $(TVAPI_PATH)/libtv/tvdb \
    $(TVAPI_PATH)/libtv/tv \
    $(TVAPI_PATH)/libtv/include \
    $(AM_LIBPLAYER_PATH)/amadec/include \
    $(AM_LIBPLAYER_PATH)/amcodec/include \
    $(AM_LIBPLAYER_PATH)/amffmpeg \
    $(AM_LIBPLAYER_PATH)/amplayer \
     system/media/audio_effects/include \


LOCAL_SHARED_LIBRARIES += \
	libtv \
	libtvserver \
	libtv_jni
include $(BUILD_SHARED_LIBRARY)
include $(LOCAL_PATH)/tvapi/Android.mk
