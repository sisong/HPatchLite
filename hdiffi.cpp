//hdiffi.cpp
// diff tool for HPatchLite
//
/*
 The MIT License (MIT)
 Copyright (c) 2020-2022 HouSisong All Rights Reserved.
 */

#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "HDiffPatch/libParallel/parallel_import.h"
#include "HDiffPatch/libHDiffPatch/HDiff/diff_for_hpatch_lite.h"
#include "HDiffPatch/libHDiffPatch/HPatchLite/hpatch_lite.h"
#include "HDiffPatch/_clock_for_demo.h"
#include "HDiffPatch/_atosize.h"
#include "HDiffPatch/file_for_patch.h"
#include "HDiffPatch/libHDiffPatch/HDiff/private_diff/mem_buf.h"
#include "hdiffi_import_patch.h"

#ifndef _IS_NEED_MAIN
#   define  _IS_NEED_MAIN 1
#endif

#ifndef _IS_NEED_DEFAULT_CompressPlugin
#   define _IS_NEED_DEFAULT_CompressPlugin 1
#endif
#if (_IS_NEED_DEFAULT_CompressPlugin)
//===== select needs decompress plugins or change to your plugin=====
#   define _CompressPlugin_tuz    // decompress requires tiny code(.text) & ram
#   define _CompressPlugin_zlib
#   define _CompressPlugin_lzma   // better compresser
#   define _CompressPlugin_lzma2  // better compresser
#endif

#include "HDiffPatch/compress_plugin_demo.h"
#include "HDiffPatch/decompress_plugin_demo.h"

static void printVersion(){
    printf("HPatchLite::hdiffi v" HPATCHLITE_VERSION_STRING "\n");
}

static void printHelpInfo(){
    printf("  -h (or -?)\n"
           "      output usage info.\n");
}

static void printUsage(){
    printVersion();
    printf("\n");
    printf("diff    usage: hdiffi [options] oldFile newFile outDiffFile\n"
           "test    usage: hdiffi    -t     oldFile newFile testDiffFile\n"
           "  oldFile can empty, and input parameter \"\"\n"
           "options:\n"
           "  -m[-matchScore]\n"
           "      requires (newFileSize+ oldFileSize*5(or *9 when oldFileSize>=2GB))+O(1)\n"
           "        bytes of memory;\n"
           "      matchScore>=0, DEFAULT -m-6\n"
           "  -cache \n"
           "      set is use a big cache for slow match, DEFAULT false;\n"
           "      if newData not similar to oldData then diff speed++,\n"
           "      big cache max used O(oldFileSize) memory, and build slow(diff speed--)\n" 
#if (_IS_USED_MULTITHREAD)
           "  -p-parallelThreadNumber\n"
           "      if parallelThreadNumber>1 then open multi-thread Parallel mode;\n"
           "      DEFAULT -p-4; requires more memory!\n"
#endif
           "  -c-compressType[-compressLevel]\n"
           "      set outDiffFile Compress type, DEFAULT uncompress;\n"
           "      support compress type & level & dict:\n"
#ifdef _CompressPlugin_tuz
           "        -c-tuz[-dictSize]               (or -tinyuz)\n"
           "        -c-tuzi[-dictSize]              (or -tinyuzi)\n"
           "            1<=dictSize<=1g, can like 250,511,1k,4k,64k,1m,64m,512m..., DEFAULT 32k\n"
           "            Note: -c-tuz is default compressor;\n"
           "            But if your compile tinyuz deccompressor source code, set tuz_isNeedLiteralLine=0,\n"
           "            then must used -c-tuzi compressor.\n"
#endif
#ifdef _CompressPlugin_zlib
           "        -c-zlib[-{1..9}[-dictBits]]     DEFAULT level 9\n"
           "            dictBits can 9--15, DEFAULT 15.\n"
#   if (_IS_USED_MULTITHREAD)
           "        -c-pzlib[-{1..9}[-dictBits]]    DEFAULT level 6\n"
           "            dictBits can 9--15, DEFAULT 15.\n"
           "            support run by multi-thread parallel, fast!\n"
#   endif
#endif
#ifdef _CompressPlugin_lzma
           "        -c-lzma[-{0..9}[-dictSize]]     DEFAULT level 7\n"
           "            dictSize can like 4096 or 4k or 4m or 128m etc..., DEFAULT 32k\n"
#   if (_IS_USED_MULTITHREAD)
           "            support run by 2-thread parallel.\n"
#   endif
#endif
#ifdef _CompressPlugin_lzma2
           "        -c-lzma2[-{0..9}[-dictSize]]    DEFAULT level 7\n"
           "            dictSize can like 4096 or 4k or 4m or 128m etc..., DEFAULT 32k\n"
#   if (_IS_USED_MULTITHREAD)
           "            support run by multi-thread parallel, fast!\n"
#   endif
           "            WARNING: code not compatible with it compressed by -c-lzma!\n"
#endif
           "  -d  Diff only, do't run patch check, DEFAULT run patch check.\n"
           "  -t  Test only, run patch check, patch(oldFile,testDiffFile)==newFile ? \n"
           "  -f  Force overwrite, ignore write path already exists;\n"
           "      DEFAULT (no -f) not overwrite and then return error;\n"
           "      if used -f and write path is exist directory, will always return error.\n"
           "  --patch\n"
           "      swap to hpatchi mode.\n"
           "  -v  output Version info.\n"
           );
    printHelpInfo();
    printf("\n");
}

