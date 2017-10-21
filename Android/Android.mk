LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE    := Branch
LOCAL_SRC_FILES := ..\include.cpp
LOCAL_LDLIBS    := -llog -landroid -lEGL -lGLESv3 -lc
LOCAL_CFLAGS    := -std=c++11 -Wno-write-strings -Wno-attributes -DANDROID_BUILD -DDEBUG

include $(BUILD_SHARED_LIBRARY)

$(call import-module,android/native_app_glue)
