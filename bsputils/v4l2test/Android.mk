LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := v4l2test

LOCAL_MODULE_TAGS := optional

LOCAL_MODULE_PATH := $(TARGET_OUT)/bin

#LOCAL_VENDOR_MODULE := true

LOCAL_SRC_FILES := \
    v4l2test.cpp \
	camdev.cpp

LOCAL_SHARED_LIBRARIES := \
    libcutils \
    libutils \
    libbinder \
    libui \
    libgui

LOCAL_STATIC_LIBRARIES := 

LOCAL_CFLAGS += \
    -D__STDC_CONSTANT_MACROS -Wno-unused-variable -Wno-unused-parameter

LOCAL_C_INCLUDES += \
		    system/core/libcutils/include	\
		    system/core/libutils/include	\
		    system/core/libbacktrace/include	\
		    system/core/liblog/include		\
		    system/core/libsystem/include	\
		    system/core/libcutils/include	\
		    out/soong/.intermediates/frameworks/native/libs/binder/libbinder/android_arm64_armv8-a_cortex-a53_core_static/gen/aidl \
		    frameworks/native/libs/binder/include	\
		    system/core/base/include     \
		    system/core/libcutils/include \
		    system/core/libutils/include   \
		    system/core/libbacktrace/include \
		    system/core/liblog/include \
		    system/core/libsystem/include \
		    system/core/base/include \
		    frameworks/native/libs/nativebase/include     \
		    hardware/libhardware/include     \
		    system/media/audio/include     \
		    system/core/libcutils/include   \
		    system/core/libsystem/include    \
		    system/bt/types     \
		    frameworks/native/libs/ui/include     \
		    frameworks/native/libs/arect/include   \
		    frameworks/native/libs/math/include     \
		    system/libhidl/base/include     \
		    system/core/libutils/include     \
		    system/core/libbacktrace/include  \
		    system/core/liblog/include		\
		    system/libhidl/transport/include	\
		    out/soong/.intermediates/system/libhidl/transport/manager/1.0/android.hidl.manager@1.0_genc++_headers/gen\
		    out/soong/.intermediates/system/libhidl/transport/manager/1.1/android.hidl.manager@1.1_genc++_headers/gen\
		    out/soong/.intermediates/system/libhidl/transport/base/1.0/android.hidl.base@1.0_genc++_headers/gen\
		    system/libhwbinder/include \
		    out/soong/.intermediates/hardware/interfaces/graphics/common/1.0/android.hardware.graphics.common@1.0_genc++_headers/gen\
		    out/soong/.intermediates/hardware/interfaces/graphics/common/1.1/android.hardware.graphics.common@1.1_genc++_headers/gen\
		    frameworks/native/libs/gui/include \
		    frameworks/native/libs/gui/include \
		    system/libhidl/base/include \
		    system/core/libcutils/include \
		    system/core/libutils/include \
		    system/core/libbacktrace/include \
		    system/core/liblog/include \
		    system/core/libsystem/include \
		    system/libhidl/transport/include \
		    out/soong/.intermediates/system/libhidl/transport/manager/1.0/android.hidl.manager@1.0_genc++_headers/gen \
		    out/soong/.intermediates/system/libhidl/transport/manager/1.1/android.hidl.manager@1.1_genc++_headers/gen \
		    out/soong/.intermediates/system/libhidl/transport/base/1.0/android.hidl.base@1.0_genc++_headers/gen \
		    system/libhwbinder/include \
		    system/core/base/include \
		    out/soong/.intermediates/hardware/interfaces/graphics/common/1.0/android.hardware.graphics.common@1.0_genc++_headers/gen \
		    out/soong/.intermediates/hardware/interfaces/graphics/common/1.1/android.hardware.graphics.common@1.1_genc++_headers/gen \
		    out/soong/.intermediates/frameworks/native/libs/binder/libbinder/android_arm64_armv8-a_cortex-a53_core_static/gen/aidl \
		    frameworks/native/libs/binder/include \
		    frameworks/native/opengl/libs/EGL/include \
		    frameworks/native/opengl/include \
		    frameworks/native/libs/nativebase/include \
		    hardware/libhardware/include \
		    system/media/audio/include \
		    system/bt/types \
		    frameworks/native/libs/ui/include \
		    frameworks/native/libs/arect/include \
		    frameworks/native/libs/math/include \
		    frameworks/native/libs/nativewindow/include \
		    frameworks/native/libs/nativewindow/include-private \
		    system/libhidl/transport/token/1.0/utils/include \
		    out/soong/.intermediates/hardware/interfaces/media/1.0/android.hardware.media@1.0_genc++_headers/gen \
		    out/soong/.intermediates/hardware/interfaces/graphics/bufferqueue/1.0/android.hardware.graphics.bufferqueue@1.0_genc++_headers/gen \
		    system/core/liblog/include \
		    external/libcxx/include \
		    external/libcxxabi/include \
		    bionic/libc/system_properties/include \
		    system/core/property_service/libpropertyinfoparser/include \
		    external/skia/include/android \
		    external/skia/include/atlastext \
		    external/skia/include/c \
		    external/skia/include/codec \
		    external/skia/include/config \
		    external/skia/include/core  \
		    external/skia/include/effects \
		    external/skia/include/encode  \
		    external/skia/include/gpu  \
		    external/skia/include/gpu/gl \
		    external/skia/include/gpu/vk \
		    external/skia/include/pathops \
		    external/skia/include/ports \
		    external/skia/include/svg \
		    external/skia/include/utils  \
		    external/skia/include/utils/mac \
		    external/skia/platform_tools/android/vulkan\
		    $(LOCAL_PATH)/ffmpeg/include\
		    vendor/nxp-opensource/imx/include

LOCAL_LDFLAGS += -ldl -llog

include $(BUILD_EXECUTABLE)








