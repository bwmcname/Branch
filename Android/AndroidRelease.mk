LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE    := firebase_app
LOCAL_LDLIBS    := -latomic
LOCAL_SRC_FILES := ../libs/firebase/$(TARGET_ARCH_ABI)/c++/libapp.a
include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE    := firebase_admob
LOCAL_LDLIBS    := -latomic
LOCAL_SRC_FILES := ../libs/firebase/$(TARGET_ARCH_ABI)/c++/libadmob.a
include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE    := Branch
LOCAL_SRC_FILES := ..\include.cpp
LOCAL_LDLIBS    := -landroid -lEGL -lGLESv3 -lc -llog -latomic
LOCAL_CFLAGS    := -std=c++11 -Wno-write-strings -Wno-attributes -Wswitch -DANDROID_BUILD -O3 -I ../libs
LOCAL_STATIC_LIBRARIES := firebase_admob firebase_app
include $(BUILD_SHARED_LIBRARY)


$(call import-module,android/native_app_glue)
