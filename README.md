# [HPatchLite](https://github.com/sisong/HPatchLite)
[![release](https://img.shields.io/badge/release-v0.4.0-blue.svg)](https://github.com/sisong/HPatchLite/releases) 
[![license](https://img.shields.io/badge/license-MIT-blue.svg)](https://github.com/sisong/HPatchLite/blob/main/LICENSE) 
[![PRs Welcome](https://img.shields.io/badge/PRs-welcome-blue.svg)](https://github.com/sisong/HPatchLite/pulls)
[![+issue Welcome](https://img.shields.io/github/issues-raw/sisong/HPatchLite?color=green&label=%2Bissue%20welcome)](https://github.com/sisong/HPatchLite/issues)   

[![Build Status](https://github.com/sisong/HPatchLite/workflows/ci/badge.svg?branch=main)](https://github.com/sisong/HPatchLite/actions?query=workflow%3Aci+branch%3Amain)   

 english | [中文版](README_cn.md)   

HPatchLite is a lite version of [HDiffPatch](https://github.com/sisong/HDiffPatch), tiny code & ram requirements when hpatch on MCU,NB-IoT,...   
The patch code(disk or Flash occupancy) compiled by Mbed Studio is 664 bytes.    
And the patch memory(RAM occupancy) can also be very small, RAM size = one decompress memory size + input cache size(>=3Byte) when patch. Tip: The smaller input cache only affects the patch speed.   

(developmenting & evaluating ...)

## Build it yourself
need submodule libraries [HDiffPatch](https://github.com/sisong/HDiffPatch) and [tinyuz](https://github.com/sisong/tinyuz)
```
$ cd <dir>
$ git clone --recursive https://github.com/sisong/HPatchLite.git
$ cd HPatchLite
$ make
```

---
## **diff** command line usage:  
```
diff    usage: hdiffi [options] oldFile newFile outDiffFile
test    usage: hdiffi    -t     oldFile newFile testDiffFile
  oldFile can empty, and input parameter ""
memory options:
  -m[-matchScore]
      requires (newFileSize+ oldFileSize*5(or *9 when oldFileSize>=2GB))+O(1)
        bytes of memory;
      matchScore>=0, DEFAULT -m-6
special options:
  -cache
      set is use a big cache for slow match, DEFAULT false;
      if newData not similar to oldData then diff speed++,
      big cache max used O(oldFileSize) memory, and build slow(diff speed--)
  -p-parallelThreadNumber
      if parallelThreadNumber>1 then open multi-thread Parallel mode;
      DEFAULT -p-4; requires more memory!
  -c-compressType[-compressLevel]
      set outDiffFile Compress type, DEFAULT uncompress;
      for resave diffFile,recompress diffFile to outDiffFile by new set;
      support compress type & level & dict:
        -c-tuz[-dictSize]               (or -tinyuz)
            1<=dictSize<=1g, can like 250,511,1k,4k,64k,1m,64m,512m..., DEFAULT 32k
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
patch usage: hpatchi [options] oldFile diffFile outNewFile
  if oldFile is empty input parameter ""
options:
  -s[-cacheSize]
      DEFAULT -s-48k; cacheSize>=3, can like 256,1k, 60k or 1m etc....
      requires (cacheSize + 1*decompress buffer size)+O(1) bytes of memory.
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

---
## Contact
housisong@hotmail.com  