typedef enum THDiffiResult {
    HDIFFI_SUCCESS=0,
    HDIFFI_OPTIONS_ERROR,
    HDIFFI_OPENREAD_ERROR,
    HDIFFI_OPENWRITE_ERROR,
    HDIFFI_FILECLOSE_ERROR,
    HDIFFI_MEM_ERROR, // 5
    HDIFFI_DIFF_ERROR,
    HDIFFI_PATCH_ERROR,
    HDIFFI_PATHTYPE_ERROR,
} THDiffiResult;

int hdiffi_cmd_line(int argc,const char * argv[]);

struct TDiffiSets{
    hpi_BOOL isDiffInMem;
    hpi_BOOL isUseBigCacheMatch;
    size_t   matchScore;
    hpi_BOOL isDoDiff;
    hpi_BOOL isDoPatchCheck;
    size_t   threadNum;
};

int hdiffi(const char* oldFileName,const char* newFileName,const char* outDiffFileName,
           const hdiffi_TCompress* compressPlugin,const TDiffiSets& diffSets);

#define _checkPatchMode(_argc,_argv)            \
    if (isSwapToPatchMode(_argc,_argv)){        \
        printf("hdiffi swap to hpatchi mode.\n\n"); \
        return hpatchi_cmd_line(_argc,_argv);    \
    }

#if (_IS_NEED_MAIN)
#   if (_IS_USED_WIN32_UTF8_WAPI)
int wmain(int argc,wchar_t* argv_w[]){
    hdiff_private::TAutoMem  _mem(hpatch_kPathMaxSize*4);
    char** argv_utf8=(char**)_mem.data();
    if (!_wFileNames_to_utf8((const wchar_t**)argv_w,argc,argv_utf8,_mem.size()))
        return HDIFFI_OPTIONS_ERROR;
    SetDefaultStringLocale();
    _checkPatchMode(argc,(const char**)argv_utf8);
    return hdiffi_cmd_line(argc,(const char**)argv_utf8);
}
#   else
int main(int argc,char* argv[]){
    _checkPatchMode(argc,(const char**)argv);
    return  hdiffi_cmd_line(argc,(const char**)argv);
}
#   endif
#endif

static hpi_BOOL findDecompressByHDP(hpatch_TDecompress** out_decompressPlugin,hpi_compressType compressType){
    switch (compressType){
        case hpi_compressType_no: *out_decompressPlugin=0; return hpi_TRUE;
      #ifdef  _CompressPlugin_tuz
        case hpi_compressType_tuz: *out_decompressPlugin=&tuzDecompressPlugin; return hpi_TRUE;
      #endif
      #ifdef  _CompressPlugin_zlib
        case hpi_compressType_zlib: *out_decompressPlugin=&zlibDecompressPlugin; return hpi_TRUE;
      #endif
      #ifdef  _CompressPlugin_lzma
        case hpi_compressType_lzma: *out_decompressPlugin=&lzmaDecompressPlugin; return hpi_TRUE;
      #endif
      #ifdef  _CompressPlugin_lzma2
        case hpi_compressType_lzma2: *out_decompressPlugin=&lzma2DecompressPlugin; return hpi_TRUE;
      #endif
        default: return hpi_FALSE;
    }
}

