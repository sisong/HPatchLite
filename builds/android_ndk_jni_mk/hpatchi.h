// hpatchi.h
// Created by sisong on 2022-08-19.
#ifndef hpatchi_h
#define hpatchi_h
#include <assert.h>
#include <string.h>
#ifdef __cplusplus
extern "C" {
#endif
    #define H_PATCH_EXPORT __attribute__((visibility("default")))
    
    // return THPatchiResult, 0 is ok
    //  'diffFileName' file is create by hdiffi app,or by create_lite_diff()
    int hpatchi(const char *oldFileName,const char *diffFileName,
                const char *outNewFileName, size_t cacheMemory) H_PATCH_EXPORT;

#ifdef __cplusplus
}
#endif
#endif // hpatchi_h
