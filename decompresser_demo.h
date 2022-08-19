//decompresser_demo.h
// decompresser demo for HPatchLite
//
/*
 The MIT License (MIT)
 Copyright (c) 2020-2022 HouSisong All Rights Reserved.
 */
#ifndef hpatchi_decompresser_demo_h
#define hpatchi_decompresser_demo_h

#ifndef _IsNeedIncludeDefaultCompressHead
#   define _IsNeedIncludeDefaultCompressHead 1
#endif


#ifdef _CompressPlugin_tuz
#if (_IsNeedIncludeDefaultCompressHead)
#   include "tuz_dec.h"  // "tinyuz/decompress/tuz_dec.h" https://github.com/sisong/tinyuz
#endif
    static size_t _tuz_TStream_getReservedMemSize(hpi_TInputStreamHandle codeStream,hpi_TInputStream_read readCode){
        const tuz_size_t dictSize=tuz_TStream_read_dict_size(codeStream,readCode);
        if (((tuz_size_t)(dictSize-1))>=tuz_kMaxOfDictSize)
            return 0;//error
        return dictSize;
    }
    static hpi_BOOL _tuz_TStream_decompress(hpi_TInputStreamHandle diffStream,hpi_byte* out_part_data,hpi_size_t* data_size){
        return tuz_STREAM_END>=tuz_TStream_decompress_partial((tuz_TStream*)diffStream,out_part_data,data_size);
    }
#endif //_CompressPlugin_tuz

