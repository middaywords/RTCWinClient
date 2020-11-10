#ifndef __RTC_CONNECTION_H__
#define __RTC_CONNECTION_H__

#include <api/peer_connection_interface.h>
#include "peer_connection_observer.h"

class RTCConnection {
public:
	RTCConnection(RTCMessageSender* sender, std::unique_ptr<PeerConnectionObserver> observer, 
		rtc::scoped_refptr<webrtc::PeerConnectionInterface> connection)
		: sender_(sender),
		observer_(std::move(observer)),
		connection_(connection) {}
	~RTCConnection();

	typedef std::function<void(webrtc::SessionDescriptionInterface*)> OnCreateSuccessFunc;
	typedef std::function<void(webrtc::RTCError)> OnCreateFailureFunc;
	typedef std::function<void()> OnSetSuccessFunc;
	typedef std::function<void(webrtc::RTCError)> OnSetFailureFunc;

	void CreateOffer(bool publish, OnCreateSuccessFunc on_success = nullptr, OnCreateFailureFunc on_failure = nullptr);
	void SetOffer(const std::string sdp, OnSetSuccessFunc on_success = nullptr, OnSetFailureFunc on_failure = nullptr);

	void CreateAnswer(OnCreateSuccessFunc on_success = nullptr, OnCreateFailureFunc on_failure = nullptr);
	void SetAnswer(const std::string sdp, OnSetSuccessFunc on_success = nullptr, OnSetFailureFunc on_failure = nullptr);

	bool GetOffer(std::string& sdp);

	void AddIceCandidate(const std::string sdp_mid, const int sdp_mlineindex, const std::string sdp);

	void GetStats(std::function<void(const rtc::scoped_refptr<const webrtc::RTCStatsReport>&)> callback);
	void SetEncodingParameters(std::vector<webrtc::RtpEncodingParameters> encodings);
	rtc::scoped_refptr<webrtc::PeerConnectionInterface> GetConnection() const;

private:
	RTCMessageSender* sender_;
	std::unique_ptr<PeerConnectionObserver> observer_;
	rtc::scoped_refptr<webrtc::PeerConnectionInterface> connection_;
};

#endif
