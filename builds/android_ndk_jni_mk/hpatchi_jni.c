// hpatch_jni.c
// Created by sisong on 2022-08-19.
#include <jni.h>
#include "hpatchi.h"
#ifdef __cplusplus
extern "C" {
#endif
    
    JNIEXPORT int
    Java_com_github_sisong_hpatchi_patch(JNIEnv* jenv,jobject jobj,
                                         jstring oldFileName,jstring diffFileName,
                                         jstring outNewFileName,jlong cacheMemory){
        const char* cOldFileName    = (*jenv)->GetStringUTFChars(jenv,oldFileName, NULL);
        const char* cDiffFileName  = (*jenv)->GetStringUTFChars(jenv,diffFileName, NULL);
        const char* cOutNewFileName = (*jenv)->GetStringUTFChars(jenv,outNewFileName, NULL);
        size_t cCacheMemory=(size_t)cacheMemory;
        assert((jlong)cCacheMemory==cacheMemory);
        int result=hpatchi(cOldFileName,cDiffFileName,cOutNewFileName,cCacheMemory);
        (*jenv)->ReleaseStringUTFChars(jenv,outNewFileName,cOutNewFileName);
        (*jenv)->ReleaseStringUTFChars(jenv,diffFileName,cDiffFileName);
        (*jenv)->ReleaseStringUTFChars(jenv,oldFileName,cOldFileName);
        return result;
    }
    
#ifdef __cplusplus
}
#endif

