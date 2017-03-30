/**
 * file :	LKHWH264Encoder.mm
 * author :	Rex
 * create :	2017-02-17 17:01
 * func : 
 * history:
 */

#include "LKHWH264Encoder.h"
#import <Foundation/Foundation.h>
#import <AVFoundation/AVFoundation.h>
#import <VideoToolbox/VideoToolbox.h>

struct lk_encoder_t{
    VTCompressionSessionRef vEnSession;
    dispatch_semaphore_t    vSemaphore;
    NSData *spsPpsData;
    NSData *naluData;
    bool    has_spspps;
};

LKHWH264Encoder::LKHWH264Encoder(LKHWH264Callback* callback)
{
    m_encoder_session = (lk_encoder_t*)calloc(1, sizeof(lk_encoder_t));
    m_encoder_session->vSemaphore = dispatch_semaphore_create(0);
    m_encoder_callback = callback;
    m_is_keyframe = false;
    m_frame_count = 0;
    m_encode_frame = 0;
}
LKHWH264Encoder::~LKHWH264Encoder(){
    free(m_encoder_session);
    CFRelease(m_encoder_session->vEnSession);
}

static void vtCompressionSessionCallback (void * CM_NULLABLE outputCallbackRefCon,
                                          void * CM_NULLABLE sourceFrameRefCon,
                                          OSStatus status,
                                          VTEncodeInfoFlags infoFlags,
                                          CM_NULLABLE CMSampleBufferRef sampleBuffer );

void LKHWH264Encoder::open(int width, int height){
    m_video_config.width = width;
    m_video_config.height = height;
    m_encode_time = time(0);
    m_frame_count = 0;
    m_encode_frame = 0;
    srand((int)time(0));
    m_frame_rate = 0;
    
    //创建 video encode session
    // 创建 video encode session
    // 传入视频宽高，编码类型：kCMVideoCodecType_H264
    // 编码回调：vtCompressionSessionCallback，这个回调函数为编码结果回调，编码成功后，会将数据传入此回调中。
    // (__bridge void * _Nullable)(self)：这个参数会被原封不动地传入vtCompressionSessionCallback中，此参数为编码回调同外界通信的唯一参数。
    // &_vEnSession，c语言可以给传入参数赋值。在函数内部会分配内存并初始化_vEnSession。
    OSStatus status = VTCompressionSessionCreate(NULL, m_video_config.width, m_video_config.height, kCMVideoCodecType_H264, NULL, NULL, NULL, vtCompressionSessionCallback, this, &m_encoder_session->vEnSession);
    if (status == noErr) {
        // 设置参数
        // ProfileLevel，h264的协议等级，不同的清晰度使用不同的ProfileLevel。
        VTSessionSetProperty(m_encoder_session->vEnSession, kVTCompressionPropertyKey_ProfileLevel, kVTProfileLevel_H264_Baseline_AutoLevel);
        // 设置码率
        VTSessionSetProperty(m_encoder_session->vEnSession, kVTCompressionPropertyKey_AverageBitRate, (__bridge CFTypeRef)@(m_video_config.bitrate));
        VTSessionSetProperty(m_encoder_session->vEnSession, kVTCompressionPropertyKey_DataRateLimits, (__bridge CFArrayRef)@[@(m_video_config.bitrate*2/8),@1]);
        VTSessionSetProperty(m_encoder_session->vEnSession, kVTCompressionPropertyKey_Quality, (__bridge CFTypeRef)@(m_quality));
        // 设置实时编码
        VTSessionSetProperty(m_encoder_session->vEnSession, kVTCompressionPropertyKey_RealTime, kCFBooleanTrue);
        // 关闭重排Frame，因为有了B帧（双向预测帧，根据前后的图像计算出本帧）后，编码顺序可能跟显示顺序不同。此参数可以关闭B帧。
        VTSessionSetProperty(m_encoder_session->vEnSession, kVTCompressionPropertyKey_AllowFrameReordering, kCFBooleanFalse);
        // 关键帧最大间隔，关键帧也就是I帧。此处表示关键帧最大间隔为2s。
        VTSessionSetProperty(m_encoder_session->vEnSession, kVTCompressionPropertyKey_MaxKeyFrameInterval, (__bridge CFTypeRef)@(m_video_config.fps * 2));
        //设置帧率，只用于初始化session， 不是实际FPS
        status = VTSessionSetProperty(m_encoder_session->vEnSession, kVTCompressionPropertyKey_ExpectedFrameRate, (__bridge CFTypeRef)@(m_video_config.fps));
        // 关于B帧 P帧 和I帧，请参考：http://blog.csdn.net/abcjennifer/article/details/6577934
        
        //参数设置完毕，准备开始，至此初始化完成，随时来数据，随时编码
        status = VTCompressionSessionPrepareToEncodeFrames(m_encoder_session->vEnSession);
        if (status != noErr) {
            // [self onErrorWithCode:AWEncoderErrorCodeVTSessionPrepareFailed des:@"硬编码vtsession prepare失败"];
        }
    }else{
        // log创建失败
    }
}

