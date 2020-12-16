LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)
LOCAL_SRC_FILES:= gpio_config.c
LOCAL_CFLAGS:=-O2 -g
LOCAL_MODULE_TAGS := optional
LOCAL_LDFLAGS += -Wl,--no-fatal-warnings
LOCAL_MODULE_PATH := $(TARGET_OUT_VENDOR_EXECUTABLES)
LOCAL_MODULE := gpio_config
LOCAL_SHARED_LIBRARIES := liblog libcutils
LOCAL_LDLIBS    := -llog -lcutils
include $(BUILD_EXECUTABLE)
