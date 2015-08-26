LOCAL_PATH:= $(call my-dir)

LIB_TV_PATH:=$(LOCAL_PATH)/../../
DVB_PATH := $(wildcard external/dvb)
ifeq ($(DVB_PATH), )
    DVB_PATH := $(wildcard vendor/amlogic/dvb)
endif

AM_LIBPLAYER_PATH := $(wildcard vendor/amlogic/frameworks/av/LibPlayer)
LIB_SQLITE_PATH := $(wildcard external/sqlite)

include $(CLEAR_VARS)

LOCAL_MODULE_TAGS := optional

LOCAL_SRC_FILES := \
	CTvSkyworthDtmbN310.cpp

LOCAL_SHARED_LIBRARIES := \
	libtv

LOCAL_C_INCLUDES += \
	bionic/libc/include \
	bionic/libc/private \
    system/extras/ext4_utils \
	$(LIB_TV_PATH)  \
	$(LIB_TV_PATH)/tv  \
	$(LIB_TV_PATH)/tvdb \
	$(LIB_TV_PATH)/tvutils \
	$(LIB_TV_PATH)/include \
	$(DVB_PATH)/include/am_adp \
    $(DVB_PATH)/include/am_mw \
    $(DVB_PATH)/include/am_ver \
    $(DVB_PATH)/android/ndk/include \
    $(AM_LIBPLAYER_PATH)/amadec/include \
    $(AM_LIBPLAYER_PATH)/amcodec/include \
    $(AM_LIBPLAYER_PATH)/amffmpeg \
    $(AM_LIBPLAYER_PATH)/amplayer \
    $(LIB_SQLITE_PATH)/dist \
    system/media/audio_effects/include

LOCAL_LDLIBS  += -L$(SYSROOT)/usr/lib -llog

LOCAL_PRELINK_MODULE := false

LOCAL_MODULE:= libSKYWORTH_N310

include $(BUILD_STATIC_LIBRARY)
