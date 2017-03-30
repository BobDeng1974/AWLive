/**
 * file :	LKHWH264Encoder.h
 * author :	Rex
 * create :	2017-02-17 17:01
 * func : 
 * history:
 */

#ifndef	__LKHWH264ENCODER_H_
#define	__LKHWH264ENCODER_H_

#include "LKVideoEncoder.h"

typedef struct lk_encoder_t lk_encoder_t;

class LKHWH264Callback{
public:
    virtual void handleH264(const unsigned char* data, uint32_t length, int nalu_type, bool is_key) = 0;
};

class LKHWH264ToFileCallback: public LKHWH264Callback{
public:
    virtual void handleH264(const unsigned char* data, uint32_t length, int nalu_type, bool is_key);
    
    int     m_fd;
};
class LKHWH264ToFlvCallback: public LKHWH264Callback{
public:
    virtual void handleH264(const unsigned char* data, uint32_t length, int nalu_type, bool is_key);
    
    uint32_t    m_pts_ms;
};

class LKHWH264Encoder: public LKVideoEncoder{
public:
    LKHWH264Encoder(LKHWH264Callback* callback=NULL);
    ~LKHWH264Encoder();
    
    virtual void open(int width, int height);
    virtual void close();
    
    virtual aw_flv_video_tag* encodeYUVDataToFlvTag(unsigned char* yuv);
    //根据flv，h264，aac协议，提供首帧需要发送的tag
    //创建sps pps
    virtual aw_flv_video_tag* createSpsPpsFlvTag();
    
    virtual bool encodeN12BytesToH264(unsigned char* yuv);
    virtual int encodePiexelBufferToH264(void* pixelbuffer);
    
    LKHWH264Callback*   m_encoder_callback;
    lk_encoder_t*       m_encoder_session;
    
    uint32_t        m_frame_count;
    uint32_t        m_frame_rate;
    uint32_t        m_encode_frame;
    time_t          m_encode_time;
};

#endif
