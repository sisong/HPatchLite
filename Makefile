# args
MT       := 1
TUZ		   := 1
# 0: not need zlib;  1: compile zlib source code;  2: used -lz to link zlib lib;
ZLIB     := 1
LZMA     := 1
ARM64ASM := 0
STATIC_CPP := 0
# used clang?
CL  	   := 0
# build with -m32?
M32      := 0
# build for out min size
MINS     := 0
ifeq ($(OS),Windows_NT) # mingw?
  CC     := gcc
endif

HDP_PATH := HDiffPatch

HDIFFI_OBJ  :=
HPATCHI_OBJ := \
    $(HDP_PATH)/file_for_patch.o \
    $(HDP_PATH)/libHDiffPatch/HPatch/patch.o \
    $(HDP_PATH)/libHDiffPatch/HPatchLite/hpatch_lite.o

TUZ_PATH := tinyuz
ifeq ($(TUZ),1) # https://github.com/sisong/tinyuz  
  HPATCHI_OBJ +=$(TUZ_PATH)/decompress/tuz_dec.o
  HDIFFI_OBJ += $(TUZ_PATH)/compress/tuz_enc.o \
  				$(TUZ_PATH)/compress/tuz_enc_private/tuz_enc_clip.o \
  				$(TUZ_PATH)/compress/tuz_enc_private/tuz_enc_code.o \
  				$(TUZ_PATH)/compress/tuz_enc_private/tuz_enc_match.o \
  				$(TUZ_PATH)/compress/tuz_enc_private/tuz_sstring.o
endif
LZMA_PATH := lzma/C
ifeq ($(LZMA),0)
else # https://www.7-zip.org  https://github.com/sisong/lzma
  HPATCHI_OBJ +=$(LZMA_PATH)/LzmaDec.o \
  				$(LZMA_PATH)/Lzma2Dec.o 
  ifeq ($(ARM64ASM),0)
  else
  	HPATCHI_OBJ += $(LZMA_PATH)/../Asm/arm64/LzmaDecOpt.o
  endif
  HDIFFI_OBJ += $(LZMA_PATH)/LzFind.o \
  				$(LZMA_PATH)/LzFindOpt.o \
  				$(LZMA_PATH)/CpuArch.o \
				  $(LZMA_PATH)/7zStream.o \
  				$(LZMA_PATH)/LzmaEnc.o \
				  $(LZMA_PATH)/Lzma2Enc.o  
  ifeq ($(MT),0)  
  else  
    HDIFFI_OBJ+=$(LZMA_PATH)/LzFindMt.o \
  				$(LZMA_PATH)/MtCoder.o \
  				$(LZMA_PATH)/MtDec.o \
				$(LZMA_PATH)/Threads.o
  endif
endif
ZLIB_PATH := zlib
ifeq ($(ZLIB),1) # http://zlib.net  https://github.com/sisong/zlib  
  HPATCHI_OBJ +=$(ZLIB_PATH)/adler32.o \
  				$(ZLIB_PATH)/crc32.o \
  				$(ZLIB_PATH)/inffast.o \
  				$(ZLIB_PATH)/inflate.o \
  				$(ZLIB_PATH)/inftrees.o \
  				$(ZLIB_PATH)/trees.o \
  				$(ZLIB_PATH)/zutil.o
  HDIFFI_OBJ += $(ZLIB_PATH)/compress.o \
  				$(ZLIB_PATH)/deflate.o \
  				$(ZLIB_PATH)/infback.o \
  				$(ZLIB_PATH)/uncompr.o
endif

HDIFF_PATH  := $(HDP_PATH)/libHDiffPatch/HDiff
HDIFFI_OBJ += \
    hdiffi_import_patch.o \
    $(HDIFF_PATH)/diff.o \
    $(HDIFF_PATH)/match_block.o \
    $(HDIFF_PATH)/private_diff/bytes_rle.o \
    $(HDIFF_PATH)/private_diff/suffix_string.o \
    $(HDIFF_PATH)/private_diff/compress_detect.o \
    $(HDIFF_PATH)/private_diff/limit_mem_diff/digest_matcher.o \
    $(HDIFF_PATH)/private_diff/limit_mem_diff/stream_serialize.o \
    $(HDIFF_PATH)/private_diff/libdivsufsort/divsufsort64.o \
    $(HDIFF_PATH)/private_diff/libdivsufsort/divsufsort.o \
    $(HDIFF_PATH)/private_diff/limit_mem_diff/adler_roll.o \
    $(HPATCHI_OBJ)

