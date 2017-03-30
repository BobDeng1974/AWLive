//
//  LKRtpSession.cpp
//  AWLive
//
//  Created by Visionin on 17/3/13.
//
//

#include "LKRtpSession.h"
#include "rtpsessionparams.h"
#include "rtpudpv4transmitter.h"

using namespace jrtplib;

LKRtpSession::LKRtpSession(int port, int timestamp){
    RTPSessionParams sessparams;
    sessparams.SetOwnTimestampUnit(1.0/timestamp);//必须设置
    
    RTPUDPv4TransmissionParams transparams;
    transparams.SetPortbase(port);//port必须是偶数
    int status = Create(sessparams, &transparams);
    if (status<0) {
        fprintf(stderr, "RTPSession Create Error: %d", status);
    }
    
    SetDefaultPayloadType(98);  // h264流
    SetDefaultTimestampIncrement(40);
    SetDefaultMark(false);
}
