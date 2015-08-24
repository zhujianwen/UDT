LOCAL_PATH := $(call my-dir)

LOCAL_CPP_EXTENSION := .cpp

include $(CLEAR_VARS)

LOCAL_MODULE := udt

LOCAL_SRC_FILES := JNIMain.cpp JNICore.cpp md5.cpp common.cpp window.cpp list.cpp buffer.cpp packet.cpp channel.cpp queue.cpp ccc.cpp cache.cpp core.cpp epoll.cpp api.cpp UdtCore.cpp

include $(BUILD_SHARED_LIBRARY)