ifeq ($(MT),0)
else
  HDIFFI_OBJ += \
    $(HDP_PATH)/libParallel/parallel_import.o \
    $(HDP_PATH)/libParallel/parallel_channel.o \
    $(HDP_PATH)/compress_parallel.o
endif


DEF_FLAGS := \
    -O3 -DNDEBUG -D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64 \
    -D_IS_NEED_DEFAULT_CompressPlugin=0
ifeq ($(M32),0)
else
  DEF_FLAGS += -m32
endif
ifeq ($(MINS),0)
else
  DEF_FLAGS += \
    -s \
    -Wno-error=format-security \
    -fvisibility=hidden  \
    -ffunction-sections -fdata-sections \
    -ffat-lto-objects -flto
  CXXFLAGS += -fvisibility-inlines-hidden
endif

ifeq ($(TUZ),0)
else
    DEF_FLAGS += -D_CompressPlugin_tuz -D_IS_USED_SHARE_hpatch_lite_types=1
	DEF_FLAGS += -I$(TUZ_PATH)/decompress -I$(TUZ_PATH)/compress -I$(HDP_PATH)/libHDiffPatch/HPatchLite
endif
ifeq ($(ZLIB),0)
else
    DEF_FLAGS += -D_CompressPlugin_zlib
    DEF_FLAGS += -I$(ZLIB_PATH)
endif
ifeq ($(LZMA),0)
else
  DEF_FLAGS += -D_CompressPlugin_lzma -D_CompressPlugin_lzma2 -I$(LZMA_PATH)
  ifeq ($(ARM64ASM),0)
  else
    DEF_FLAGS += -DZ7_LZMA_DEC_OPT
  endif
endif

ifeq ($(MT),0)
  DEF_FLAGS += \
    -DZ7_ST \
    -D_IS_USED_MULTITHREAD=0
else
  DEF_FLAGS += \
    -DZSTD_MULTITHREAD=1 \
    -D_IS_USED_MULTITHREAD=1 \
    -D_IS_USED_PTHREAD=1
endif

PATCH_LINK := 
ifeq ($(ZLIB),2)
  PATCH_LINK += -lz			# link zlib
endif
DIFF_LINK  := $(PATCH_LINK)
ifeq ($(MT),0)
else
  DIFF_LINK += -lpthread	# link pthread
endif
ifeq ($(M32),0)
else
  DIFF_LINK += -m32
endif
ifeq ($(MINS),0)
else
  DIFF_LINK += -Wl,--gc-sections,--as-needed
endif
ifeq ($(CL),1)
  CXX := clang++
  CC  := clang
endif
ifeq ($(STATIC_CPP),0)
  DIFF_LINK += -lstdc++
else
  DIFF_LINK += -static-libstdc++
endif

CFLAGS   += $(DEF_FLAGS) 
CXXFLAGS += $(DEF_FLAGS)

.PHONY: all install clean

all: libhpatchlite.a hpatchi hdiffi mostlyclean

libhpatchlite.a: $(HDIFFI_OBJ)
	$(AR) rcs $@ $^

hpatchi: $(HPATCHI_OBJ)
	$(CC) hpatchi.c $(HPATCHI_OBJ) $(CFLAGS) $(PATCH_LINK) -o hpatchi
hdiffi: libhpatchlite.a 
	$(CXX) hdiffi.cpp libhpatchlite.a $(CXXFLAGS) $(DIFF_LINK) -o hdiffi

ifeq ($(OS),Windows_NT) # mingw?
  RM := del /Q /F
  DEL_HDIFFI_OBJ := $(subst /,\,$(HDIFFI_OBJ))
else
  RM := rm -f
  DEL_HDIFFI_OBJ := $(HDIFFI_OBJ)
endif
INSTALL_X := install -m 0755
INSTALL_BIN := $(DESTDIR)/usr/local/bin

mostlyclean: hpatchi hdiffi
	$(RM) $(DEL_HDIFFI_OBJ)
clean:
	$(RM) libhpatchlite.a hpatchi hdiffi $(DEL_HDIFFI_OBJ)

install: all
	$(INSTALL_X) hdiffi $(INSTALL_BIN)/hdiffi
	$(INSTALL_X) hpatchi $(INSTALL_BIN)/hpatchi

uninstall:
	$(RM)  $(INSTALL_BIN)/hdiffi  $(INSTALL_BIN)/hpatchi
