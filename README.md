# [HPatchLite](https://github.com/sisong/HPatchLite)
[![release](https://img.shields.io/badge/release-v1.0.0-blue.svg)](https://github.com/sisong/HPatchLite/releases) 
[![license](https://img.shields.io/badge/license-MIT-blue.svg)](https://github.com/sisong/HPatchLite/blob/main/LICENSE) 
[![PRs Welcome](https://img.shields.io/badge/PRs-welcome-blue.svg)](https://github.com/sisong/HPatchLite/pulls)
[![+issue Welcome](https://img.shields.io/github/issues-raw/sisong/HPatchLite?color=green&label=%2Bissue%20welcome)](https://github.com/sisong/HPatchLite/issues)   
[![Build Status](https://github.com/sisong/HPatchLite/workflows/ci/badge.svg?branch=main)](https://github.com/sisong/HPatchLite/actions?query=workflow%3Aci+branch%3Amain)   
 english | [中文版](README_cn.md)   

HPatchLite is a lite version of [HDiffPatch](https://github.com/sisong/HDiffPatch), tiny code & ram requirements when patch on embedded systems,MCU,NB-IoT,...   
HPatchLite also supports a simple inplace-patch implementation to support storage-constrained devices.   

The patch code(ROM or flash occupancy) very small, compiled by Mbed Studio is 662 bytes.    
Tip: *if define _IS_RUN_MEM_SAFE_CHECK=0, unsafe mode can reduce 48 bytes; 
if used [tinyuz](https://github.com/sisong/tinyuz) & define _IS_USED_SHARE_hpatch_lite_types=1, can reduce 52 bytes.*   
   
At the same time, the patch memory(RAM occupancy) can also be very small, 
RAM size = one decompress memory size + input cache size(>=3Byte) when patch.    
Tip: *The smaller input cache only affects the patch speed.*   

---
## Releases/Binaries
[Download from latest release](https://github.com/sisong/HPatchLite/releases) : Command line app for Windows, Linux, MacOS.     

## Build it yourself
### Linux or MacOS X ###
```
$ cd <dir>
$ git clone --recursive https://github.com/sisong/HPatchLite.git
$ cd HPatchLite
$ make
```

### Windows ###
```
$ cd <dir>
$ git clone --recursive https://github.com/sisong/HPatchLite.git
```
build `HPatchLite/builds/vc/HPatchLite.sln` with [`Visual Studio`](https://visualstudio.microsoft.com)   

---
## **diff** command line usage:  
```
diff    usage: hdiffi [options] oldFile newFile outDiffFile
test    usage: hdiffi    -t     oldFile newFile testDiffFile
  oldFile can empty, and input parameter ""
options:
  -m[-matchScore]
      requires (newFileSize+ oldFileSize*5(or *9 when oldFileSize>=2GB))+O(1)
        bytes of memory;
      matchScore>=0, DEFAULT -m-6
  -inplace[-extraSafeSize]
      open create inplace-patch mode, DEFAULT closed;
      extraSafeSize: extra safe access distances for inplace-patch;
      extraSafeSize>=0, DEFAULT 0;
      if extraSafeSize>0, need use extra space to cache new data when patch,
        thus increasing the chance of matchmaking when diff, also increases
        the memory usage by the same amount when patch and increases the
        complexity of patch code.
  -cache
      set is use a big cache for slow match, DEFAULT false;
      if newData not similar to oldData then diff speed++,
      big cache max used O(oldFileSize) memory, and build slow(diff speed--)
  -p-parallelThreadNumber
      if parallelThreadNumber>1 then open multi-thread Parallel mode;
      DEFAULT -p-4; requires more memory!
  -c-compressType[-compressLevel]
      set outDiffFile Compress type, DEFAULT uncompress;
      support compress type & level & dict:
        -c-tuz[-dictSize]               (or -c-tinyuz)
        -c-tuzi[-dictSize]              (or -c-tinyuzi)
            1<=dictSize<=1g, can like 250,511,1k,4k,64k,1m,64m..., DEFAULT 32k
            Note: -c-tuz is default compressor;
            But if your compile tinyuz deccompressor source code by yourself,
             & set tuz_isNeedLiteralLine=0, then must used -c-tuzi compressor.
        -c-zlib[-{1..9}[-dictBits]]     DEFAULT level 9
            dictBits can 9--15, DEFAULT 15.
        -c-pzlib[-{1..9}[-dictBits]]    DEFAULT level 6
            dictBits can 9--15, DEFAULT 15.
            support run by multi-thread parallel, fast!
        -c-lzma[-{0..9}[-dictSize]]     DEFAULT level 7
            dictSize can like 4096 or 4k or 4m or 128m etc..., DEFAULT 32k
            support run by 2-thread parallel.
        -c-lzma2[-{0..9}[-dictSize]]    DEFAULT level 7
            dictSize can like 4096 or 4k or 4m or 128m etc..., DEFAULT 32k
            support run by multi-thread parallel, fast!
            WARNING: code not compatible with it compressed by -c-lzma!
  -d  Diff only, do't run patch check, DEFAULT run patch check.
  -t  Test only, run patch check, patch(oldFile,testDiffFile)==newFile ?
  -f  Force overwrite, ignore write path already exists;
      DEFAULT (no -f) not overwrite and then return error;
      if used -f and write path is exist directory, will always return error.
  --patch
      swap to hpatchi mode.
  -v  output Version info.
  -h (or -?)
      output usage info.
```

## **patch** command line usage:  
```
patch   usage: hpatchi [options] oldFile diffFile outNewFile
inplace-patch: hpatchi [options] oldFile diffFile -INPLACE
  if oldFile is empty input parameter ""
options:
  -s[-cacheSize]
      DEFAULT -s-32k; cacheSize>=3, can like 256,1k, 60k or 1m etc....
      requires (cacheSize + 1*decompress buffer size [+ extraSafeSize for inplace-patch])+O(1) bytes of memory.
  -INPLACE
      open inplace-patch mode, DEFAULT closed;
      WANING: oldFile will be modified to newFile when inplace-patch successful,
           and it will be damaged & cannot be recovered when inplace-patch fail.
  -f  Force overwrite, ignore write path already exists;
      DEFAULT (no -f) not overwrite and then return error;
      if used -f and write path is exist directory, will always return error.
  -v  output Version info.
  -h  (or -?)
      output usage info.
```

---
## library API usage:
create hpatch_lite diff:
```
create_lite_diff(newData,OldData,out_lite_diff,compressPlugin,...);
```
apply patch:
```
hpi_BOOL hpatch_lite_open(hpi_TInputStreamHandle diff_data,hpi_TInputStream_read read_diff,
                          hpi_compressType* out_compress_type,hpi_pos_t* out_newSize,hpi_pos_t* out_uncompressSize);
hpi_BOOL hpatch_lite_patch(hpatchi_listener_t* listener,hpi_pos_t newSize,
                           hpi_byte* temp_cache,hpi_size_t temp_cache_size);
```
create inplaceB diff:
```
create_inplaceB_lite_diff(newData,OldData,out_lite_diff,extraSafeSize,compressPlugin,...);
```
apply inplaceB patch:
```
hpi_BOOL hpatchi_inplace_open(hpi_TInputStreamHandle diff_data,hpi_TInputStream_read read_diff,
                              hpi_compressType* out_compress_type,hpi_pos_t* out_newSize,
                              hpi_pos_t* out_uncompressSize,hpi_size_t* out_extraSafeSize);
hpi_BOOL hpatchi_inplaceB(hpatchi_listener_t* listener,hpi_pos_t newSize,
                          hpi_byte* temp_cache,hpi_size_t extraSafeSize_in_temp_cache,hpi_size_t temp_cache_size);
```

---
## Porting patch algorithm to embedded devices:
* Add or copy the entire directory `HDiffPatch/libHDiffPatch/HPatchLite/` to your project;
* Add a reference to the `hpatch_lite.h` header file where the patch algorithm is needed, and call the patch functions declared in this file.
* Key points of the code: 
  1. Use the `hpatch_lite_open` function to open the patch data, diff_data is the handle pointing to the patch data, and read_diff is the function that reads the patch data sequentially through the handle.
  1. After opening the patch, you can get the compression algorithm type compress_type used by the patch and the amount of data that can be decompressed uncompressSize; if there is compression, then use the corresponding decompression algorithm to wrap diff_data and read_diff to get new diff_data and read_diff that can read uncompressed data;
  1. Fill an hpatchi_listener_t structure to call `hpatch_lite_patch`; read_old is used to randomly read the old version of the data, and write_new is used to sequentially write the new version of the newSize size of data.
  1. If inplace-patch are used, replace `hpatch_lite_open` with `hpatchi_inplace_open` and `hpatch_lite_patch` with `hpatchi_inplaceB`. When alloc memory for temp_cache, memory size need to add **extraSafeSize**. (best practices: when needed, after obtaining extraSafeSize, you can add an min-write block size of storage chips to extraSafeSize, this simplifies the implementation of read_old & write_new.)

---
## Contact
housisong@hotmail.com  