#ifdef _CompressPlugin_zlib
#if (_IsNeedIncludeDefaultCompressHead)
#define Byte __Byte
#   include "zlib.h" // http://zlib.net/  https://github.com/madler/zlib
#undef Byte
#define ZLIB_INTERNAL 
#   include "inftrees.h" //for inflate_state::code 
#undef ZLIB_INTERNAL
#   include "inflate.h" //for inflate_state
#endif
    typedef struct zlib_TStream{
        hpi_TInputStreamHandle  codeStream;
        hpi_TInputStream_read   readCode;
        hpi_pos_t               uncompressSize;
        unsigned char*  reservedMem;
        size_t          reservedMemSize;
        
        unsigned char*  dec_buf;
        hpi_size_t      dec_buf_size;
        z_stream        d_stream;
        signed char     windowBits;
    } zlib_TStream;

    static voidpf __zlib_TStream_malloc OF((voidpf opaque, uInt items, uInt size)){
        zlib_TStream* self=(zlib_TStream*)opaque;
        size_t alloc_size=items*(size_t)size;
        voidpf result=self->reservedMem;
        if (alloc_size>self->reservedMemSize)
            return 0;
        self->reservedMem+=alloc_size;
        self->reservedMemSize-=alloc_size;
        return result;
    }
    static void __zlib_TStream_free OF((voidpf opaque, voidpf address)) { }

    static void _zlib_TStream_init(zlib_TStream* self,hpi_pos_t uncompressSize,
                                   hpi_TInputStreamHandle codeStream,hpi_TInputStream_read readCode){
        memset(self,0,sizeof(*self));
        self->codeStream=codeStream;
        self->readCode=readCode;
        self->uncompressSize=uncompressSize;
        self->d_stream.opaque=self;
        self->d_stream.zalloc=__zlib_TStream_malloc;
        self->d_stream.zfree=__zlib_TStream_free;
    }

    static size_t _zlib_TStream_getReservedMemSize(zlib_TStream* self){
        int dictBits;
        hpi_size_t  rlen=1;
        if (!(self->readCode(self->codeStream,(hpi_byte*)&self->windowBits,&rlen)&&(rlen==1)))
            return 0;//error
        dictBits=(int)self->windowBits;
        if (dictBits<0) dictBits=-dictBits;
        if (dictBits>15) dictBits-=16;
        if ((dictBits<9)|(dictBits>15))
            return 0;//error
        self->reservedMemSize=sizeof(struct inflate_state)+((size_t)1<<dictBits);
        return self->reservedMemSize;
    }
    static hpi_BOOL _zlib_TStream_open(zlib_TStream* self,hpi_byte* buf,size_t bufSize){
        assert(bufSize>self->reservedMemSize);
        self->reservedMem=buf;
        self->dec_buf=buf+self->reservedMemSize;
        self->dec_buf_size=(hpi_size_t)(bufSize-self->reservedMemSize);
        return Z_OK==inflateInit2(&self->d_stream,(int)self->windowBits);
    }
    static hpi_BOOL _zlib_TStream_close(zlib_TStream* self){
        if (self->d_stream.state!=0)
            return Z_OK==inflateEnd(&self->d_stream);
        else
            return hpi_TRUE;
    }

    static hpatch_BOOL __zlib_TStream_inflate(zlib_TStream* self){
        int ret;
        uInt avail_out_back;
        hpi_size_t avail_in_back=self->d_stream.avail_in;
        if (avail_in_back==0) {
            avail_in_back=self->dec_buf_size;
            if (!self->readCode(self->codeStream,self->dec_buf,&avail_in_back))
                return hpi_FALSE;//error;
            assert(avail_in_back==(uInt)avail_in_back);
            self->d_stream.avail_in=(uInt)avail_in_back;
            self->d_stream.next_in=self->dec_buf;
        }
        
        avail_out_back=self->d_stream.avail_out;
        ret=inflate(&self->d_stream,Z_NO_FLUSH);
        if (ret==Z_OK){
            if ((self->d_stream.avail_in==avail_in_back)&&(self->d_stream.avail_out==avail_out_back))
                return hpi_FALSE;//error;
        }else if (ret==Z_STREAM_END){//all end
            if (self->d_stream.avail_out!=0)
                return hpi_FALSE;//error;
        }else{
            return hpi_FALSE;//error;
        }
        return hpi_TRUE;
    }
    static hpi_BOOL _zlib_TStream_decompress(hpi_TInputStreamHandle diffStream,hpi_byte* out_part_data,hpi_size_t* data_size){
        zlib_TStream* self=(zlib_TStream*)diffStream;
        hpi_pos_t r_size=*data_size;
        if (r_size>self->uncompressSize){
            r_size=self->uncompressSize;
            *data_size=(hpi_pos_t)r_size;
        }
        self->uncompressSize-=r_size;
        self->d_stream.next_out=out_part_data;
        self->d_stream.avail_out=(uInt)r_size;
        assert(self->d_stream.avail_out==r_size);
        while (self->d_stream.avail_out>0) {
            if (!__zlib_TStream_inflate(self))
                return hpi_FALSE;//error;
        }
        return hpi_TRUE;
    }
#endif //_CompressPlugin_zlib


