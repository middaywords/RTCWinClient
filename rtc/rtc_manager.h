#ifndef __RTC_MANAGER_H__
#define __RTC_MANAGER_H__

#include <api/peer_connection_interface.h>
#include <pc/video_track_source.h>

#include "rtc_connection.h"
#include "rtc_data_manager.h"
#include "rtc_message_sender.h"
#include "video_track_receiver.h"

struct RTCManagerConfig {
	bool insecure = false;

	bool no_video_device = false;
	bool no_audio_device = false;

	bool fixed_resolution = false;
	bool show_me = true;
	bool simulcast = false;

	bool disable_echo_cancellation = false;
	bool disable_auto_gain_control = false;
	bool disable_noise_suppression = false;
	bool disable_highpass_filter = false;
	bool disable_typing_detection = false;
	bool disable_residual_echo_detector = false;

	std::string priority = "BALANCE";
	webrtc::DegradationPreference GetPriority() {
		if (priority == "FRAMERATE") {
			return webrtc::DegradationPreference::MAINTAIN_RESOLUTION;
		} else if (priority == "RESOLUTION") {
			return webrtc::DegradationPreference::MAINTAIN_FRAMERATE;
		}
		return webrtc::DegradationPreference::BALANCED;
	}
};

class RTCManager {
public:
	RTCManager(RTCManagerConfig config, VideoTrackReceiver* receiver);
	~RTCManager();

	void SetDataManager(RTCDataManager* data_manager);

	std::shared_ptr<RTCConnection> CreateConnection(webrtc::PeerConnectionInterface::RTCConfiguration rtc_config, 
		RTCMessageSender* sender);

	void CreateLocalMediaStream(std::shared_ptr<RTCConnection> conn);
	void DestroyLocalMediaStream(std::shared_ptr<RTCConnection> conn);

protected:
	std::string GenerateRandomChars();

private:
	rtc::scoped_refptr<webrtc::PeerConnectionFactoryInterface> factory_;
	rtc::scoped_refptr<webrtc::AudioTrackInterface> audio_track_;
	rtc::scoped_refptr<webrtc::VideoTrackInterface> video_track_;
	rtc::scoped_refptr<webrtc::RtpSenderInterface>  audio_sender_;
	rtc::scoped_refptr<webrtc::RtpSenderInterface>  video_sender_;

	std::unique_ptr<rtc::Thread> network_thread_;
	std::unique_ptr<rtc::Thread> worker_thread_;
	std::unique_ptr<rtc::Thread> signaling_thread_;                     

	RTCManagerConfig config_;
	VideoTrackReceiver* receiver_;
	RTCDataManager* data_manager_;
};

#endif
