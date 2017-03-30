/**
 * file :	LKVideoEncoder.cpp
 * author :	Rex
 * create :	2017-02-17 17:00
 * func : 
 * history:
 */

#include "LKVideoEncoder.h"

LKVideoEncoder::LKVideoEncoder(int widht, int height, float quality){
    m_video_config.width = widht;
    m_video_config.height = height;
    m_video_config.fps = 15;
    m_quality = quality;
    m_video_config.bitrate = (m_video_config.width*m_video_config.height*m_video_config.fps*(quality+0.5)*0.1);
    m_video_config.input_data_format = X264_CSP_NV12;
    m_frame_count = 0;
}