static bool _tryGetCompressSet(const char** isMatchedType,const char* ptype,const char* ptypeEnd,
                               const char* cmpType,const char* cmpType2=0,
                               size_t* compressLevel=0,size_t levelMin=0,size_t levelMax=0,size_t levelDefault=0,
                               size_t* dictSize=0,size_t dictSizeMin=0,size_t dictSizeMax=0,size_t dictSizeDefault=0){
    assert (0==(*isMatchedType));
    const size_t ctypeLen=strlen(cmpType);
    const size_t ctype2Len=(cmpType2!=0)?strlen(cmpType2):0;
    if ( ((ctypeLen==(size_t)(ptypeEnd-ptype))&&(0==strncmp(ptype,cmpType,ctypeLen)))
        || ((cmpType2!=0)&&(ctype2Len==(size_t)(ptypeEnd-ptype))&&(0==strncmp(ptype,cmpType2,ctype2Len))) )
        *isMatchedType=cmpType; //ok
    else
        return true;//type mismatch
    
    if ((compressLevel)&&(ptypeEnd[0]=='-')){
        const char* plevel=ptypeEnd+1;
        const char* plevelEnd=findUntilEnd(plevel,'-');
        if (!kmg_to_size(plevel,plevelEnd-plevel,compressLevel)) return false; //error
        if (*compressLevel<levelMin) *compressLevel=levelMin;
        else if (*compressLevel>levelMax) *compressLevel=levelMax;
        if ((dictSize)&&(plevelEnd[0]=='-')){
            const char* pdictSize=plevelEnd+1;
            const char* pdictSizeEnd=findUntilEnd(pdictSize,'-');
            if (!kmg_to_size(pdictSize,pdictSizeEnd-pdictSize,dictSize)) return false; //error
            if (*dictSize<dictSizeMin) *dictSize=dictSizeMin;
            else if (*dictSize>dictSizeMax) *dictSize=dictSizeMax;
        }else{
            if (plevelEnd[0]!='\0') return false; //error
            if (dictSize) *dictSize=dictSizeDefault;
        }
    }else{
        if (ptypeEnd[0]!='\0') return false; //error
        if (compressLevel) *compressLevel=levelDefault;
        if (dictSize) *dictSize=dictSizeDefault;
    }
    return true;
}

#define _options_check(value,errorInfo){ \
    if (!(value)) { LOG_ERR("options " errorInfo " ERROR!\n\n"); printHelpInfo(); return HDIFFI_OPTIONS_ERROR; } }

#define __getCompressSet(_tryGet_code,_errTag)  \
    if (isMatchedType==0){                      \
        _options_check(_tryGet_code,_errTag);   \
        if (isMatchedType)

static int _checkSetCompress(hdiffi_TCompress* out_compressPlugin,
                             const char* ptype,const char* ptypeEnd){
    const char* isMatchedType=0;
    size_t      compressLevel=0;
#if (defined _CompressPlugin_lzma)||(defined _CompressPlugin_lzma2)||(defined _CompressPlugin_tuz)
    size_t       dictSize=0;
    const size_t defaultDictSize=(1<<10)*32; //32k
#endif
#if (defined _CompressPlugin_zlib)
    size_t       dictBits=0;
    const size_t defaultDictBits=15; //32k
#endif
#ifdef _CompressPlugin_zlib
    __getCompressSet(_tryGetCompressSet(&isMatchedType,ptype,ptypeEnd,"zlib",0,
                                        &compressLevel,1,9,9, &dictBits,9,15,defaultDictBits),"-c-zlib-?"){
        static TCompressPlugin_zlib _zlibCompressPlugin=zlibCompressPlugin;
        _zlibCompressPlugin.compress_level=(int)compressLevel;
        _zlibCompressPlugin.windowBits=-(signed char)dictBits;
        out_compressPlugin->compress=&_zlibCompressPlugin.base;
        out_compressPlugin->compress_type=hpi_compressType_zlib; }}
#   if (_IS_USED_MULTITHREAD)
    //pzlib
    __getCompressSet(_tryGetCompressSet(&isMatchedType,ptype,ptypeEnd,"pzlib",0,
                                        &compressLevel,1,9,6, &dictBits,9,15,defaultDictBits),"-c-pzlib-?"){
        static TCompressPlugin_pzlib _pzlibCompressPlugin=pzlibCompressPlugin;
        _pzlibCompressPlugin.base.compress_level=(int)compressLevel;
        _pzlibCompressPlugin.base.windowBits=-(signed char)dictBits;
        out_compressPlugin->compress=&_pzlibCompressPlugin.base.base;
        out_compressPlugin->compress_type=hpi_compressType_zlib; }}