void LKHWH264Encoder::close(){
    dispatch_semaphore_signal(m_encoder_session->vSemaphore);
    
    VTCompressionSessionInvalidate(m_encoder_session->vEnSession);
    m_encoder_session->vEnSession = nil;
    
    m_encoder_session->naluData = nil;
    m_is_keyframe = false;
    m_frame_count = 0;
    m_encode_frame = 0;
    m_encoder_session->spsPpsData = nil;
}

bool LKHWH264Encoder::encodeN12BytesToH264(unsigned char* yuv_data){
    //yuv 变成 转CVPixelBufferRef
    //视频宽度
    size_t width = m_video_config.width;
    //视频高度
    size_t height = m_video_config.height;
    
    //现在要把NV12数据放入 CVPixelBufferRef中，因为 硬编码主要调用VTCompressionSessionEncodeFrame函数，此函数不接受yuv数据，但是接受CVPixelBufferRef类型。
    CVPixelBufferRef pixelBuf = NULL;
    size_t planeBytesPerRow[] = {width, width};
    size_t planeWidth[] = {width, width/2};
    size_t planeHeight[] = {height, height/2};
    void * planeBytes[2] = {yuv_data, yuv_data+width*height};
    // kCVPixelFormatType_420YpCbCr8BiPlanarFullRange格式pixelBuffer，如果是连续数据需要设置dataSize，编码时为连续数据
    size_t dataSize = width*height*3/2;
    CVPixelBufferCreateWithPlanarBytes(kCFAllocatorDefault, width, height, kCVPixelFormatType_420YpCbCr8BiPlanarFullRange, NULL, dataSize, 2, planeBytes, planeWidth, planeHeight, planeBytesPerRow, nil, nil, nil, &pixelBuf);
    
    if(CVPixelBufferLockBaseAddress(pixelBuf, 0) != kCVReturnSuccess){
        //[self onErrorWithCode:AWEncoderErrorCodeLockSampleBaseAddressFailed des:@"encode video lock base address failed"];
        return NULL;
    }
    
    bool st = encodePiexelBufferToH264(pixelBuf);
    
    CVPixelBufferUnlockBaseAddress(pixelBuf, 0);
    CFRelease(pixelBuf);
    return st;
}

int LKHWH264Encoder::encodePiexelBufferToH264(void* pixelbuffer){
    if (!m_encoder_session->vEnSession) {
        return NULL;
    }
    
    m_frame_count++;
    
    // 每秒统计一次帧率
    if(time(0)-m_encode_time>0){
        m_frame_rate = m_frame_count/(time(0)-m_encode_time);
    }
    
    int r = rand()%100;
    if(r<((int)m_frame_rate-15)*100/(int)m_frame_rate){
        return noErr;
    }
    
    CVPixelBufferRef pixelBuf = (CVPixelBufferRef)pixelbuffer;
    //硬编码 CmSampleBufRef
    //时间戳
    //uint32_t ptsMs = self.manager.timestamp + 1; //self.vFrameCount++ * 1000.f / self.videoConfig.fps;
    CMTime pts = CMTimeMake(m_encode_frame++, 1000);
    printf("frame[%d / %d]\n", m_encode_frame, m_encode_frame);
    //硬编码主要其实就这一句。将携带NV12数据的PixelBuf送到硬编码器中，进行编码。
    VTEncodeInfoFlags flags;
    OSStatus status = VTCompressionSessionEncodeFrame(m_encoder_session->vEnSession, pixelBuf, pts, kCMTimeInvalid, NULL, NULL, &flags);
    return status;
}

