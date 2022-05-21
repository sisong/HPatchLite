//hdiffi_import_patch.h
// import hpatchi cmd line to hdiffi
/*
 The MIT License (MIT)
 Copyright (c) 2020-2022 HouSisong All Rights Reserved.
 */
#ifndef hdiffi_import_patch_h
#define hdiffi_import_patch_h
#include <stdlib.h>
#include "HDiffPatch/libHDiffPatch/HPatchLite/hpatch_lite.h"
#ifdef __cplusplus
extern "C" {
#endif
#define HPATCHLITE_VERSION_MAJOR    0
#define HPATCHLITE_VERSION_MINOR    4
#define HPATCHLITE_VERSION_RELEASE  0

#define _HPATCHLITE_VERSION          HPATCHLITE_VERSION_MAJOR.HPATCHLITE_VERSION_MINOR.HPATCHLITE_VERSION_RELEASE
#define _HDIFFPATCH_QUOTE(str) #str
#define _HDIFFPATCH_EXPAND_AND_QUOTE(str) _HDIFFPATCH_QUOTE(str)
#define HPATCHLITE_VERSION_STRING   _HDIFFPATCH_EXPAND_AND_QUOTE(_HPATCHLITE_VERSION)

int isSwapToPatchMode(int argc,const char* argv[]);
int hpatchi_cmd_line(int argc,const char* argv[]);

int hpatchi_patch(hpatchi_listener_t* listener,hpi_compressType compress_type,hpi_pos_t newSize,
                  hpi_pos_t uncompressSize,size_t patchCacheSize);

#ifdef __cplusplus
}
#endif
#endif
