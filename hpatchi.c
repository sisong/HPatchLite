//hpatchi.c
// patch tool for HPatchLite
//
/*
 The MIT License (MIT)
 Copyright (c) 2020-2022 HouSisong All Rights Reserved.
 */

#include <string.h>
#include <stdlib.h>
#include <stdio.h>  //fprintf
#include "HDiffPatch/libHDiffPatch/HPatchLite/hpatch_lite.h"
#include "HDiffPatch/_clock_for_demo.h"
#include "HDiffPatch/_atosize.h"
#include "HDiffPatch/file_for_patch.h"

#ifndef _IS_NEED_MAIN
#   define  _IS_NEED_MAIN 1
#endif

#ifndef _IS_NEED_DEFAULT_CompressPlugin
#   define _IS_NEED_DEFAULT_CompressPlugin 1
#endif
#if (_IS_NEED_DEFAULT_CompressPlugin)
//===== select needs decompress plugins or change to your plugin=====
//#   define _CompressPlugin_zlib
//#   define _CompressPlugin_lzma
//#   define _CompressPlugin_lzma2
#   define _CompressPlugin_tuz
#endif

#ifdef _CompressPlugin_tuz
#   include "tuz_dec.h"
#endif

static void printVersion(){
    printf("HPatchLite::hpatchi v" HPATCHLITE_VERSION_STRING "\n");
}

static void printHelpInfo(){
    printf("  -h  (or -?)\n"
           "      output usage info.\n");
}

static void printUsage(){
    printVersion();
    printf("\n");
    printf("patch usage: hpatchi [options] oldFile diffFile outNewFile\n"
           "  if oldFile is empty input parameter \"\"\n"
           "options:\n"
           "  -s[-cacheSize] \n"
           "      DEFAULT -s-12k; cacheSize>=3, can like 256,1k, 60k or 1m etc....\n"
           "      requires (cacheSize + 1*decompress buffer size)+O(1) bytes of memory.\n"
           "  -f  Force overwrite, ignore write path already exists;\n"
           "      DEFAULT (no -f) not overwrite and then return error;\n"
           "  -v  output Version info.\n"
           );
    printHelpInfo();
    printf("\n");
}

typedef enum THPatchiResult {
    HPATCHI_SUCCESS=0,
    HPATCHI_OPTIONS_ERROR,
    HPATCHI_OPENREAD_ERROR,
    HPATCHI_OPENWRITE_ERROR,
    HPATCHI_FILEREAD_ERROR,
    HPATCHI_FILEWRITE_ERROR, // 5
    HPATCHI_FILEDATA_ERROR,
    HPATCHI_FILECLOSE_ERROR,
    HPATCHI_MEM_ERROR,
    HPATCHI_OPENDECOMPRESSER_ERROR,
    HPATCHI_COMPRESSTYPE_ERROR, //10
    HPATCHI_PATCH_OPEN_ERROR,
    HPATCHI_PATCH_ERROR,
    HPATCHI_PATHTYPE_ERROR,
} THPatchiResult;

int hpatchi_cmd_line(int argc, const char * argv[]);

int hpatchi(const char* oldFileName,const char* diffFileName,const char* outNewFileName,size_t patchCacheSize);

#if (_IS_NEED_MAIN)
#   if (_IS_USED_WIN32_UTF8_WAPI)
int wmain(int argc,wchar_t* argv_w[]){
    char* argv_utf8[hpatch_kPathMaxSize*3/sizeof(char*)];
    if (!_wFileNames_to_utf8((const wchar_t**)argv_w,argc,argv_utf8,sizeof(argv_utf8)))
        return HPATCHI_OPTIONS_ERROR;
    SetDefaultStringLocale();
    return hpatchi_cmd_line(argc,(const char**)argv_utf8);
}
#   else
int main(int argc, const char * argv[]){
    return hpatchi_cmd_line(argc,argv);
}
#   endif
#endif

#define _return_check(value,exitCode,errorInfo){ \
    if (!(value)) { LOG_ERR(errorInfo " ERROR!\n"); return exitCode; } }

#define _options_check(value,errorInfo){ \
    if (!(value)) { LOG_ERR("options " errorInfo " ERROR!\n\n"); \
        printHelpInfo(); return HPATCHI_OPTIONS_ERROR; } }

