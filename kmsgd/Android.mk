# Copyright 2006 The Android Open Source Project

LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES:= kmsgd.cpp 

LOCAL_SHARED_LIBRARIES := liblog

LOCAL_MODULE:= kmsgd

include $(BUILD_EXECUTABLE)