#   endif // _IS_USED_MULTITHREAD
#endif
#ifdef _CompressPlugin_lzma
    __getCompressSet(_tryGetCompressSet(&isMatchedType,ptype,ptypeEnd,"lzma",0,
                                        &compressLevel,0,9,7, &dictSize,1<<12,
                                        (sizeof(size_t)<=4)?(1<<27):((size_t)3<<29),defaultDictSize),"-c-lzma-?"){
        static TCompressPlugin_lzma _lzmaCompressPlugin=lzmaCompressPlugin;
        _lzmaCompressPlugin.compress_level=(int)compressLevel;
        _lzmaCompressPlugin.dict_size=(int)dictSize;
        out_compressPlugin->compress=&_lzmaCompressPlugin.base;
        out_compressPlugin->compress_type=hpi_compressType_lzma; }}
#endif
#ifdef _CompressPlugin_lzma2
    __getCompressSet(_tryGetCompressSet(&isMatchedType,ptype,ptypeEnd,"lzma2",0,
                                        &compressLevel,0,9,7, &dictSize,1<<12,
                                        (sizeof(size_t)<=4)?(1<<27):((size_t)3<<29),defaultDictSize),"-c-lzma2-?"){
        static TCompressPlugin_lzma2 _lzma2CompressPlugin=lzma2CompressPlugin;
        _lzma2CompressPlugin.compress_level=(int)compressLevel;
        _lzma2CompressPlugin.dict_size=(int)dictSize;
        out_compressPlugin->compress=&_lzma2CompressPlugin.base;
        out_compressPlugin->compress_type=hpi_compressType_lzma2; }}
#endif
#ifdef _CompressPlugin_tuz
    __getCompressSet(_tryGetCompressSet(&isMatchedType,
                                        ptype,ptypeEnd,"tuz","tinyuz",
                                        &dictSize,tuz_kMinOfDictSize,tuz_kMaxOfDictSize,defaultDictSize),"-c-tuz-?"){
        static TCompressPlugin_tuz _tuzCompressPlugin=tuzCompressPlugin;
        _tuzCompressPlugin.props.dictSize=(tuz_size_t)dictSize;
        _tuzCompressPlugin.props.isNeedLiteralLine=true;
        out_compressPlugin->compress=&_tuzCompressPlugin.base;
        out_compressPlugin->compress_type=hpi_compressType_tuz; }}
    __getCompressSet(_tryGetCompressSet(&isMatchedType,
                                        ptype,ptypeEnd,"tuzi","tinyuzi",
                                        &dictSize,tuz_kMinOfDictSize,tuz_kMaxOfDictSize,defaultDictSize),"-c-tuzi-?"){
        static TCompressPlugin_tuz _tuzCompressPlugin=tuzCompressPlugin;
        _tuzCompressPlugin.props.dictSize=(tuz_size_t)dictSize;
        _tuzCompressPlugin.props.isNeedLiteralLine=false;
        out_compressPlugin->compress=&_tuzCompressPlugin.base;
        out_compressPlugin->compress_type=hpi_compressType_tuz; }}
#endif

    _options_check((out_compressPlugin->compress!=0),"-c-?");
    return HDIFFI_SUCCESS;
}

#define _return_check(value,exitCode,errorInfo){ \
    if (!(value)) { LOG_ERR(errorInfo " ERROR!\n"); return exitCode; } }

#define _kNULL_VALUE    ((hpi_BOOL)(-1))
#define _kNULL_SIZE     (~(size_t)0)

#define _THREAD_NUMBER_NULL     0
#define _THREAD_NUMBER_MIN      1
#define _THREAD_NUMBER_DEFUALT  kDefaultCompressThreadNumber
#define _THREAD_NUMBER_MAX      (1<<8)