#define kPatchCacheSize_min      ((hpi_kMinCacheSize+1)/2 *3)
#define kPatchCacheSize_default  (1024*32 *3)
#define kPatchCacheSize_bestmax  (1024*1024*64 *3)

#define _kNULL_VALUE    (-1)
#define _kNULL_SIZE     (~(size_t)0)

#define _isSwapToPatchTag(tag) (0==strcmp("--patch",tag))

int isSwapToPatchMode(int argc,const char* argv[]){
    int i;
    for (i=1;i<argc;++i){
        if (_isSwapToPatchTag(argv[i]))
            return hpatch_TRUE;
    }
    return hpatch_FALSE;
}

int hpatchi_cmd_line(int argc, const char * argv[]){
    hpatch_BOOL isForceOverwrite=_kNULL_VALUE;
    hpatch_BOOL isOutputHelp=_kNULL_VALUE;
    hpatch_BOOL isOutputVersion=_kNULL_VALUE;
    hpatch_BOOL isOldFileInputEmpty=_kNULL_VALUE;
    size_t      patchCacheSize=_kNULL_VALUE;
    #define kMax_arg_values_size 3
    const char* arg_values[kMax_arg_values_size]={0};
    int         arg_values_size=0;
    int         i;
    if (argc<=1){
        printUsage();
        return HPATCHI_OPTIONS_ERROR;
    }
    for (i=1; i<argc; ++i) {
        const char* op=argv[i];
        _options_check(op!=0,"?");
        if (_isSwapToPatchTag(op))
            continue;
        if (op[0]!='-'){
            hpatch_BOOL isEmpty=(strlen(op)==0);
            _options_check(arg_values_size<kMax_arg_values_size,"input count");
            if (isEmpty){
                if (isOldFileInputEmpty==_kNULL_VALUE)
                    isOldFileInputEmpty=hpatch_TRUE;
                else
                    _options_check(!isEmpty,"?"); //error return
            }else{
                if (isOldFileInputEmpty==_kNULL_VALUE)
                    isOldFileInputEmpty=hpatch_FALSE;
            }
            arg_values[arg_values_size]=op; //file
            ++arg_values_size;
            continue;
        }
        _options_check((op!=0)&&(op[0]=='-'),"?");
        switch (op[1]) {
            case 's':{
                _options_check((patchCacheSize==(size_t)_kNULL_VALUE)&&(op[2]=='-')||(op[2]=='\0'),"-s");
                if (op[2]=='-'){
                    const char* pnum=op+3;
                    _options_check(kmg_to_size(pnum,strlen(pnum),&patchCacheSize),"-s-?");
                }else{
                    patchCacheSize=kPatchCacheSize_default;
                }
            } break;
            case 'f':{
                _options_check((isForceOverwrite==_kNULL_VALUE)&&(op[2]=='\0'),"-f");
                isForceOverwrite=hpatch_TRUE;
            } break;
            case '?':
            case 'h':{
                _options_check((isOutputHelp==_kNULL_VALUE)&&(op[2]=='\0'),"-h");
                isOutputHelp=hpatch_TRUE;
            } break;
            case 'v':{
                _options_check((isOutputVersion==_kNULL_VALUE)&&(op[2]=='\0'),"-v");
                isOutputVersion=hpatch_TRUE;
            } break;
            default: {
                _options_check(hpatch_FALSE,"-?");
            } break;
        }//switch
    }
    
    if (isOutputHelp==_kNULL_VALUE)
        isOutputHelp=hpatch_FALSE;
    if (isOutputVersion==_kNULL_VALUE)
        isOutputVersion=hpatch_FALSE;
    if (isForceOverwrite==_kNULL_VALUE)
        isForceOverwrite=hpatch_FALSE;
    if (isOutputHelp||isOutputVersion){
        if (isOutputHelp)
            printUsage();//with version
        else
            printVersion();
        if (arg_values_size==0)
            return 0; //ok
    }
    
    if (patchCacheSize==(size_t)_kNULL_VALUE)
        patchCacheSize=kPatchCacheSize_default;
    else if (patchCacheSize<kPatchCacheSize_min)
        patchCacheSize=kPatchCacheSize_min;
    else if (patchCacheSize>kPatchCacheSize_bestmax)
        patchCacheSize=kPatchCacheSize_bestmax;
    if (isOldFileInputEmpty==_kNULL_VALUE)
        isOldFileInputEmpty=hpatch_FALSE;

    {//patch default mode
        _options_check(arg_values_size==3,"input count");
    }
    {//patch
        const char* oldFile     =0;
        const char* diffFileName=0;
        const char* outNewFile  =0;
        hpatch_StreamPos_t diffDataOffert=0;
        hpatch_StreamPos_t diffDataSize=0;
        {
            oldFile     =arg_values[0];
            diffFileName=arg_values[1];
            outNewFile  =arg_values[2];
        }
        if (!isForceOverwrite){
            hpatch_TPathType   outNewFileType;
            _return_check(hpatch_getPathStat(outNewFile,&outNewFileType,0),
                          HPATCHI_PATHTYPE_ERROR,"get outNewFile type");
            _return_check(outNewFileType==kPathType_notExist,
                          HPATCHI_PATHTYPE_ERROR,"outNewFile already exists, overwrite");
        }
        
        //patch out new file
        return hpatchi(oldFile,diffFileName,outNewFile,patchCacheSize);
    }
}