#ifdef _CompressPlugin_lzma
#if (_IsNeedIncludeDefaultCompressHead)
#   include "LzmaDec.h" // "lzma/C/LzmaDec.h" https://github.com/sisong/lzma
#endif
    typedef struct lzma_TStream{
        ISzAlloc        alloc_base;
        hpi_TInputStreamHandle  codeStream;
        hpi_TInputStream_read   readCode;
        hpi_pos_t               uncompressSize;
        unsigned char*  reservedMem;
        size_t          reservedMemSize;
        
        unsigned char*  dec_buf;
        hpi_size_t      dec_buf_size;
        CLzmaDec        decEnv;
        SizeT           decCopyPos;
        SizeT           decReadPos;
        unsigned char   propsSize;
        unsigned char   props[8];
    } lzma_TStream;

    static void * __lzma_dec_Alloc_sumSize(ISzAllocPtr p, size_t size){
        lzma_TStream* self=(lzma_TStream*)p;
        self->reservedMemSize+=size;
        return (void*)1;
    }
    static void * __lzma_dec_Alloc(ISzAllocPtr p, size_t size){
        lzma_TStream* self=(lzma_TStream*)p;
        void* result=self->reservedMem;
        if (size>self->reservedMemSize)
            return 0;
        self->reservedMem+=size;
        self->reservedMemSize-=size;
        return result;
    }
    static void __lzma_dec_Free(ISzAllocPtr p, void *address){ }

    static void _lzma_TStream_init(lzma_TStream* self,hpi_pos_t uncompressSize,
                                   hpi_TInputStreamHandle codeStream,hpi_TInputStream_read readCode){
        memset(self,0,sizeof(*self));
        self->codeStream=codeStream;
        self->readCode=readCode;
        self->uncompressSize=uncompressSize;
        self->alloc_base.Free=__lzma_dec_Free;
    }
    static size_t _lzma_TStream_getReservedMemSize(lzma_TStream* self){
        SRes res;
        hpi_size_t  rlen=1;
        unsigned char propsSize;
        unsigned char* props=self->props;
        if (!(self->readCode(self->codeStream,&propsSize,&rlen)&&(rlen==1)))
            return 0;//error
        self->propsSize=propsSize;
        rlen=propsSize;
        if (((hpi_size_t)(rlen-1))>=sizeof(self->props))
            return 0;//error
        if (!(self->readCode(self->codeStream,props,&rlen)&&(rlen==propsSize)))
            return 0;//error
        self->alloc_base.Alloc=__lzma_dec_Alloc_sumSize;
        res=LzmaDec_Allocate(&self->decEnv,props,rlen,&self->alloc_base);
        memset(&self->decEnv,0,sizeof(self->decEnv));
        if (SZ_OK!=res)
            return 0;//error
        return self->reservedMemSize;
    }
    static hpi_BOOL _lzma_TStream_open(lzma_TStream* self,hpi_byte* buf,size_t bufSize){
        assert(bufSize>self->reservedMemSize);
        self->reservedMem=buf;
        self->dec_buf=buf+self->reservedMemSize;
        self->dec_buf_size=(hpi_size_t)(bufSize-self->reservedMemSize);
        //self->decCopyPos=0;
        self->decReadPos=self->dec_buf_size;
        
        self->alloc_base.Alloc=__lzma_dec_Alloc;
        if (SZ_OK!=LzmaDec_Allocate(&self->decEnv,self->props,self->propsSize,&self->alloc_base))
            return hpi_FALSE;//error
        LzmaDec_Init(&self->decEnv);
        return hpi_TRUE;
    }
    static hpi_BOOL _lzma_TStream_close(lzma_TStream* self){
        LzmaDec_Free(&self->decEnv,&self->alloc_base);
        return hpi_TRUE;
    }

    static hpi_BOOL _lzma_TStream_decompress(hpi_TInputStreamHandle diffStream,hpi_byte* out_part_data,hpi_size_t* data_size){
        hpi_byte* out_part_data_end;
        unsigned char* out_cur;
        hpi_BOOL isReadedEnd=hpi_FALSE;
        lzma_TStream* self=(lzma_TStream*)diffStream;
        hpi_pos_t r_size=*data_size;
        if (r_size>self->uncompressSize){
            r_size=self->uncompressSize;
            *data_size=(hpi_pos_t)r_size;
        }
        self->uncompressSize-=r_size;
        out_part_data_end=out_part_data+r_size;

        out_cur=out_part_data;
        while (out_cur<out_part_data_end){
            size_t copyLen=(self->decEnv.dicPos-self->decCopyPos);
            if (copyLen>0){
                if (copyLen>(size_t)(out_part_data_end-out_cur))
                    copyLen=(out_part_data_end-out_cur);
                memcpy(out_cur,self->decEnv.dic+self->decCopyPos,copyLen);
                out_cur+=copyLen;
                self->decCopyPos+=copyLen;
                if ((self->decEnv.dicPos==self->decEnv.dicBufSize)
                    &&(self->decEnv.dicPos==self->decCopyPos)){
                    self->decEnv.dicPos=0;
                    self->decCopyPos=0;
                }
            }else{
                ELzmaStatus status;
                SizeT inSize,dicPos_back;
                SRes res;
                if ((self->decReadPos==self->dec_buf_size)&(!isReadedEnd)) {
                    hpi_size_t readLen=self->dec_buf_size;
                    if (!self->readCode(self->codeStream,self->dec_buf,&readLen))
                        return hpi_FALSE;//error;
                    self->decReadPos=self->dec_buf_size-readLen;
                    if (self->decReadPos>0){
                        isReadedEnd=hpi_TRUE;
                        memmove(self->dec_buf+self->decReadPos,self->dec_buf,readLen);
                    }else{
                        self->decReadPos=0;
                    }
                }

                inSize=self->dec_buf_size-self->decReadPos;
                dicPos_back=self->decEnv.dicPos;
                res=LzmaDec_DecodeToDic(&self->decEnv,self->decEnv.dicBufSize,
                                        self->dec_buf+self->decReadPos,&inSize,LZMA_FINISH_ANY,&status);
                if(res==SZ_OK){
                    if ((inSize==0)&&(self->decEnv.dicPos==dicPos_back))
                        return hpi_FALSE;//error;
                }else{
                    return hpi_FALSE;//error;
                }
                self->decReadPos+=inSize;
            }
        }
        return hpi_TRUE;
    }
