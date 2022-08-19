package com.github.sisong;

public class hpatchi{
    // return THPatchiResult, 0 is ok
    //  'diffFileName' file is create by hdiffi app,or by create_lite_diff()
    //  cacheMemory recommended 32*2024 256*1024 ...
    public static native int patch(String oldFileName,String diffFileName,
                                   String outNewFileName,long cacheMemory);
}