aw_flv_video_tag* LKHWH264Encoder::encodeYUVDataToFlvTag(unsigned char *yuv_data){
    return NULL;
}

aw_flv_video_tag* LKHWH264Encoder::createSpsPpsFlvTag(){
    while(!m_encoder_session->spsPpsData) {
        dispatch_semaphore_wait(m_encoder_session->vSemaphore, DISPATCH_TIME_FOREVER);
    }
    aw_data *sps_pps_data = alloc_aw_data((uint32_t)m_encoder_session->spsPpsData.length);
    memcpy_aw_data(&sps_pps_data, (uint8_t *)m_encoder_session->spsPpsData.bytes, (uint32_t)m_encoder_session->spsPpsData.length);
    aw_flv_video_tag *sps_pps_tag = aw_encoder_create_sps_pps_tag(sps_pps_data);
    free_aw_data(&sps_pps_data);
    return sps_pps_tag;
}

static void vtCompressionSessionCallback (void * CM_NULLABLE outputCallbackRefCon,
                                          void * CM_NULLABLE sourceFrameRefCon,
                                          OSStatus status,
                                          VTEncodeInfoFlags infoFlags,
                                          CM_NULLABLE CMSampleBufferRef sampleBuffer ){
    //通过outputCallbackRefCon获取AWHWH264Encoder的对象指针，将编码好的h264数据传出去。
    LKHWH264Encoder *encoder = (LKHWH264Encoder *)(outputCallbackRefCon);
    ++encoder->m_encode_frame;
    lk_encoder_t* session = encoder->m_encoder_session;
    //判断是否编码成功
    if (status != noErr) {
        //[encoder onErrorWithCode:AWEncoderErrorCodeEncodeVideoFrameFailed des:@"encode video frame error 1"];
        return;
    }
    
    //是否数据是完整的
    if (!CMSampleBufferDataIsReady(sampleBuffer)) {
        //[encoder onErrorWithCode:AWEncoderErrorCodeEncodeVideoFrameFailed des:@"encode video frame error 2"];
        return;
    }
    
    //是否是关键帧，关键帧和非关键帧要区分清楚。推流时也要注明。
    BOOL isKeyFrame = !CFDictionaryContainsKey( (CFDictionaryRef)(CFArrayGetValueAtIndex(CMSampleBufferGetSampleAttachmentsArray(sampleBuffer, true), 0)), kCMSampleAttachmentKey_NotSync);
    
    //首先获取sps 和pps
    //sps pss 也是h264的一部分，可以认为它们是特别的h264视频帧，保存了h264视频的一些必要信息。
    //没有这部分数据h264视频很难解析出来。
    //数据处理时，sps pps 数据可以作为一个普通h264帧，放在h264视频流的最前面。
    if (isKeyFrame) {
//        if (isKeyFrame) {
//            //获取avcC，这就是我们想要的sps和pps数据。
//            //如果保存到文件中，需要将此数据前加上 [0 0 0 1] 4个字节，写入到h264文件的最前面。
//            //如果推流，将此数据放入flv数据区即可。
//            CMFormatDescriptionRef sampleBufFormat = CMSampleBufferGetFormatDescription(sampleBuffer);
//            NSDictionary *dict = (__bridge NSDictionary *)CMFormatDescriptionGetExtensions(sampleBufFormat);
//            session->spsPpsData = dict[@"SampleDescriptionExtensionAtoms"][@"avcC"];
//        }
        size_t spsSize, spsCount;
        size_t ppsSize, ppsCount;
        
        const uint8_t *spsData, *ppsData;
        
        CMFormatDescriptionRef formatDesc = CMSampleBufferGetFormatDescription(sampleBuffer);
        OSStatus err0 = CMVideoFormatDescriptionGetH264ParameterSetAtIndex(formatDesc, 0, &spsData, &spsSize, &spsCount, 0 );
        OSStatus err1 = CMVideoFormatDescriptionGetH264ParameterSetAtIndex(formatDesc, 1, &ppsData, &ppsSize, &ppsCount, 0 );
        
        if (err0==noErr && err1==noErr)
        {
            if (encoder->m_encoder_callback!=NULL) {
                encoder->m_encoder_callback->handleH264(spsData, (uint32_t)spsSize, 0x07, true);
                encoder->m_encoder_callback->handleH264(ppsData, (uint32_t)ppsSize, 0x08, true);
            }
            
            NSLog(@"got sps/pps data. Length: sps=%zu, pps=%zu", spsSize, ppsSize);
        }
    }
    
    //获取真正的视频帧数据
    CMBlockBufferRef blockBuffer = CMSampleBufferGetDataBuffer(sampleBuffer);
    size_t blockDataLen;
    uint8_t *blockData;
    status = CMBlockBufferGetDataPointer(blockBuffer, 0, NULL, &blockDataLen, (char **)&blockData);
    if (status == noErr) {
        size_t currReadPos = 0;
        //一般情况下都是只有1帧，在最开始编码的时候有2帧，取最后一帧
        while (currReadPos < blockDataLen - 4) {
            uint32_t naluLen = 0;
            memcpy(&naluLen, blockData + currReadPos, 4);
            naluLen = CFSwapInt32BigToHost(naluLen);
            
            //naluData 即为一帧h264数据。
            //如果保存到文件中，需要将此数据前加上 [0 0 0 1] 4个字节，按顺序写入到h264文件中。
            //如果推流，需要将此数据前加上4个字节表示数据长度的数字，此数据需转为大端字节序。
            //关于大端和小端模式，请参考此网址：http://blog.csdn.net/hackbuteer1/article/details/7722667
            //session->naluData = [NSData dataWithBytes:blockData + currReadPos + 4 length:naluLen];
            if (encoder->m_encoder_callback!=NULL) {
                encoder->m_encoder_callback->handleH264(blockData+currReadPos+4, naluLen, (blockData+currReadPos+4)[0]&0x1f, isKeyFrame);
            }
            
            currReadPos += 4 + naluLen;
            encoder->m_is_keyframe = isKeyFrame;
        }
    }else{
        //[encoder onErrorWithCode:AWEncoderErrorCodeEncodeGetH264DataFailed des:@"got h264 data failed"];
    }
}

