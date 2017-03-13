/**
 * file :	LKLKMP4Encoder.cpp
 * author :	Rex
 * create :	2017-02-20 15:04
 * func : 
 * history:
 */

#include "LKMP4Encoder.h"
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <stdlib.h>
#include <fcntl.h>

/* Header for class com_example_sj_newhellofromc_JniText */
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, "mytest",__VA_ARGS__)

#define BUFFER_SIZE (1024*1024*4)
LKMP4Encoder::LKMP4Encoder(void):
/*m_videoId(NULL),
 m_nWidth(0),
 m_nHeight(0),
 m_nTimeScale(0),*/
m_nFrameRate(0){
    m_has_pps = false;
    m_has_sps = false;
}

LKMP4Encoder::~LKMP4Encoder(void)
{
    if(data)
        delete[] data;
}
MP4FileHandle LKMP4Encoder::createMP4File(const char *pFileName,int width,int height,int timeScale/* = 90000*/,int frameRate/* = 25*/)
{
    data = new unsigned char[width*height];
    meta.nSpsLen = meta.nPpsLen = meta.nUsrLen = 0;
    meta.Usr[0] =
    meta.Usr[1] =
    meta.Usr[2] = 0;
    meta.Usr[3] = 0x01;
    meta.Usr[4] = 0x06;
    
    if(pFileName == NULL)
    {
        return 0;
    }
    // create mp4 file
    MP4FileHandle hMp4file = MP4Create(pFileName);
    if (hMp4file == MP4_INVALID_FILE_HANDLE)
    {
        return 0;
    }
    //LOGE("CreateMP4File : Open file successed.");
    m_nWidth = width;
    m_nHeight = height;
    m_nTimeScale = timeScale;
    m_nFrameRate = frameRate;
    MP4SetTimeScale(hMp4file, m_nTimeScale);
    return hMp4file;
}

void LKMP4Encoder::setUserData(int bytes, void*user)
{
    if(!bytes || !user) {
        meta.nUsrLen = 0;
        return;
    }
    memcpy(meta.Usr+5, user, meta.nUsrLen = bytes);
}

//bool LKMP4Encoder::Write264Metadata(MP4FileHandle hMp4File,LPMP4ENC_Metadata lpMetadata)
//{
//    m_videoId = MP4AddH264VideoTrack
//    (hMp4File,
//    m_nTimeScale,
//    m_nTimeScale / m_nFrameRate,
//    m_nWidth, // width
//    m_nHeight,// height
//    lpMetadata->Sps[1], // sps[1] AVCProfileIndication
//    lpMetadata->Sps[2], // sps[2] profile_compat
//    lpMetadata->Sps[3], // sps[3] AVCLevelIndication
//    3); // 4 bytes length before each NAL unit
//    if (m_videoId == MP4_INVALID_TRACK_ID)
//    {
//    printf("add video track failed.\n");
//    return false;
//    }
//    MP4SetVideoProfileLevel(hMp4File, 0x01); // Simple Profile @ Level 3
//    // write sps
//    MP4AddH264SequenceParameterSet(hMp4File,m_videoId,lpMetadata->Sps,lpMetadata->nSpsLen);
//    // write pps
//    MP4AddH264PictureParameterSet(hMp4File,m_videoId,lpMetadata->Pps,lpMetadata->nPpsLen);
//    return true;
//}

int LKMP4Encoder::writeH264Data(MP4FileHandle hMp4File,const unsigned char* pData,int size)
{
    if(hMp4File == NULL)
    {
        return -1;
    }
    if(pData == NULL)
    {
        return -1;
    }
    MP4ENC_NaluUnit nalu;
    int pos = 0, len = 0;
    while ((len = readOneNaluFromBuf(pData,size,pos,nalu))>0)
    {
        if(!writeH264DataNalu(hMp4File, nalu.data, nalu.size, nalu.type))
            return 0;
        pos += len;
    }
    
    return pos;
}