int hdiffi_cmd_line(int argc, const char * argv[]){
    TDiffiSets diffSets;
    memset(&diffSets,0,sizeof(diffSets));
    diffSets.isDiffInMem=_kNULL_VALUE;
    diffSets.isDoDiff =_kNULL_VALUE;
    diffSets.isDoPatchCheck=_kNULL_VALUE;
    diffSets.isUseBigCacheMatch =_kNULL_VALUE;
    diffSets.threadNum = _THREAD_NUMBER_NULL;
    hpi_BOOL isForceOverwrite=_kNULL_VALUE;
    hpi_BOOL isOutputHelp=_kNULL_VALUE;
    hpi_BOOL isOutputVersion=_kNULL_VALUE;
    hpi_BOOL isOldFileInputEmpty=_kNULL_VALUE;
    hdiffi_TCompress      compressPlugin={0,hpi_compressType_no};
    std::vector<const char *> arg_values;
    if (argc<=1){
        printUsage();
        return HDIFFI_OPTIONS_ERROR;
    }
    for (int i=1; i<argc; ++i) {
        const char* op=argv[i];
        _options_check(op!=0,"?");
        if (op[0]!='-'){
            hpi_BOOL isEmpty=(strlen(op)==0);
            if (isEmpty){
                if (isOldFileInputEmpty==_kNULL_VALUE)
                    isOldFileInputEmpty=hpi_TRUE;
                else
                    _options_check(!isEmpty,"?"); //error return
            }else{
                if (isOldFileInputEmpty==_kNULL_VALUE)
                    isOldFileInputEmpty=hpi_FALSE;
            }
            arg_values.push_back(op); //path:file or directory
            continue;
        }
        switch (op[1]) {
            case 'm':{ //diff in memory
                _options_check((diffSets.isDiffInMem==_kNULL_VALUE)&&((op[2]=='-')||(op[2]=='\0')),"-m");
                diffSets.isDiffInMem=hpi_TRUE;
                if (op[2]=='-'){
                    const char* pnum=op+3;
                    _options_check(kmg_to_size(pnum,strlen(pnum),&diffSets.matchScore),"-m-?");
                    _options_check((0<=(int)diffSets.matchScore)&&(diffSets.matchScore==(size_t)(int)diffSets.matchScore),"-m-?");
                }else{
                    diffSets.matchScore=kLiteMatchScore_default;
                }
            } break;
            case '?':
            case 'h':{
                _options_check((isOutputHelp==_kNULL_VALUE)&&(op[2]=='\0'),"-h");
                isOutputHelp=hpi_TRUE;
            } break;
            case 'v':{
                _options_check((isOutputVersion==_kNULL_VALUE)&&(op[2]=='\0'),"-v");
                isOutputVersion=hpi_TRUE;
            } break;
            case 't':{
                _options_check((diffSets.isDoDiff==_kNULL_VALUE)&&(diffSets.isDoPatchCheck==_kNULL_VALUE)&&(op[2]=='\0'),"-t -d?");
                diffSets.isDoDiff=hpi_FALSE;
                diffSets.isDoPatchCheck=hpi_TRUE; //only test diffFile
            } break;
            case 'd':{
                _options_check((diffSets.isDoDiff==_kNULL_VALUE)&&(diffSets.isDoPatchCheck==_kNULL_VALUE)&&(op[2]=='\0'),"-t -d?");
                diffSets.isDoDiff=hpi_TRUE; //only diff
                diffSets.isDoPatchCheck=hpi_FALSE;
            } break;
            case 'f':{
                _options_check((isForceOverwrite==_kNULL_VALUE)&&(op[2]=='\0'),"-f");
                isForceOverwrite=hpi_TRUE;
            } break;
#if (_IS_USED_MULTITHREAD)
            case 'p':{
                _options_check((diffSets.threadNum==_THREAD_NUMBER_NULL)&&(op[2]=='-'),"-p-?");
                const char* pnum=op+3;
                _options_check(a_to_size(pnum,strlen(pnum),&diffSets.threadNum),"-p-?");
                _options_check(diffSets.threadNum>=_THREAD_NUMBER_MIN,"-p-?");
            } break;
#endif
            case 'c':{
                if (op[2]=='-'){
                    _options_check((compressPlugin.compress==0),"-c-");
                    const char* ptype=op+3;
                    const char* ptypeEnd=findUntilEnd(ptype,'-');
                    int result=_checkSetCompress(&compressPlugin,ptype,ptypeEnd);
                    if (HDIFFI_SUCCESS!=result)
                        return result;
                }else if (op[2]=='a'){
                    _options_check((diffSets.isUseBigCacheMatch==_kNULL_VALUE)&&
                        (op[3]=='c')&&(op[4]=='h')&&(op[5]=='e')&&(op[6]=='\0'),"-cache?");
                    diffSets.isUseBigCacheMatch=hpi_TRUE; //use big cache for match 
                }else{
                    _options_check(false,"-c?");
                }
            } break;
            default: {
                _options_check(hpi_FALSE,"-?");
            } break;
        }//switch
    }

    if (isOutputHelp==_kNULL_VALUE)
        isOutputHelp=hpi_FALSE;
    if (isOutputVersion==_kNULL_VALUE)
        isOutputVersion=hpi_FALSE;
    if (isForceOverwrite==_kNULL_VALUE)
        isForceOverwrite=hpi_FALSE;
    if (isOutputHelp||isOutputVersion){
        if (isOutputHelp)
            printUsage();//with version
        else
            printVersion();
        if (arg_values.empty())
            return 0; //ok
    }
    if (diffSets.threadNum==_THREAD_NUMBER_NULL)
        diffSets.threadNum=_THREAD_NUMBER_DEFUALT;
    else if (diffSets.threadNum>_THREAD_NUMBER_MAX)
        diffSets.threadNum=_THREAD_NUMBER_MAX;
    if (compressPlugin.compress!=0){
        hdiff_TCompress* compress=(hdiff_TCompress*)compressPlugin.compress;
        compress->setParallelThreadNumber(compress,(int)diffSets.threadNum);
    }
    
    if (isOldFileInputEmpty==_kNULL_VALUE)
        isOldFileInputEmpty=hpi_FALSE;
    _options_check((arg_values.size()==3),"input count");
    { //diff
        if (diffSets.isDiffInMem==_kNULL_VALUE){
            diffSets.isDiffInMem=hpi_TRUE;
            diffSets.matchScore=kLiteMatchScore_default;
        }
        if (diffSets.isDoDiff==_kNULL_VALUE)
            diffSets.isDoDiff=hpi_TRUE;
        if (diffSets.isDoPatchCheck==_kNULL_VALUE)
            diffSets.isDoPatchCheck=hpi_TRUE;
        assert(diffSets.isDoDiff||diffSets.isDoPatchCheck);
        if (diffSets.isUseBigCacheMatch==_kNULL_VALUE)
            diffSets.isUseBigCacheMatch=hpi_FALSE;
        if (diffSets.isDoDiff&&(!diffSets.isDiffInMem)){
            _options_check(!diffSets.isUseBigCacheMatch, "-cache must run with -m");
        }

        const char* oldFile        =arg_values[0];
        const char* newFile        =arg_values[1];
        const char* outDiffFileName=arg_values[2];
        
        _return_check(!hpatch_getIsSamePath(oldFile,outDiffFileName),
                      HDIFFI_PATHTYPE_ERROR,"oldFile outDiffFile same path");
        _return_check(!hpatch_getIsSamePath(newFile,outDiffFileName),
                      HDIFFI_PATHTYPE_ERROR,"newFile outDiffFile same path");
        if (!isForceOverwrite){
            hpatch_TPathType   outDiffFileType;
            _return_check(hpatch_getPathStat(outDiffFileName,&outDiffFileType,0),
                          HDIFFI_PATHTYPE_ERROR,"get outDiffFile type");
            _return_check((outDiffFileType==kPathType_notExist)||(!diffSets.isDoDiff),
                          HDIFFI_PATHTYPE_ERROR,"diff outDiffFile already exists, overwrite");
        }
        hpatch_TPathType oldType;
        hpatch_TPathType newType;
        if (isOldFileInputEmpty){
            oldType=kPathType_file;     //as empty file
        }else{
            _return_check(hpatch_getPathStat(oldFile,&oldType,0),HDIFFI_PATHTYPE_ERROR,"get oldFile type");
            _return_check((oldType!=kPathType_notExist),HDIFFI_PATHTYPE_ERROR,"oldFile not exist");
        }
        _return_check(hpatch_getPathStat(newFile,&newType,0),HDIFFI_PATHTYPE_ERROR,"get newFile type");
        _return_check((newType!=kPathType_notExist),HDIFFI_PATHTYPE_ERROR,"newFile not exist");

        return hdiffi(oldFile,newFile,outDiffFileName,&compressPlugin,diffSets);
    }
}

