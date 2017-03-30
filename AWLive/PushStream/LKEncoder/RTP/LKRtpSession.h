//
//  LKRtpSession.hpp
//  AWLive
//
//  Created by Visionin on 17/3/13.
//
//

#ifndef LKRtpSession_hpp
#define LKRtpSession_hpp

#include <stdio.h>
#include "rtpsession.h"

class LKRtpSession: public jrtplib::RTPSession{
public:
    LKRtpSession(int port, int timestamp);
    
protected:
    
};
#endif /* LKRtpSession_hpp */
