LOCAL_PATH := $(call my-dir)

LOCAL_CPP_EXTENSION := .cpp

include $(CLEAR_VARS)

LOCAL_MODULE := udt

LOCAL_SRC_FILES := md5.cpp common.cpp window.cpp list.cpp buffer.cpp packet.cpp channel.cpp queue.cpp ccc.cpp cache.cpp core.cpp api.cpp recvfile_jni.cpp

include $(BUILD_SHARED_LIBRARY)