#define _check_readFile(value) { if (!(value)) { hpatch_TFileStreamInput_close(&file); return hpi_FALSE; } }
static hpi_BOOL readFileAll(hdiff_private::TAutoMem& out_mem,const char* fileName){
    size_t              dataSize;
    hpatch_TFileStreamInput    file;
    hpatch_TFileStreamInput_init(&file);
    _check_readFile(hpatch_TFileStreamInput_open(&file,fileName));
    dataSize=(size_t)file.base.streamSize;
    _check_readFile(dataSize==file.base.streamSize);
    try {
        out_mem.realloc(dataSize);
    } catch (...) {
        _check_readFile(false);
    }
    _check_readFile(file.base.read(&file.base,0,out_mem.data(),out_mem.data_end()));
    return hpatch_TFileStreamInput_close(&file);
}

#define  _check_on_error(errorType) { \
    if (result==HDIFFI_SUCCESS) result=errorType; if (!_isInClear){ goto clear; } }
#define check(value,errorType,errorInfo) { \
    std::string erri=std::string()+errorInfo+" ERROR!\n"; \
    if (!(value)){ hpatch_printStdErrPath_utf8(erri.c_str()); _check_on_error(errorType); } }

static bool check_lite_diff_by_hpatchi(const hpi_byte* newData,const hpi_byte* newData_end,
                                       const hpi_byte* oldData,const hpi_byte* oldData_end,
                                       const hpi_byte* lite_diff,const hpi_byte* lite_diff_end);

