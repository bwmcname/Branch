LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE    := native-activity
LOCAL_SRC_FILES := Android2.cpp
LOCAL_LDLIBS    := -llog -landroid -lEGL -lGLESv1_CM -lc -lstdc++
LOCAL_CFLAGS    := -std=c++11 -Wno-write-strings -Wno-attributes -DANDROID_BUILD

include $(BUILD_SHARED_LIBRARY)

$(call import-module,android/native_app_glue)