#define  check_on_error(errorType) { \
    if (result==HPATCHI_SUCCESS) result=errorType; if (!_isInClear){ goto clear; } }
#define  check(value,errorType,errorInfo) { \
    if (!(value)){ LOG_ERR(errorInfo " ERROR!\n"); check_on_error(errorType); } }

#define _free_mem(p) { if (p) { free(p); p=0; } }

typedef struct TPatchListener{
    hpatchi_listener_t      base;
    int                     result;
    hpatch_TStreamInput*    old_file;
    FILE*                   new_file;
} TPatchListener;

static hpi_BOOL _do_readFile(hpi_TInputStreamHandle diffStream,hpi_byte* out_data,hpi_size_t* data_size){
    *data_size=(hpi_size_t)fread(out_data,1,*data_size,(FILE*)diffStream);
    return hpi_TRUE;
}
static hpi_BOOL _do_readOld(struct hpatchi_listener_t* listener,hpi_pos_t read_from_pos,hpi_byte* out_data,hpi_size_t data_size){
    TPatchListener* self=(TPatchListener*)listener;
    return self->old_file->read(self->old_file,read_from_pos,out_data,out_data+data_size);
}
static hpi_BOOL _do_writeNew(struct hpatchi_listener_t* listener,const hpi_byte* data,hpi_size_t data_size){
    TPatchListener* self=(TPatchListener*)listener;
    hpi_BOOL ret=(data_size==fwrite(data,1,data_size,self->new_file));
    if (!ret) self->result=HPATCHI_FILEWRITE_ERROR;
    return ret;
}

#ifdef _CompressPlugin_tuz
static hpi_BOOL _do_tuz_decompress(hpi_TInputStreamHandle diffStream,hpi_byte* out_data,hpi_size_t* data_size){
    return tuz_STREAM_END>=tuz_TStream_decompress_partial((tuz_TStream*)diffStream,out_data,data_size);
}
#endif

