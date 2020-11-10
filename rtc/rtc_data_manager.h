#ifndef __RTC_DATA_MANAGER_H__
#define __RTC_DATA_MANAGER_H__

#include <api/data_channel_interface.h>

class RTCDataManager {
public:
	virtual ~RTCDataManager() = default;

	virtual void OnDataChannel(rtc::scoped_refptr<webrtc::DataChannelInterface> data_channel) = 0;
};

#endif