int LKMP4Encoder::writeH264DataNalu(MP4FileHandle hMp4File,const unsigned char* nalu_data,int nalu_size, int nalu_type)
{
    //sps pps 找到之后先存起来，加在每个i帧前面
    if(hMp4File == NULL)
    {
        return -1;
    }
    if(nalu_data == NULL)
    {
        return -1;
    }
    if(nalu_type == 0x07) // sps
    {
        if (m_has_sps) {
            return -1;
        }
        
        m_has_sps = true;
        // h264 track
        m_videoId = MP4AddH264VideoTrack
        (hMp4File,
         m_nTimeScale,
         m_nTimeScale/m_nFrameRate,
         m_nWidth, // width
         m_nHeight, // height
         nalu_data[1], // sps[1] AVCProfileIndication
         //nalu_data[2], // sps[2] profile_compat
         0xc0,
         nalu_data[3], // sps[3] AVCLevelIndication
         3); // 4 bytes length before each NAL unit
        
        if (m_videoId == MP4_INVALID_TRACK_ID)
        {
            //LOGE("add video track failed.");
            return 0;
        }
        MP4SetVideoProfileLevel(hMp4File, 1); // Simple Profile @ Level 3
        
        memcpy(meta.Sps, nalu_data, meta.nSpsLen = nalu_size);
        MP4AddH264SequenceParameterSet(hMp4File,m_videoId,nalu_data,nalu_size);
    }
    else if(nalu_type == 0x08) // pps
    {
        if (m_has_pps) {
            return -1;
        }
        
        m_has_pps = true;
        memcpy(meta.Pps, nalu_data, meta.nPpsLen = nalu_size);
        MP4AddH264PictureParameterSet(hMp4File,m_videoId,nalu_data,nalu_size);
    }
    else
    {
        // 等待读到sps和pps之后才开始录制
        if (!m_has_sps || !m_has_pps) {
            return -1;
        }
        
        bool key = (nalu_type == 0x05);
//        if(key) {
//            MP4AddH264SequenceParameterSet(hMp4File,m_videoId,meta.Sps,meta.nSpsLen);
//            MP4AddH264PictureParameterSet(hMp4File,m_videoId,meta.Pps,meta.nPpsLen);
//        }
        data[0] = nalu_size>>24;
        data[1] = nalu_size>>16;
        data[2] = nalu_size>>8;
        data[3] = nalu_size&0xff;
        memcpy(data+4,nalu_data,nalu_size);
        int len = 4+nalu_size;
        if(meta.nUsrLen) {
            memcpy(data+4+nalu_size, meta.Usr, 5+meta.nUsrLen);
            len += 5+meta.nUsrLen;
        }
        /// if(!MP4WriteSample(hMp4File, m_videoId, data, len,m_nTimeScale/m_nFrameRate/2, 0, key))
        if(!MP4WriteSample(hMp4File, m_videoId, data, len,MP4_INVALID_DURATION, 0, key))
        {
            return 0;
        }
    }
    return nalu_size;
}

