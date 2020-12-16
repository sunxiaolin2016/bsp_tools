LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := mii-tool

LOCAL_MODULE_TAGS := optional

LOCAL_MODULE_PATH := $(TARGET_OUT)/bin

LOCAL_VENDOR_MODULE := true

LOCAL_SRC_FILES := \
	mii-tool.c

LOCAL_SHARED_LIBRARIES := 

LOCAL_STATIC_LIBRARIES :=

LOCAL_CFLAGS += \
    -D__STDC_CONSTANT_MACROS -Wno-unused-variable -Wno-unused-parameter

LOCAL_C_INCLUDES +=

LOCAL_LDFLAGS += -ldl 

include $(BUILD_EXECUTABLE)








