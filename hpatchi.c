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
#   define _CompressPlugin_tuz
#   define _CompressPlugin_zlib
//#   define _CompressPlugin_lzma
//#   define _CompressPlugin_lzma2
#endif

#ifdef _CompressPlugin_tuz
#   include "tuz_dec.h"  // "tinyuz/decompress/tuz_dec.h" https://github.com/sisong/tinyuz
#endif
#ifdef _CompressPlugin_zlib
#   include "zutil.h"  // http://zlib.net/  https://github.com/madler/zlib
#   include "inftrees.h" //for code
#   include "inflate.h" //for inflate_state
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
           "      DEFAULT -s-48k; cacheSize>=3, can like 256,1k, 60k or 1m etc....\n"
           "      requires (cacheSize + 1*decompress buffer size)+O(1) bytes of memory.\n"
           "  -f  Force overwrite, ignore write path already exists;\n"
           "      DEFAULT (no -f) not overwrite and then return error;\n"
           "      if used -f and write path is exist directory, will always return error.\n"
           "  -v  output Version info.\n"
           );
    printHelpInfo();
    printf("\n");
}

typedef enum THPatchiResult {
    HPATCHI_SUCCESS=0,
    HPATCHI_OPTIONS_ERROR,
    HPATCHI_PATHTYPE_ERROR,
    HPATCHI_OPENREAD_ERROR,
    HPATCHI_OPENWRITE_ERROR,
    HPATCHI_FILEREAD_ERROR,// 5
    HPATCHI_FILEWRITE_ERROR, 
    HPATCHI_FILEDATA_ERROR,
    HPATCHI_FILECLOSE_ERROR,
    HPATCHI_MEM_ERROR,
    HPATCHI_COMPRESSTYPE_ERROR, //10
    HPATCHI_DECOMPRESSER_DICT_ERROR,
    HPATCHI_DECOMPRESSER_OPEN_ERROR,
    HPATCHI_DECOMPRESSER_CLOSE_ERROR,
    HPATCHI_PATCH_OPEN_ERROR=20,
    HPATCHI_PATCH_ERROR,
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
#define kPatchCacheSize_default  (1024*16 *3)
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
                _options_check((patchCacheSize==(size_t)_kNULL_VALUE)&&((op[2]=='-')||(op[2]=='\0')),"-s");
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

static hpatch_BOOL _read_empty(const struct hpatch_TStreamInput* stream,hpatch_StreamPos_t readFromPos,
                               unsigned char* out_data,unsigned char* out_data_end){
    return (readFromPos==0)&(out_data==out_data_end);
}
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

#ifdef _CompressPlugin_zlib
    typedef struct _zlib_TStream{
        hpi_TInputStreamHandle  code_stream;
        hpi_TInputStream_read   read_code;
        hpi_pos_t               uncompressSize;
        unsigned char*  zlib_mem;
        size_t          zlib_mem_size;
        
        unsigned char*  dec_buf;
        hpi_size_t      dec_buf_size;
        z_stream        d_stream;
    } _zlib_TStream;

static voidpf _zlib_TStream_malloc OF((voidpf opaque, uInt items, uInt size)){
    _zlib_TStream* self=(_zlib_TStream*)opaque;
    size_t alloc_size=items*(size_t)size;
    voidpf result=self->zlib_mem;
    if (alloc_size>self->zlib_mem_size)
        return 0;
    self->zlib_mem+=alloc_size;
    self->zlib_mem_size-=alloc_size;
    return result;
}
static void _zlib_TStream_free OF((voidpf opaque, voidpf address)) { }

    static hpatch_BOOL __zlib_TStream_inflate(_zlib_TStream* self){
        int ret;
        uInt avail_out_back;
        hpi_size_t avail_in_back=self->d_stream.avail_in;
        if (avail_in_back==0) {
            avail_in_back=self->dec_buf_size;
            if (!self->read_code(self->code_stream,self->dec_buf,&avail_in_back))
                return hpatch_FALSE;//error;
            assert(avail_in_back==(uInt)avail_in_back);
            self->d_stream.avail_in=(uInt)avail_in_back;
            self->d_stream.next_in=self->dec_buf;
        }
        
        avail_out_back=self->d_stream.avail_out;
        ret=inflate(&self->d_stream,Z_NO_FLUSH);
        if (ret==Z_OK){
            if ((self->d_stream.avail_in==avail_in_back)&&(self->d_stream.avail_out==avail_out_back))
                return hpatch_FALSE;//error;
        }else if (ret==Z_STREAM_END){//all end
            if (self->d_stream.avail_out!=0)
                return hpatch_FALSE;//error;
        }else{
            return hpatch_FALSE;//error;
        }
        return hpatch_TRUE;
    }
static hpi_BOOL _zlib_TStream_decompress(hpi_TInputStreamHandle diffStream,hpi_byte* out_data,hpi_size_t* data_size){
    _zlib_TStream* self=(_zlib_TStream*)diffStream;
    hpi_pos_t r_size=*data_size;
    if (r_size>self->uncompressSize){
        r_size=self->uncompressSize;
        *data_size=(hpi_pos_t)r_size;
    }
    self->uncompressSize-=r_size;
    self->d_stream.next_out=out_data;
    self->d_stream.avail_out=(uInt)r_size;
    assert(self->d_stream.avail_out==r_size);
    while (self->d_stream.avail_out>0) {
        if (!__zlib_TStream_inflate(self))
            return hpatch_FALSE;//error;
    }
    return hpatch_TRUE;
}
#endif

int hpatchi(const char* oldFileName,const char* diffFileName,const char* outNewFileName,size_t patchCacheSize){
    int     result=HPATCHI_SUCCESS;
    int     _isInClear=hpatch_FALSE;
    double  time0=clock_s();
    hpatch_TFileStreamOutput    newData;
    hpatch_TFileStreamInput     diffData;
    hpatch_TFileStreamInput     oldData;
    hpi_byte*           pmem=0;
    hpi_byte*           temp_cache;
    TPatchListener      patchListener;
    hpi_compressType    compress_type;
#ifdef _CompressPlugin_tuz
    tuz_TStream         tuzStream;
#endif
#ifdef _CompressPlugin_zlib
    _zlib_TStream       zlibStream;
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
            oldData.base.read=_read_empty;
        }else{
            check(hpatch_TFileStreamInput_open(&oldData,oldFileName),
                  HPATCHI_OPENREAD_ERROR,"open oldFile for read");
        }
        check(hpatch_TFileStreamInput_open(&diffData,diffFileName),
              HPATCHI_OPENREAD_ERROR,"open diffFile for read");
    }
    printf("oldDataSize : %" PRIu64 "\ndiffDataSize: %" PRIu64 "\n",
           oldData.base.streamSize,diffData.base.streamSize);

    patchListener.base.diff_data=diffData.m_file;
    patchListener.base.read_diff=_do_readFile;
    {//open diff info
        hpi_pos_t newSize;
        hpi_pos_t uncompressSize;
        if (!hpatch_lite_open(patchListener.base.diff_data,patchListener.base.read_diff,
                              &compress_type,&newSize,&uncompressSize))
            check(hpatch_FALSE,HPATCHI_PATCH_OPEN_ERROR,"hpatch_lite_open() run");

        printf("newDataSize : %" PRIu64 "\n",(hpatch_StreamPos_t)newSize);
        check(hpatch_TFileStreamOutput_open(&newData,outNewFileName,newSize),
              HPATCHI_OPENWRITE_ERROR,"open out newFile for write");

        switch (compress_type){
        case hpi_compressType_no: {
            printf("hpatchi run with decompresser: \"\"\n");
            pmem=(hpi_byte*)malloc(patchCacheSize);
            check(pmem,HPATCHI_MEM_ERROR,"alloc cache memory");
            temp_cache=pmem;
        } break;
    #ifdef _CompressPlugin_tuz
        case hpi_compressType_tuz: {
            size_t allMemSize;
            const size_t stepSize=patchCacheSize/3;
            const tuz_size_t dictSize=tuz_TStream_read_dict_size(patchListener.base.diff_data,patchListener.base.read_diff);
            printf("hpatchi run with decompresser: \"tuz\"\n");
            check(((tuz_size_t)(dictSize-1))<tuz_kMaxOfDictSize,HPATCHI_DECOMPRESSER_DICT_ERROR,"tuz_TStream_read_dict_size() dict size");
            assert(stepSize==(tuz_size_t)stepSize);
            allMemSize=dictSize+stepSize*3;
            pmem=(hpi_byte*)malloc(allMemSize);
            check(pmem,HPATCHI_MEM_ERROR,"alloc cache memory");
            temp_cache=pmem;
            check(tuz_OK==tuz_TStream_open(&tuzStream,patchListener.base.diff_data,patchListener.base.read_diff,
                                           temp_cache,dictSize,(tuz_size_t)stepSize),
                  HPATCHI_DECOMPRESSER_OPEN_ERROR,"tuz_TStream_open()");
            temp_cache+=stepSize+dictSize;

            patchCacheSize=stepSize*2;
            patchListener.base.diff_data=&tuzStream;
            patchListener.base.read_diff=_do_tuz_decompress;
        } break;
    #endif // _CompressPlugin_tuz
    #ifdef _CompressPlugin_zlib
        case hpi_compressType_zlib: {
            size_t allMemSize;
            const size_t stepSize=patchCacheSize/3;
            int         dictBits;
            signed char windowBits;
            hpi_size_t  rlen=1;
            printf("hpatchi run with decompresser: \"zlib\"\n");
            check(patchListener.base.read_diff(patchListener.base.diff_data,(hpi_byte*)&windowBits,&rlen)
                  &&(rlen==1),HPATCHI_DECOMPRESSER_DICT_ERROR,"read windowBits from diffData");
            dictBits=(int)windowBits;
            if (dictBits<0) dictBits=-dictBits;
            if (dictBits>15) dictBits-=16;
            check((dictBits>=9)&(dictBits<=15),HPATCHI_DECOMPRESSER_DICT_ERROR,
                  "readed windowBits value from diffData");
            
            memset(&zlibStream,0,sizeof(zlibStream));
            zlibStream.code_stream=patchListener.base.diff_data;
            zlibStream.read_code=patchListener.base.read_diff;
            zlibStream.uncompressSize=uncompressSize;
            zlibStream.d_stream.opaque=&zlibStream;
            zlibStream.d_stream.zalloc=_zlib_TStream_malloc;
            zlibStream.d_stream.zfree=_zlib_TStream_free;

            assert(stepSize==(hpi_size_t)stepSize);
            zlibStream.zlib_mem_size=sizeof(struct inflate_state)+((size_t)1<<dictBits);
            allMemSize=zlibStream.zlib_mem_size+stepSize*3;
            pmem=(hpi_byte*)malloc(allMemSize);
            check(pmem,HPATCHI_MEM_ERROR,"alloc cache memory");
            temp_cache=pmem;

            zlibStream.zlib_mem=temp_cache;
            temp_cache+=zlibStream.zlib_mem_size;
            zlibStream.dec_buf=temp_cache;
            zlibStream.dec_buf_size=(hpi_size_t)stepSize;
            temp_cache+=stepSize;

            check(Z_OK==inflateInit2(&zlibStream.d_stream,(int)windowBits),
                  HPATCHI_DECOMPRESSER_OPEN_ERROR,"zlib inflateInit2()");

            patchCacheSize=stepSize*2;
            patchListener.base.diff_data=&zlibStream;
            patchListener.base.read_diff=_zlib_TStream_decompress;
        } break;
    #endif // _CompressPlugin_zlib
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
#ifdef _CompressPlugin_zlib
    if ((hpi_compressType_zlib==compress_type)&&(zlibStream.d_stream.state!=0)){
        check(Z_OK==inflateEnd(&zlibStream.d_stream),HPATCHI_DECOMPRESSER_CLOSE_ERROR,"zlib inflateEnd()");
    }
#endif
    _free_mem(pmem);
    printf("\nhpatchi time: %.3f s\n",(clock_s()-time0));
    return result;
}