static int hdiffi_in_mem(const char* oldFileName,const char* newFileName,const char* outDiffFileName,
                         const hdiffi_TCompress* compressPlugin,const TDiffiSets& diffSets){
    double diff_time0=clock_s();
    int    result=HDIFFI_SUCCESS;
    int    _isInClear=hpi_FALSE;
    hpatch_TFileStreamOutput diffData_out;
    hpatch_TFileStreamOutput_init(&diffData_out);
    hdiff_private::TAutoMem oldMem(0);
    hdiff_private::TAutoMem newMem(0);
    if (oldFileName&&(strlen(oldFileName)>0))
        check(readFileAll(oldMem,oldFileName),HDIFFI_OPENREAD_ERROR,"open oldFile");
    check(readFileAll(newMem,newFileName),HDIFFI_OPENREAD_ERROR,"open newFile");
    printf("oldDataSize : %" PRIu64 "\nnewDataSize : %" PRIu64 "\n",
           (hpatch_StreamPos_t)oldMem.size(),(hpatch_StreamPos_t)newMem.size());
    if (diffSets.isDoDiff){
        std::vector<hpi_byte> outDiffData;
        try {
                create_lite_diff(newMem.data(),newMem.data_end(),oldMem.data(),oldMem.data_end(),
                                 outDiffData,compressPlugin,(int)diffSets.matchScore,
                                 diffSets.isUseBigCacheMatch?true:false,0,diffSets.threadNum);
        }catch(const std::exception& e){
            check(false,HDIFFI_DIFF_ERROR,"diff run error: "+e.what());
        }
        {
            check(hpatch_TFileStreamOutput_open(&diffData_out,outDiffFileName,outDiffData.size()),
                HDIFFI_OPENWRITE_ERROR,"open out diffFile");
            check(diffData_out.base.write(&diffData_out.base,0,outDiffData.data(),outDiffData.data()+outDiffData.size()),
                HDIFFI_OPENWRITE_ERROR,"write diffFile");
            check(hpatch_TFileStreamOutput_close(&diffData_out),HDIFFI_FILECLOSE_ERROR,"out diffFile close");
        }
        printf("diffDataSize: %" PRIu64 "\n",(hpatch_StreamPos_t)outDiffData.size());
        printf("hdiffi  time: %.3f s\n",(clock_s()-diff_time0));
        printf("  out diff file ok!\n");
    }
    if (diffSets.isDoPatchCheck){
        double patch_time0=clock_s();
        hdiff_private::TAutoMem diffMem(0);
        {
            printf("\nload diffFile for test by patch:\n");
            check(readFileAll(diffMem,outDiffFileName),
                HDIFFI_OPENREAD_ERROR,"open diffFile for test");
            printf("diffDataSize: %" PRIu64 "\n",(hpatch_StreamPos_t)diffMem.size());
        }
        hpatch_TDecompress* saved_decompressPlugin;
        if (0){ // check diffData by HDiffPatch
            hpi_compressType    compressType;
            if (!check_lite_diff_open(diffMem.data(),diffMem.data_end(),&compressType))
                check(hpi_FALSE,HDIFFI_PATCH_ERROR,"check_lite_diff_open()");
            check(findDecompressByHDP(&saved_decompressPlugin,compressType),
                  HDIFFI_PATCH_ERROR,"diff data saved compress type");

            check(check_lite_diff(newMem.data(),newMem.data_end(),oldMem.data(),oldMem.data_end(),
                                  diffMem.data(),diffMem.data_end(),saved_decompressPlugin),
                  HDIFFI_PATCH_ERROR,"check_lite_diff()");
        }else{ // check diffData by HPatchLite
            check(check_lite_diff_by_hpatchi(newMem.data(),newMem.data_end(),oldMem.data(),oldMem.data_end(),
                                             diffMem.data(),diffMem.data_end()),
                  HDIFFI_PATCH_ERROR,"check_lite_diff_by_hpatchi()");
        }
        printf("hpatchi time: %.3f s\n",(clock_s()-patch_time0));
        printf("  patch check diff data ok!\n");
    }
clear:
    _isInClear=hpi_TRUE;
    check(hpatch_TFileStreamOutput_close(&diffData_out),HDIFFI_FILECLOSE_ERROR,"out diffFile close");
    return result;
}

