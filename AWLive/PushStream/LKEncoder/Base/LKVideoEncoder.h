/**
 * file :	LKVideoEncoder.h
 * author :	Rex
 * create :	2017-02-17 17:00
 * func : 
 * history:
 */

#ifndef	__LKVIDEOENCODER_H_
#define	__LKVIDEOENCODER_H_

#include "LKEncoder.h"

class LKVideoEncoder: public LKEncoder{
public:
    LKVideoEncoder(int width, int height, float quality=0.75);
    
    virtual bool encodeN12BytesToH264(unsigned char* yuv) = 0;
    virtual aw_flv_video_tag* encodeYUVDataToFlvTag(unsigned char* yuv) = 0;
    //根据flv，h264，aac协议，提供首帧需要发送的tag
    //创建sps pps
    virtual aw_flv_video_tag* createSpsPpsFlvTag() = 0;
    
    virtual bool encodePiexelBufferToH264(void* pixelbuffer) = 0;
    //aw_flv_video_tag* encodeVideoSampleBufferToFlvTag(void* sample);
    
    aw_x264_config m_video_config;
    
    bool            m_is_keyframe;
    uint32_t        m_frame_count;
};

#endif