int LKMP4Encoder::readOneNaluFromBuf(const unsigned char *buffer,unsigned int nBufferSize,unsigned int offSet,MP4ENC_NaluUnit &nalu)
{
    int i = offSet;
    while(i<nBufferSize)
    {
        if(buffer[i++] == 0x00 &&
           buffer[i++] == 0x00 &&
           buffer[i++] == 0x00 &&
           buffer[i++] == 0x01
           )
        {
            int pos = i;
            while (pos<nBufferSize)
            {
                if(buffer[pos++] == 0x00 &&
                   buffer[pos++] == 0x00 &&
                   buffer[pos++] == 0x00 &&
                   buffer[pos++] == 0x01
                   )
                {
                    break;
                }
            }
            if(pos == nBufferSize)
            {
                nalu.size = pos-i;
            }
            else
            {
                nalu.size = (pos-4)-i;
            }
            nalu.type = buffer[i]&0x1f;
            nalu.data =(unsigned char*)&buffer[i];
            return (nalu.size+i-offSet);
        }
    }
    return 0;
}
void LKMP4Encoder::closeMP4File(MP4FileHandle hMp4File)
{
    if(hMp4File)
    {
        MP4Close(hMp4File);
        hMp4File = NULL;
        m_has_pps = false;
        m_has_sps = false;
    }
}
bool LKMP4Encoder::writeH264File(const char* pFile264,const char* pFileMp4)
{
    
    if(pFile264 == NULL || pFileMp4 == NULL)
    {
        return false;
    }
    MP4FileHandle hMp4File = createMP4File(pFileMp4,640,480);
    if(hMp4File == NULL)
    {
        return false;
    }
    
    FILE *fp = fopen(pFile264, "rb");
    if(fp == NULL)
    {
        return false;
    }
    fseek(fp, 0, SEEK_SET);
    unsigned char *buffer = new unsigned char[BUFFER_SIZE];
    int pos = 0;
    while(1)
    {
        int readlen = fread(buffer+pos, sizeof(unsigned char), BUFFER_SIZE-pos, fp);
        if(readlen<=0)
        {
            break;
        }
        readlen += pos;
        int writelen = 0;
        for(int i = readlen-1; i>=0; i--)
        {
            if(buffer[i--] == 0x01 &&
               buffer[i--] == 0x00 &&
               buffer[i--] == 0x00 &&
               buffer[i--] == 0x00)
            {
                writelen = i + 5;
                break;
            }
        }
        writelen = writeH264Data(hMp4File,buffer,writelen);
        if(writelen<=0)
        {
            break;
        }
        memcpy(buffer,buffer+writelen,readlen-writelen+1);
        pos = readlen-writelen+1;
    }
    fclose(fp);
    delete[] buffer;
    closeMP4File(hMp4File);
    return true;
}
bool LKMP4Encoder:: praseMetadata(const unsigned char* pData,int size,MP4ENC_Metadata &metadata)
{
    if(pData == NULL || size<4)
    {
        return false;
    }
    MP4ENC_NaluUnit nalu;
    int pos = 0;
    bool bRet1 = false,bRet2 = false;
    while (int len = readOneNaluFromBuf(pData,size,pos,nalu))
    {
        if(nalu.type == 0x07)
        {
            memcpy(metadata.Sps,nalu.data,nalu.size);
            metadata.nSpsLen = nalu.size;
            bRet1 = true;
        }
        else if(nalu.type == 0x08)
        {
            memcpy(metadata.Pps,nalu.data,nalu.size);
            metadata.nPpsLen = nalu.size;
            bRet2 = true;
        }
        pos += len;
    }
    if(bRet1 && bRet2)
    {
        return true;
    }
    return false;
}

int LKMP4Encoder::writeAACData(MP4FileHandle hMp4File,const unsigned char* pData,int size)
{
    if(audio == NULL){
        audio = MP4AddAudioTrack(hMp4File, 48000, 1024, MP4_MPEG4_AUDIO_TYPE);
        if (audio == MP4_INVALID_TRACK_ID)
        {
            //printf("add audio track failed.\n");
            return 1;
        }
        MP4SetAudioProfileLevel(hMp4File, 0x2);
    }
    MP4WriteSample(hMp4File, audio,pData , size , MP4_INVALID_DURATION, 0, 1);
    
    return 0;
}


void LKMP4Encoder::writeH264DataToFile(const unsigned char * bytes,int length)
{
    //fp = open("/sdcard/H264FromC/addhead.h264",O_WRONLY,S_IROTH);
    //m_flag++;
    //    LOGE("fp = %d",fp);
    //    LOGE("open file");
    //    if(length != write(fp,bytes,length)){
    //        LOGE("write bytes failed!");
    //    }
    //LOGE("write bytes success!");
}
unsigned char* LKMP4Encoder::addHead(const unsigned char * bytes,int length)
{
    unsigned char addbytes[] = {0x00,0x00,0x00,0x01,0x06,0xaa,0x34,0x54,0x56,0x23,0x54,0x12,0x54};
    unsigned char *sumbytes = new unsigned char[length+13];
    memcpy(sumbytes,bytes,length);
    memcpy(sumbytes+length,addbytes,13);
    return sumbytes;
}
