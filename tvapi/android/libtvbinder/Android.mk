LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_MODULE_TAGS := optional

LOCAL_SRC_FILES:= \
	TvClient.cpp \
	ITv.cpp \
	ITvClient.cpp \
	ITvService.cpp

LOCAL_SHARED_LIBRARIES := \
	libcutils \
	libutils \
	libbinder \
	libui

LOCAL_MODULE:= libtvbinder

LOCAL_C_INCLUDES += \
	$(LOCAL_PATH)/../

LOCAL_PRELINK_MODULE := false
include $(BUILD_SHARED_LIBRARY)
