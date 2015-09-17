LOCAL_PATH:= $(call my-dir)


DVB_PATH := $(wildcard external/dvb)

ifeq ($(DVB_PATH), )
    DVB_PATH := $(wildcard vendor/amlogic/dvb)
endif

AM_LIBPLAYER_PATH := $(wildcard vendor/amlogic/frameworks/av/LibPlayer)
LIB_ZVBI_PATH := $(wildcard external/libzvbi)
LIB_SQLITE_PATH := $(wildcard external/sqlite)


#tvserver
include $(CLEAR_VARS)



LOCAL_MODULE_TAGS := optional

LOCAL_SRC_FILES:= \
    TvService.cpp

LOCAL_SHARED_LIBRARIES := \
    libtv

LOCAL_SHARED_LIBRARIES += \
    libui \
    libutils \
    libbinder \
    libcutils \
    libsqlite \
    libmedia \
    libtvbinder \
    libhardware_legacy \
    libdl \
    libskia \
    libtinyxml

LOCAL_SHARED_LIBRARIES += \
    libzvbi \
    libntsc_decode \
    libam_mw \
    libam_adp \
    libam_ver

LOCAL_C_INCLUDES := \
    $(LOCAL_PATH)/../../libtv \
    $(LOCAL_PATH)/../ \
    $(LOCAL_PATH)/../../libtv/tvdb \
    $(LOCAL_PATH)/../../libtv/tv \
    $(LOCAL_PATH)/../../libtv/include \
    $(LOCAL_PATH)/../../build/include \

LOCAL_C_INCLUDES += \
    bionic/libc/include \
	bionic/libc/private \
    system/extras/ext4_utils \
    $(LOCAL_PATH)/../ \
    $(LIB_SQLITE_PATH)/dist \
    system/media/audio_effects/include


LOCAL_C_INCLUDES += \
    $(LIB_ZVBI_PATH)/ntsc_decode/include \
    $(LIB_ZVBI_PATH)/ntsc_decode/include/ntsc_dmx \
    $(LIB_ZVBI_PATH)/src \
    $(DVB_PATH)/include/am_adp \
    $(DVB_PATH)/include/am_mw \
    $(DVB_PATH)/include/am_ver \
    $(DVB_PATH)/android/ndk/include \
    $(AM_LIBPLAYER_PATH)/amadec/include \
    $(AM_LIBPLAYER_PATH)/amcodec/include \
    $(AM_LIBPLAYER_PATH)/amffmpeg \
    $(AM_LIBPLAYER_PATH)/amplayer


LOCAL_MODULE:= libtvserver

include $(BUILD_SHARED_LIBRARY)
