LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_MODULE_TAGS := optional

LOCAL_SRC_FILES:= \
	CTv.cpp \

LOCAL_SHARED_LIBRARIES := \
	libcutils \
	libutils \
	libtvbinder \
	libbinder \
	libui \


LOCAL_MODULE:= libtvplay

LOCAL_C_INCLUDES += \
	$(LOCAL_PATH)/../

LOCAL_PRELINK_MODULE := false
include $(BUILD_SHARED_LIBRARY)
