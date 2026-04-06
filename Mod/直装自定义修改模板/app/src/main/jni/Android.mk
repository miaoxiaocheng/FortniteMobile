LOCAL_PATH := $(call my-dir)
MAIN_LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_MODULE  := Syxy


LOCAL_ARM_MODE := arm
LOCAL_CFLAGS := -Wno-error=format-security -fpermissive -fvisibility=hidden -fvisibility-inlines-hidden
LOCAL_CFLAGS += -fno-rtti -fno-exceptions -g0 -fomit-frame-pointer -ffunction-sections -fdata-sections
LOCAL_CPPFLAGS += -fvisibility=hidden -ffunction-sections -fdata-sections
LOCAL_LDFLAGS += -Wl,--strip-all

# Here you add the cpp file
LOCAL_C_INCLUDES += $(MAIN_LOCAL_PATH)
LOCAL_SRC_FILES := src/main.cpp \
	
	
LOCAL_LDLIBS := -llog -landroid

include $(BUILD_SHARED_LIBRARY)
