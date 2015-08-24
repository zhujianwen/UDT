LOCAL_PATH := $(call my-dir)

UDT_SRC_DIR := ../src

UDT_SRC_FILES := \
	$(UDT_SRC_DIR)/api.cpp \
	$(UDT_SRC_DIR)/buffer.cpp \
	$(UDT_SRC_DIR)/cache.cpp \
	$(UDT_SRC_DIR)/ccc.cpp \
	$(UDT_SRC_DIR)/channel.cpp \
	$(UDT_SRC_DIR)/common.cpp \
	$(UDT_SRC_DIR)/core.cpp \
	$(UDT_SRC_DIR)/epoll.cpp \
	$(UDT_SRC_DIR)/list.cpp \
	$(UDT_SRC_DIR)/md5.cpp \
	$(UDT_SRC_DIR)/packet.cpp \
	$(UDT_SRC_DIR)/queue.cpp \
	$(UDT_SRC_DIR)/window.cpp


TURBO_SRC_DIR := ../app

TURBO_SRC_FILES := \
	$(TURBO_SRC_DIR)/threads.cpp \
	$(TURBO_SRC_DIR)/threadsposix.cpp \
	$(TURBO_SRC_DIR)/threadtask.cpp \
	$(TURBO_SRC_DIR)/times.cpp \
	$(TURBO_SRC_DIR)/transfer.cpp \
	transfer-jni.cpp


# static library
# =====================================================
#
include $(CLEAR_VARS)
LOCAL_CPP_EXTENSION := .cpp
LOCAL_MODULE := udt
LOCAL_SRC_FILES:= $(UDT_SRC_FILES)
LOCAL_C_INCLUDES:= $(UDT_SRC_DIR)
LOCAL_CFLAGS  += -DLINUX
#LOCAL_PRELINK_MODULE:= false
include $(BUILD_STATIC_LIBRARY)


## dynamic library
## =====================================================
#
include $(CLEAR_VARS)
LOCAL_CPP_EXTENSION := .cpp
LOCAL_MODULE := turbo
LOCAL_SRC_FILES:= $(TURBO_SRC_FILES)
LOCAL_C_INCLUDES:= $(UDT_SRC_DIR) $(TURBO_SRC_FILES)
LOCAL_STATIC_LIBRARIES := udt
LOCAL_LDLIBS += -llog
LOCAL_CFLAGS += -DLINUX
#LOCAL_PRELINK_MODULE:= false
include $(BUILD_SHARED_LIBRARY)