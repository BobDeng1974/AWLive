/**
 * file :	LKMP4Encoder.h
 * author :	Rex
 * create :	2017-02-20 15:04
 * func : 
 * history:
 */

#ifndef	__LKMP4ENCODER_H_
#define	__LKMP4ENCODER_H_

#include "mp4v2/mp4v2.h"
typedef struct _MP4ENC_NaluUnit
{
    int type;
    int size;
    unsigned char *data;
}MP4ENC_NaluUnit;

typedef struct _MP4ENC_Metadata
{
    // video, must be h264 type
    unsigned int nSpsLen;
    unsigned char Sps[1024];
    unsigned int nPpsLen;
    unsigned char Pps[1024];
    unsigned int nUsrLen;
    unsigned char Usr[1024];
} MP4ENC_Metadata,*LPMP4ENC_Metadata;

class LKMP4Encoder
{
    MP4ENC_Metadata meta;
    unsigned char *data;
public:
    LKMP4Encoder(void);
    ~LKMP4Encoder(void);
public:
    void setUserData(int bytes, void*user);
    
    // open or creat a mp4 file.
    MP4FileHandle createMP4File(const char *fileName,int width,int height,int timeScale = 90000,int frameRate = 25);
    // wirte 264 metadata in mp4 file.
    bool write264Metadata(MP4FileHandle hMp4File,LPMP4ENC_Metadata lpMetadata);
    // wirte 264 data, data can contain multiple frame.
    int writeH264Data(MP4FileHandle hMp4File,const unsigned char* pData,int size);
    
    int writeAACData(MP4FileHandle hMp4File,const unsigned char* pData,int size);
    
    int writeH264DataNalu(MP4FileHandle hMp4File,const unsigned char* nalu_data,int nalu_size,int nalu_type);
    // close mp4 file.
    void closeMP4File(MP4FileHandle hMp4File);
    // convert H264 file to mp4 file.
    // no need to call CreateMP4File and CloseMP4File,it will create/close mp4 file automaticly.
    bool writeH264File(const char* pFile264,const char* pFileMp4);
    // Prase H264 metamata from H264 data frame
    static bool praseMetadata(const unsigned char* pData,int size,MP4ENC_Metadata &metadata);
    
    void writeH264DataToFile(const unsigned char * bytes,int length);
    
    unsigned char* addHead(const unsigned char * bytes,int length);
    
private:
    // read one nalu from H264 data buffer
    static int readOneNaluFromBuf(const unsigned char *buffer,unsigned int nBufferSize,unsigned int offSet,MP4ENC_NaluUnit &nalu);
private:
    int m_nWidth;
    int m_nHeight;
    int m_nFrameRate;
    int m_nTimeScale;
    
    bool    m_has_pps;
    bool    m_has_sps;
    
    FILE * m_fp;
    
    //long m_flag = 1;
    
    MP4TrackId m_videoId;
    MP4TrackId audio;
};

#endif
