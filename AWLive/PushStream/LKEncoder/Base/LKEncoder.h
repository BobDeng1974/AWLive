/**
 * file :	LKEncoder.h
 * author :	Rex
 * create :	2017-02-17 16:59
 * func : 
 * history:
 */

#ifndef	__LKENCODER_H_
#define	__LKENCODER_H_

/*
 编码器基类声明公共接口
 */
#include <stdio.h>
#include <_types.h>
#include "aw_all.h"

typedef enum {
    AWEncoderErrorCodeVTSessionCreateFailed,
    AWEncoderErrorCodeVTSessionPrepareFailed,
    AWEncoderErrorCodeLockSampleBaseAddressFailed,
    AWEncoderErrorCodeEncodeVideoFrameFailed,
    AWEncoderErrorCodeEncodeCreateBlockBufFailed,
    AWEncoderErrorCodeEncodeCreateSampleBufFailed,
    AWEncoderErrorCodeEncodeGetSpsPpsFailed,
    AWEncoderErrorCodeEncodeGetH264DataFailed,
    
    AWEncoderErrorCodeCreateAudioConverterFailed,
    AWEncoderErrorCodeAudioConverterGetMaxFrameSizeFailed,
    AWEncoderErrorCodeAudioEncoderFailed,
} AWEncoderErrorCode;

class LKEncoder{
public:
    virtual void open() = 0;
    virtual void close() = 0;
    void onError(AWEncoderErrorCode code, const char* des){
        // log
    }
    
    //AWEncoderManager *manager;
    float   m_quality;
};
#endif