#endif //_CompressPlugin_lzma

#ifdef _CompressPlugin_lzma2
#if (_IsNeedIncludeDefaultCompressHead)
#   include "LzmaDec.h" // "lzma/C/LzmaDec.h" https://github.com/sisong/lzma
#   include "Lzma2Dec.h" 
#endif
    typedef struct lzma2_TStream{
        ISzAlloc        alloc_base;
        hpi_TInputStreamHandle  codeStream;
        hpi_TInputStream_read   readCode;
        hpi_pos_t               uncompressSize;
        unsigned char*  reservedMem;
        size_t          reservedMemSize;
        
        unsigned char*  dec_buf;
        hpi_size_t      dec_buf_size;
        CLzma2Dec       decEnv;
        SizeT           decCopyPos;
        SizeT           decReadPos;
        unsigned char   prop;
    } lzma2_TStream;

    static void * __lzma2_dec_Alloc_sumSize(ISzAllocPtr p, size_t size){
        lzma2_TStream* self=(lzma2_TStream*)p;
        self->reservedMemSize+=size;
        return (void*)1;
    }
    static void * __lzma2_dec_Alloc(ISzAllocPtr p, size_t size){
        lzma2_TStream* self=(lzma2_TStream*)p;
        void* result=self->reservedMem;
        if (size>self->reservedMemSize)
            return 0;
        self->reservedMem+=size;
        self->reservedMemSize-=size;
        return result;
    }
    static void __lzma2_dec_Free(ISzAllocPtr p, void *address){ }

    static void _lzma2_TStream_init(lzma2_TStream* self,hpi_pos_t uncompressSize,
                                    hpi_TInputStreamHandle codeStream,hpi_TInputStream_read readCode){
        memset(self,0,sizeof(*self));
        self->codeStream=codeStream;
        self->readCode=readCode;
        self->uncompressSize=uncompressSize;
        self->alloc_base.Free=__lzma2_dec_Free;
    }
    static size_t _lzma2_TStream_getReservedMemSize(lzma2_TStream* self){
        SRes res;
        hpi_size_t  rlen=1;
        if (!(self->readCode(self->codeStream,&self->prop,&rlen)&&(rlen==1)))
            return 0;//error
        self->alloc_base.Alloc=__lzma2_dec_Alloc_sumSize;
        res=Lzma2Dec_Allocate(&self->decEnv,self->prop,&self->alloc_base);
        memset(&self->decEnv,0,sizeof(self->decEnv));
        if (SZ_OK!=res)
            return 0;//error
        return self->reservedMemSize;
    }
    static hpi_BOOL _lzma2_TStream_open(lzma2_TStream* self,hpi_byte* buf,size_t bufSize){
        assert(bufSize>self->reservedMemSize);
        self->reservedMem=buf;
        self->dec_buf=buf+self->reservedMemSize;
        self->dec_buf_size=(hpi_size_t)(bufSize-self->reservedMemSize);
        //self->decCopyPos=0;
        self->decReadPos=self->dec_buf_size;
        
        self->alloc_base.Alloc=__lzma2_dec_Alloc;
        if (SZ_OK!=Lzma2Dec_Allocate(&self->decEnv,self->prop,&self->alloc_base))
            return hpi_FALSE;//error
        Lzma2Dec_Init(&self->decEnv);
        return hpi_TRUE;
    }
    static hpi_BOOL _lzma2_TStream_close(lzma2_TStream* self){
        Lzma2Dec_Free(&self->decEnv,&self->alloc_base);
        return hpi_TRUE;
    }

    static hpi_BOOL _lzma2_TStream_decompress(hpi_TInputStreamHandle diffStream,hpi_byte* out_part_data,hpi_size_t* data_size){
        hpi_byte* out_part_data_end;
        unsigned char* out_cur;
        hpi_BOOL isReadedEnd=hpi_FALSE;
        lzma2_TStream* self=(lzma2_TStream*)diffStream;
        hpi_pos_t r_size=*data_size;
        if (r_size>self->uncompressSize){
            r_size=self->uncompressSize;
            *data_size=(hpi_pos_t)r_size;
        }
        self->uncompressSize-=r_size;
        out_part_data_end=out_part_data+r_size;

        out_cur=out_part_data;
        while (out_cur<out_part_data_end){
            size_t copyLen=(self->decEnv.decoder.dicPos-self->decCopyPos);
            if (copyLen>0){
                if (copyLen>(size_t)(out_part_data_end-out_cur))
                    copyLen=(out_part_data_end-out_cur);
                memcpy(out_cur,self->decEnv.decoder.dic+self->decCopyPos,copyLen);
                out_cur+=copyLen;
                self->decCopyPos+=copyLen;
                if ((self->decEnv.decoder.dicPos==self->decEnv.decoder.dicBufSize)
                    &&(self->decEnv.decoder.dicPos==self->decCopyPos)){
                    self->decEnv.decoder.dicPos=0;
                    self->decCopyPos=0;
                }
            }else{
                ELzmaStatus status;
                SizeT inSize,dicPos_back;
                SRes res;
                if ((self->decReadPos==self->dec_buf_size)&(!isReadedEnd)) {
                    hpi_size_t readLen=self->dec_buf_size;
                    if (!self->readCode(self->codeStream,self->dec_buf,&readLen))
                        return hpi_FALSE;//error;
                    self->decReadPos=self->dec_buf_size-readLen;
                    if (self->decReadPos>0){
                        isReadedEnd=hpi_TRUE;
                        memmove(self->dec_buf+self->decReadPos,self->dec_buf,readLen);
                    }else{
                        self->decReadPos=0;
                    }
                }

                inSize=self->dec_buf_size-self->decReadPos;
                dicPos_back=self->decEnv.decoder.dicPos;
                res=Lzma2Dec_DecodeToDic(&self->decEnv,self->decEnv.decoder.dicBufSize,
                                        self->dec_buf+self->decReadPos,&inSize,LZMA_FINISH_ANY,&status);
                if(res==SZ_OK){
                    if ((inSize==0)&&(self->decEnv.decoder.dicPos==dicPos_back))
                        return hpi_FALSE;//error;
                }else{
                    return hpi_FALSE;//error;
                }
                self->decReadPos+=inSize;
            }
        }
        return hpi_TRUE;
    }
#endif //_CompressPlugin_lzma2

#endif //hpatchi_decompresser_demo_h