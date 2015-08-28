#	dns/cstr.c dns/client.c dns/dname.c dns/hdr.c dns/ns.c dns/res.c dns/rr.c dns/rrlist.c \

LOCAL_PATH:= $(call my-dir)

RE_SRC_FILES:=  \
	fmt/str.c fmt/str_error.c fmt/time.c fmt/prm.c fmt/regex.c fmt/pl.c fmt/print.c fmt/ch.c fmt/hexdump.c\
	mbuf/mbuf.c \
	list/list.c \
	sa/sa.c sa/pton.c sa/ntop.c sa/printaddr.c \
	base64/b64.c \
	dbg/dbg.c\
	sys/sys.c sys/sleep.c sys/rand.c sys/endian.c sys/daemon.c sys/fs.c\
	conf/conf.c \
	crc32/crc32.c \
	hash/func.c hash/hash.c\
	sha/sha1.c\
	hmac/hmac_sha1.c\
	md5/md5.c md5/wrap.c\
	mem/mem.c\
	mod/dl.c mod/mod.c\
	lock/lock.c\
	tmr/tmr.c\
	net/if.c net/net.c net/netstr.c net/rt.c net/sock.c net/sockopt.c net/posix/pif.c \
	main/init.c main/main.c main/method.c\
	stun/addr.c stun/attr.c stun/ctrans.c stun/hdr.c stun/ind.c stun/keepalive.c stun/msg.c stun/rep.c stun/req.c stun/stun.c stun/stunstr.c\
	turn/chan.c turn/perm.c turn/turnc.c \
	ice/cand.c ice/candpair.c ice/chklist.c ice/comp.c ice/connchk.c ice/gather.c ice/ice.c ice/icem.c ice/icesdp.c ice/icestr.c ice/stunsrv.c ice/util.c\
	tcp/tcp.c tcp/tcp_high.c \
	udp/udp.c udp/mcast.c \
	p2p/p2p.c

RE_C_INCLUDES += $(LOCAL_PATH)/include

UDT_SRC_FILES:= \
	udt/common.cpp \
	udt/md5.cpp \
	udt/window.cpp \
	udt/losslist.cpp \
	udt/buffer.cpp \
	udt/packet.cpp \
	udt/channel.cpp \
	udt/queue.cpp \
	udt/ccc.cpp \
	udt/cache.cpp \
	udt/core.cpp \
	udt/epoll.cpp \
	udt/api.cpp \
	jniapi.cpp
	
UDT_C_INCLUDES += $(LOCAL_PATH)/udt


# static library
# =====================================================
#
include $(CLEAR_VARS)
LOCAL_MODULE := libre
LOCAL_SRC_FILES:= $(RE_SRC_FILES)
LOCAL_C_INCLUDES:= $(RE_C_INCLUDES)
LOCAL_CFLAGS  += -DHAVE_SELECT -DHAVE_SELECT_H -fPIC -DLINUX -g
#LOCAL_PRELINK_MODULE:= false
include $(BUILD_STATIC_LIBRARY)

## dynamic library
## =====================================================
#
include $(CLEAR_VARS)
#LOCAL_CPP_EXTENSION := .cpp
LOCAL_MODULE := libp2p
LOCAL_SRC_FILES:= $(UDT_SRC_FILES)
LOCAL_C_INCLUDES:= $(RE_C_INCLUDES) $(UDT_C_INCLUDES)
LOCAL_STATIC_LIBRARIES := libre
LOCAL_LDLIBS += -L$(SYSROOT)/usr/lib -llog
LOCAL_CFLAGS += -DHAVE_STDMAXMIN
#LOCAL_PRELINK_MODULE:= false
include $(BUILD_SHARED_LIBRARY)

