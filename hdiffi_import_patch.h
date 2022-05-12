//hdiffi_import_patch.h
// import hpatchi cmd line to hdiffi
/*
 The MIT License (MIT)
 Copyright (c) 2020-2022 HouSisong All Rights Reserved.
 */
#ifndef hdiffi_import_patch_h
#define hdiffi_import_patch_h
#ifdef __cplusplus
extern "C" {
#endif

int isSwapToPatchMode(int argc,const char* argv[]);
int hpatchi_cmd_line(int argc,const char* argv[]);

#ifdef __cplusplus
}
#endif
#endif
