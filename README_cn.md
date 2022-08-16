# [HPatchLite](https://github.com/sisong/HPatchLite)
[![release](https://img.shields.io/badge/release-v0.4.1-blue.svg)](https://github.com/sisong/HPatchLite/releases) 
[![license](https://img.shields.io/badge/license-MIT-blue.svg)](https://github.com/sisong/HPatchLite/blob/main/LICENSE) 
[![PRs Welcome](https://img.shields.io/badge/PRs-welcome-blue.svg)](https://github.com/sisong/HPatchLite/pulls)
[![+issue Welcome](https://img.shields.io/github/issues-raw/sisong/HPatchLite?color=green&label=%2Bissue%20welcome)](https://github.com/sisong/HPatchLite/issues)   

[![Build Status](https://github.com/sisong/HPatchLite/workflows/ci/badge.svg?branch=main)](https://github.com/sisong/HPatchLite/actions?query=workflow%3Aci+branch%3Amain)   

 中文版 | [english](README.md)   

HPatchLite 是 [HDiffPatch](https://github.com/sisong/HDiffPatch) 的一个精简(Lite)版，为在MCU、NB-IoT等小型设备上执行打补丁(patch)功能而优化。   
编译后的patch代码(ROM 占用)非常的小，用 Mbed Studio 编译后为 664 字节(设置宏_IS_RUN_MEM_SAFE_CHECK=0可以节省74字节)。
提示：如果使用了tinyuz并且设置宏_IS_USED_SHARE_hpatch_lite_types=1，可以节省52字节。   
同时，patch时内存(RAM 占用)也可以非常的小，大小为 一个解压缩流需要的内存大小 + patch时输入的缓存区大小(>=3Byte)。
提示：输入缓存区较小时只影响patch速度。    

---
## 二进制发布包
[从 release 下载](https://github.com/sisong/HPatchLite/releases) : 分别运行在 Windows、Linux、MacOS操作系统的命令行程序。     
( 编译出这些发布文件的项目路径在 `HPatchLite/builds` )   

## 自己编译
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
用 [`Visual Studio`](https://visualstudio.microsoft.com) 打开 `HPatchLite/builds/vc/HPatchLite.sln` 编译   

---
## **diff** 命令行使用:  
```
创建补丁: hdiffi [options] oldFile newFile outDiffFile
测试补丁: hdiffi    -t     oldFile newFile testDiffFile
  oldFile可以为空, 输入参数为 ""
内存选项:
  -m[-matchScore]
      所有文件数据都会被加载到内存;
      需要的内存大小:(新版本文件大小+ 旧版本文件大小*5(或*9 当旧版本文件大小>=2GB时))+O(1);
      匹配分数matchScore>=0,默认为6,一般输入数据的可压缩性越大,这个值就可以越大。
其他选项:
  -cache
      给较慢的匹配开启一个大型缓冲区,来加快匹配速度(不影响补丁大小), 默认不开启;
      如果新版本和旧版本不相同数据比较多,那diff速度就会比较快;
      该大型缓冲区最大占用O(旧版本文件大小)的内存, 并且需要较多的时间来创建(从而可能降低diff速度)。
  -p-parallelThreadNumber
      设置线程数parallelThreadNumber>1时,开启多线程并行模式;
      默认为-p-4;需要占用较多的内存。
-c-compressType[-compressLevel]
      设置补丁数据使用的压缩算法和压缩级别等, 默认不压缩;
      支持的压缩算法、压缩级别和字典大小等:
        -c-tuz[-dictSize]               (或者 -tinyuz)
            压缩字典大小dictSize范围1字节到1GB, 默认为 32k；
            也可以设置为 250,511,1k,4k,64k,1m,64m,512m等
        -c-zlib[-{1..9}[-dictBits]]     默认级别 9
            压缩字典比特数dictBits可以为9到15, 默认为15。
        -c-pzlib[-{1..9}[-dictBits]]    默认级别 6
            压缩字典比特数dictBits可以为9到15, 默认为15。
            支持多线程并行压缩,很快！
        -c-lzma[-{0..9}[-dictSize]]     默认级别 7
            压缩字典大小dictSize可以设置为 4096, 4k, 4m, 128m等, 默认为32k
            支持2个线程并行压缩。
        -c-lzma2[-{0..9}[-dictSize]]    默认级别 7
            压缩字典大小dictSize可以设置为 4096, 4k, 4m, 128m等, 默认为32k
            支持多线程并行压缩,很快。
            警告: lzma和lzma2是不同的压缩编码格式。
  -d  只执行diff, 不要执行patch检查, 默认会执行patch检查.
  -t  只执行patch检查, 检查是否 patch(oldFile,testDiffFile)==newFile ?
  -f  强制文件写覆盖, 忽略输出的路径是否已经存在;
      默认不执行覆盖, 如果输出路径已经存在, 直接返回错误;
      如果设置了-f,但路径已经存在并且是一个文件夹, 那么会始终返回错误。
  --patch
      切换到 hpatchi 模式; 可以支持hpatchi命令行的相关参数和功能。
  -v  输出程序版本信息。
  -h 或 -?
      输出命令行帮助信息 (该说明)。
```

## **patch** 命令行使用:  
```
打补丁: hpatchi [options] oldFile diffFile outNewFile
  如果oldFile为空, 输入参数为 ""
options:
  -s[-cacheSize]
      默认设置为-s-32k; 缓冲区cacheSize>=3;
      可以设置为256,1k,60k,1m等
      需要的内存大小: (cacheSize + 1*解压缩缓冲区)+O(1)
  -f  强制文件写覆盖, 忽略输出的路径是否已经存在;
      默认不执行覆盖, 如果输出路径已经存在, 直接返回错误;
      如果设置了-f,但路径已经存在并且是一个文件夹, 那么会始终返回错误。
  -v  输出程序版本信息。
  -h 或 -?
      输出命令行帮助信息 (该说明)。
```

---
## 库 API 使用:
创建 hpatch_lite 补丁:
```
create_lite_diff(newData,OldData,out_lite_diff,compressPlugin,...);
```
应用补丁:
```
hpi_BOOL hpatch_lite_open(hpi_TInputStreamHandle diff_data,hpi_TInputStream_read read_diff,
                          hpi_compressType* out_compress_type,hpi_pos_t* out_newSize,hpi_pos_t* out_uncompressSize);
hpi_BOOL hpatch_lite_patch(hpatchi_listener_t* listener,hpi_pos_t newSize,
                           hpi_byte* temp_cache,hpi_size_t temp_cache_size);
```

---
## Contact
housisong@hotmail.com  