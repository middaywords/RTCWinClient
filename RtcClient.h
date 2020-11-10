#ifndef __RTC_CLIENT_H__
#define __RTC_CLIENT_H__

#include <string>
#include <atomic>
#include "rtc_manager.h"
#include "rtc_connection.h"
#include "RtcWinUtils.h"

class RtcClientStateObserver {
public:
	virtual void OnIceCandidate(std::string uid, const std::string mid, const std::string sdp) = 0;
};

class RtcClient : public RTCMessageSender {
public:
	RtcClient(std::string uid, std::string name, std::shared_ptr<RTCManager> manager, bool local) {
		is_local_ = local;
		user_name_ = name;
		user_id_ = uid;
		rtc_manager_ = manager;
		rtc_connection_ = nullptr;
		observer_ = nullptr;
		send_offer_ = false;
	}

	~RtcClient() {
		DestroyPeerConnection();
	}

	bool CreatePeerConnection() {
		if (rtc_manager_ && rtc_connection_ == nullptr) {
			webrtc::PeerConnectionInterface::RTCConfiguration rtc_config;
			rtc_config.type = webrtc::PeerConnectionInterface::IceTransportsType::kNoHost;
			rtc_connection_ = rtc_manager_->CreateConnection(rtc_config, this);
			if (is_local_ && rtc_connection_) {
				rtc_manager_->CreateLocalMediaStream(rtc_connection_);
			}
		}
		return rtc_connection_ != nullptr;
	}

	void DestroyPeerConnection() {
		if (is_local_) {
			rtc_manager_->DestroyLocalMediaStream(rtc_connection_);
		}
		rtc_connection_ = nullptr;
	}

	void SetSendOffer(bool send_offer) {
		send_offer_ = send_offer;
	}

	bool ShouldSendOffer() {
		return send_offer_;
	}
	
	void SetMid(std::string mid) {
		mid_ = mid;
	}

	bool GetOffer(std::string& sdp) {
		if (rtc_connection_) {
			return rtc_connection_->GetOffer(sdp);
		} else {
			return false;
		}
	}

	std::string GetMid() {
		return mid_;
	}

	std::string GetUserName() {
		return user_name_;
	}

	std::string GetUserId() {
		return user_id_;
	}

	bool IsLocalUser() {
		return is_local_;
	}


	std::shared_ptr<RTCConnection> GetConnection() {
		return rtc_connection_;
	}

	virtual void OnIceConnectionStateChange(webrtc::PeerConnectionInterface::IceConnectionState new_state) {
		LogPrintf("OnIceConnectionStateChange:[%s] = %d", user_id_.c_str(), (int)new_state);
	}

	virtual void OnIceCandidate(const std::string sdp_mid, const int sdp_mlineindex, const std::string sdp) {
		if (observer_) {
			observer_->OnIceCandidate(user_id_, sdp_mid, sdp);
		}
	}

	void SetStateObserver(RtcClientStateObserver* observer) {
		observer_ = observer;
	}

private:
	std::atomic_bool is_local_;
	std::atomic_bool send_offer_;
	std::string user_name_;
	std::string user_id_;
	std::string mid_;

	std::shared_ptr<RTCManager> rtc_manager_;
	std::shared_ptr<RTCConnection> rtc_connection_;
	RtcClientStateObserver* observer_;
};


#endif