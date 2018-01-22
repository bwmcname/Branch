LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE    := firebase_app
LOCAL_SRC_FILES := W:/firebase_cpp_sdk/libs/android/$(TARGET_ARCH_ABI)/c++/libapp.a
LOCAL_EXPORT_C_INCLUDES := W:/firebase_cpp_sdk/include/
include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE    := firebase_admob
LOCAL_SRC_FILES := W:/firebase_cpp_sdk/libs/android/$(TARGET_ARCH_ABI)/c++/libadmob.a
LOCAL_EXPORT_C_INCLUDES := W:/firebase_cpp_sdk/include/
include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE    := Branch
LOCAL_SRC_FILES := ..\..\include.cpp
LOCAL_LDLIBS    := -llog -landroid -lEGL -lGLESv3 -lc
LOCAL_CFLAGS    := -std=c++11 -Wno-write-strings -Wno-attributes -DANDROID_BUILD -I -O3
LOCAL_STATIC_LIBRARIES := firebase_admob firebase_app
include $(BUILD_SHARED_LIBRARY)

$(call import-module,android/native_app_glue)