int hpatchi(const char* oldFileName,const char* diffFileName,const char* outNewFileName,size_t patchCacheSize){
    int     result=HPATCHI_SUCCESS;
    int     _isInClear=hpatch_FALSE;
    double  time0=clock_s();
    hpatch_TFileStreamOutput    newData;
    hpatch_TFileStreamInput     diffData;
    hpatch_TFileStreamInput     oldData;
    hpatch_TStreamInput* poldData=&oldData.base;
    hpatch_StreamPos_t   savedNewSize=0;
    int                  patch_result=HPATCHI_SUCCESS;
    hpi_byte*            temp_cache=0;
    TPatchListener       patchListener;
#ifdef _CompressPlugin_tuz
    tuz_TStream          tuzStream;
#endif

    patchListener.result=HPATCHI_SUCCESS;
    hpatch_TFileStreamInput_init(&oldData);
    hpatch_TFileStreamInput_init(&diffData);
    hpatch_TFileStreamOutput_init(&newData);
    {//open
        printf(    "old : \""); hpatch_printPath_utf8(oldFileName);
        printf("\"\ndiff: \""); hpatch_printPath_utf8(diffFileName);
        printf("\"\nout : \""); hpatch_printPath_utf8(outNewFileName);
        printf("\"\n");
        if (0==strcmp(oldFileName,"")){ // isOldFileInputEmpty
            mem_as_hStreamInput(&oldData.base,0,0);
        }else{
            check(hpatch_TFileStreamInput_open(&oldData,oldFileName),
                  HPATCHI_OPENREAD_ERROR,"open oldFile for read");
        }
        check(hpatch_TFileStreamInput_open(&diffData,diffFileName),
              HPATCHI_OPENREAD_ERROR,"open diffFile for read");
    }
    printf("oldDataSize : %" PRIu64 "\ndiffDataSize: %" PRIu64 "\n",
           poldData->streamSize,diffData.base.streamSize);

    patchListener.base.diff_data=diffData.m_file;
    patchListener.base.read_diff=_do_readFile;
    {//open diff info
        hpi_pos_t newSize;
        hpi_compressType compress_type;
        if (!hpatch_lite_open(patchListener.base.diff_data,patchListener.base.read_diff,&compress_type,&newSize))
            check(hpatch_FALSE,HPATCHI_PATCH_OPEN_ERROR,"hpatch_lite_open() run");
        
        printf("newDataSize : %" PRIu64 "\n",(hpatch_StreamPos_t)newSize);
        check(hpatch_TFileStreamOutput_open(&newData,outNewFileName,newSize),
              HPATCHI_OPENWRITE_ERROR,"open out newFile for write");

        switch (compress_type){
        case hpi_compressType_no: {
            temp_cache=(hpi_byte*)malloc(patchCacheSize);
            check(temp_cache,HPATCHI_MEM_ERROR,"alloc cache memory");
        }
      #ifdef _CompressPlugin_tuz
        case hpi_compressType_tuz: {
            const size_t stepSize=patchCacheSize/3;
            const tuz_size_t dictSize=tuz_TStream_read_dict_size(patchListener.base.diff_data,patchListener.base.read_diff);
            assert(dictSize<tuz_kMaxOfDictSize);
            assert(stepSize==(tuz_size_t)stepSize);
            temp_cache=(hpi_byte*)malloc(dictSize+stepSize*3);
            check(temp_cache,HPATCHI_MEM_ERROR,"alloc cache memory");
            if (tuz_OK!=tuz_TStream_open(&tuzStream,patchListener.base.diff_data,patchListener.base.read_diff,
                                            temp_cache+stepSize*2,dictSize,(tuz_size_t)stepSize))
                check(hpatch_FALSE,HPATCHI_OPENDECOMPRESSER_ERROR,"tuz_TStream_open() run");
            patchCacheSize=stepSize*2;
            patchListener.base.diff_data=&tuzStream;
            patchListener.base.read_diff=_do_tuz_decompress;
        }
      #endif // _CompressPlugin_tuz
        default: {
            LOG_ERR("unknow compress_type \"%d\" ERROR!\n",(int)compress_type);
            check(hpatch_FALSE,HPATCHI_COMPRESSTYPE_ERROR,"diff info");
        } }
    }

    patchListener.old_file=&oldData.base;
    patchListener.base.read_old=_do_readOld;
    patchListener.new_file=newData.m_file;
    patchListener.base.write_new=_do_writeNew;
    assert(newData.base.streamSize==(hpi_pos_t)newData.base.streamSize);
    assert(patchCacheSize==(hpi_size_t)patchCacheSize);
    if (!hpatch_lite_patch(&patchListener.base,(hpi_pos_t)newData.base.streamSize,
                           temp_cache,(hpi_size_t)patchCacheSize)){
        check(patchListener.result==HPATCHI_SUCCESS,patchListener.result,"patchListener");
        check(!oldData.fileError,HPATCHI_FILEREAD_ERROR,"oldFile read");
        //check(!diffData.fileError,HPATCHI_FILEREAD_ERROR,"diffFile read");
        check(hpatch_FALSE,HPATCHI_PATCH_ERROR,"hpatch_lite_patch() run");
    }
    printf("  patch ok!\n");
    
clear:
    _isInClear=hpatch_TRUE;
    check(hpatch_TFileStreamOutput_close(&newData),HPATCHI_FILECLOSE_ERROR,"out newFile close");
    check(hpatch_TFileStreamInput_close(&diffData),HPATCHI_FILECLOSE_ERROR,"diffFile close");
    check(hpatch_TFileStreamInput_close(&oldData),HPATCHI_FILECLOSE_ERROR,"oldFile close");
    _free_mem(temp_cache);
    printf("\nhpatchi time: %.3f s\n",(clock_s()-time0));
    return result;
}