int hdiffi(const char* oldFileName,const char* newFileName,const char* outDiffFileName,
           const hdiffi_TCompress* compressPlugin,const TDiffiSets& diffSets){
    double time0=clock_s();
    std::string fnameInfo=std::string("old : \"")+oldFileName+"\"\n"
                                     +"new : \""+newFileName+"\"\n"
                             +(diffSets.isDoDiff?"out : \"":"test: \"")+outDiffFileName+"\"\n";
    hpatch_printPath_utf8(fnameInfo.c_str());
    
    if (diffSets.isDoDiff) {
        const char* compressType="";
        if (compressPlugin->compress) compressType=compressPlugin->compress->compressType();
        printf("hdiffi run with compress plugin: \"%s\"\n",compressType);
    }
    
    int exitCode;
    assert(diffSets.isDiffInMem);
    exitCode=hdiffi_in_mem(oldFileName,newFileName,outDiffFileName,compressPlugin,diffSets);
    if (diffSets.isDoDiff && diffSets.isDoPatchCheck)
        printf("\nall   time: %.3f s\n",(clock_s()-time0));
    return exitCode;
}


// check_lite_diff_by_hpatchi

struct TPatchiChecker{
    hpatchi_listener_t base;
    const hpi_byte* newData_cur;
    const hpi_byte* newData_end;
    const hpi_byte* oldData;
    const hpi_byte* oldData_end;
    const hpi_byte* diffData_cur;
    const hpi_byte* diffData_end;

    static hpi_BOOL _read_diff(hpi_TInputStreamHandle inputStream,hpi_byte* out_data,hpi_size_t* data_size){
        TPatchiChecker& self=*(TPatchiChecker*)inputStream;
        const hpi_byte* cur=self.diffData_cur;
        size_t d_size=self.diffData_end-cur;
        size_t r_size=*data_size;
        if (r_size>d_size){
            r_size=d_size;
            *data_size=(hpi_size_t)r_size;
        }
        memcpy(out_data,cur,r_size);
        self.diffData_cur=cur+r_size;
        return hpi_TRUE;
    }
    static hpi_BOOL _read_old(struct hpatchi_listener_t* listener,hpi_pos_t read_from_pos,hpi_byte* out_data,hpi_size_t data_size){
        TPatchiChecker& self=*(TPatchiChecker*)listener;
        size_t dsize=self.oldData_end-self.oldData;
        if ((read_from_pos>dsize)|(data_size>(size_t)(dsize-read_from_pos))) return hpi_FALSE;
        memcpy(out_data,self.oldData+(size_t)read_from_pos,data_size);
        return hpi_TRUE;
    }
    static hpi_BOOL _write_new(struct hpatchi_listener_t* listener,const hpi_byte* data,hpi_size_t data_size){
        TPatchiChecker& self=*(TPatchiChecker*)listener;
        if (data_size>(size_t)(self.newData_end-self.newData_cur)) 
            return hpi_FALSE;
        if (0!=memcmp(self.newData_cur,data,data_size))
            return hpi_FALSE;
        self.newData_cur+=data_size;
        return hpi_TRUE;
    }
};

static bool check_lite_diff_by_hpatchi(const hpi_byte* newData,const hpi_byte* newData_end,
                                       const hpi_byte* oldData,const hpi_byte* oldData_end,
                                       const hpi_byte* lite_diff,const hpi_byte* lite_diff_end){
    hpi_compressType    compress_type;
    hpi_pos_t           newSize;
    hpi_pos_t           uncompressSize;
    TPatchiChecker     listener={{&listener,TPatchiChecker::_read_diff,TPatchiChecker::_read_old,TPatchiChecker::_write_new},
                                  newData,newData_end,oldData,oldData_end,lite_diff,lite_diff_end};

    if (!hpatch_lite_open(listener.base.diff_data,listener.base.read_diff,
                          &compress_type,&newSize,&uncompressSize)) return hpi_FALSE;
    if (newSize!=(size_t)(newData_end-newData)) return hpi_FALSE;
    size_t patchCacheSize=kDecompressBufSize;
    if (0!=hpatchi_patch(&listener.base,compress_type,newSize,uncompressSize,patchCacheSize)) return hpi_FALSE;
    return listener.newData_cur==listener.newData_end;
}