void LKHWH264ToFileCallback::handleH264(const unsigned char* data, uint32_t length, int nalu_type, bool is_key){
    // 添加4字节的 h264 协议 start code
    const Byte bytes[] = "\x00\x00\x00\x01";
    if (m_fd > 0) {
        write(m_fd, bytes, 4);
        write(m_fd, data, length);
    }
}

void LKHWH264ToFlvCallback::handleH264(const unsigned char *data, uint32_t naluLen, int nalu_type, bool is_key){
    //此处 硬编码成功，_naluData内的数据即为h264视频帧。
    //我们是推流，所以获取帧长度，转成大端字节序，放到数据的最前面
    //小端转大端。计算机内一般都是小端，而网络和文件中一般都是大端。大端转小端和小端转大端算法一样，就是字节序反转就行了。
    uint8_t naluLenArr[4] = {static_cast<uint8_t>(naluLen >> 24 & 0xff), static_cast<uint8_t>(naluLen >> 16 & 0xff), static_cast<uint8_t>(naluLen >> 8 & 0xff), static_cast<uint8_t>(naluLen & 0xff)};
    //将数据拼在一起
    int8_t* buffer = (int8_t*)malloc(naluLen+4);
    memcpy(buffer, naluLenArr, 4);
    memcpy(buffer+4, data, naluLen);
    
    //将h264数据合成flv tag，合成flvtag之后就可以直接发送到服务端了。后续会介绍
    aw_flv_video_tag *video_tag = aw_encoder_create_video_tag(buffer, naluLen+4, m_pts_ms, 0, is_key);
    // 处理video_tag
    
    free(buffer);
}
