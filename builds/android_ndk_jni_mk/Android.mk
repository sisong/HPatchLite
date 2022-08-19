LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_MODULE := hpatchi

# args
LZMA  := 0
TUZ   := 1
ZLIB  := 1   # -lz dynamic link unsafe for inflate_state

ifeq ($(LZMA),0)
  Lzma_Files :=
else
  # https://github.com/sisong/lzma
  LZMA_PATH  := $(LOCAL_PATH)/../../lzma/C/
  Lzma_Files := $(LZMA_PATH)/LzmaDec.c  \
                $(LZMA_PATH)/Lzma2Dec.c
 ifeq ($(TARGET_ARCH_ABI),arm64-v8a)
  Lzma_Files += $(LZMA_PATH)/../Asm/arm64/LzmaDecOpt.S
 endif
endif
ifeq ($(TUZ),0)
  Tuz_Files :=
else
  # https://github.com/sisong/tinyuz
  TUZ_PATH  := $(LOCAL_PATH)/../../tinyuz/decompress/
  Tuz_Files := $(TUZ_PATH)/tuz_dec.c
endif
ifeq ($(ZLIB),0)
else
  # http://zlib.net/  https://github.com/madler/zlib
  ZLIB_PATH  := $(LOCAL_PATH)/../../zlib/
endif

HDP_PATH  := $(LOCAL_PATH)/../../HDiffPatch
Hdp_Files := $(HDP_PATH)/file_for_patch.c \
             $(HDP_PATH)/libHDiffPatch/HPatchLite/hpatch_lite.c

Src_Files := $(LOCAL_PATH)/hpatchi_jni.c \
             $(LOCAL_PATH)/hpatchi.c

DEF_FLAGS := -Os -D_IS_NEED_DEFAULT_CompressPlugin=0
ifeq ($(LZMA),0)
else
  DEF_FLAGS += -D_CompressPlugin_lzma -D_CompressPlugin_lzma2 -D_7ZIP_ST -I$(LZMA_PATH)
  ifeq ($(TARGET_ARCH_ABI),arm64-v8a)
    DEF_FLAGS += -D_LZMA_DEC_OPT
  endif
endif
ifeq ($(TUZ),0)
else
  DEF_FLAGS += -D_CompressPlugin_tuz -I$(TUZ_PATH)
endif
ifeq ($(ZLIB),0)
else
  DEF_FLAGS += -D_CompressPlugin_zlib -I$(ZLIB_PATH)
endif

LOCAL_SRC_FILES  := $(Src_Files) $(Lzma_Files) $(Tuz_Files) $(Hdp_Files)
LOCAL_LDLIBS     := -llog
ifeq ($(ZLIB),0)
else
  LOCAL_LDLIBS += -lz
endif
LOCAL_CFLAGS     := -DANDROID_NDK -DNDEBUG $(DEF_FLAGS)
include $(BUILD_SHARED_LIBRARY)

