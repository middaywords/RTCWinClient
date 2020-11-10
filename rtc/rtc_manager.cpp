#include "rtc_manager.h"

#include <iostream>

#include <absl/memory/memory.h>
#include <api/audio_codecs/builtin_audio_decoder_factory.h>
#include <api/audio_codecs/builtin_audio_encoder_factory.h>
#include <api/video_codecs/builtin_video_decoder_factory.h>
#include <api/video_codecs/builtin_video_encoder_factory.h>
#include <api/create_peerconnection_factory.h>
#include <api/rtc_event_log/rtc_event_log_factory.h>
#include <api/task_queue/default_task_queue_factory.h>
#include <api/video_track_source_proxy.h>
#include <media/engine/webrtc_media_engine.h>

#include <modules/audio_device/include/audio_device.h>
#include <modules/audio_device/include/audio_device_factory.h>
#include <modules/audio_processing/include/audio_processing.h>
#include <modules/video_capture/video_capture.h>
#include <modules/video_capture/video_capture_factory.h>
#include <rtc_base/logging.h>
#include <rtc_base/ssl_adapter.h>

#include "peer_connection_observer.h"
#include "capture_track_source.h"

#define kStreamIdLength		32
#define ARRAY_SIZE(arr)    (sizeof(arr) / sizeof((arr)[0]))

RTCManager::RTCManager(RTCManagerConfig config, VideoTrackReceiver* receiver)
	: config_(std::move(config)), receiver_(receiver), data_manager_(nullptr) {
	rtc::InitializeSSL();

	network_thread_ = rtc::Thread::CreateWithSocketServer();
	network_thread_->Start();
	worker_thread_ = rtc::Thread::Create();
	worker_thread_->Start();
	signaling_thread_ = rtc::Thread::Create();
	signaling_thread_->Start();

	webrtc::AudioDeviceModule::AudioLayer audio_layer = webrtc::AudioDeviceModule::kPlatformDefaultAudio;
	if (config_.no_audio_device) {
		audio_layer = webrtc::AudioDeviceModule::kDummyAudio;
	}

	webrtc::PeerConnectionFactoryDependencies dependencies;

	dependencies.network_thread = network_thread_.get();
	dependencies.worker_thread = worker_thread_.get();
	dependencies.signaling_thread = signaling_thread_.get();
	dependencies.task_queue_factory = webrtc::CreateDefaultTaskQueueFactory();
	dependencies.call_factory = webrtc::CreateCallFactory();
	dependencies.event_log_factory = absl::make_unique<webrtc::RtcEventLogFactory>(dependencies.task_queue_factory.get());

	// media_dependencies
	cricket::MediaEngineDependencies media_dependencies;
	media_dependencies.task_queue_factory = dependencies.task_queue_factory.get();

	media_dependencies.adm =
		worker_thread_->Invoke<rtc::scoped_refptr<webrtc::AudioDeviceModule> >(
			RTC_FROM_HERE, [&] {
				return webrtc::CreateWindowsCoreAudioAudioDeviceModule(
					dependencies.task_queue_factory.get());
			});
	media_dependencies.audio_encoder_factory = webrtc::CreateBuiltinAudioEncoderFactory();
	media_dependencies.audio_decoder_factory = webrtc::CreateBuiltinAudioDecoderFactory();

	{
		media_dependencies.video_encoder_factory = webrtc::CreateBuiltinVideoEncoderFactory();
		media_dependencies.video_decoder_factory = webrtc::CreateBuiltinVideoDecoderFactory();

		media_dependencies.audio_mixer = nullptr;
		media_dependencies.audio_processing = webrtc::AudioProcessingBuilder().Create();
		dependencies.media_engine = cricket::CreateMediaEngine(std::move(media_dependencies));

		factory_ = webrtc::CreateModularPeerConnectionFactory(std::move(dependencies));
		if (!factory_.get()) {
			RTC_LOG(LS_ERROR) << __FUNCTION__ << ": Failed to initialize PeerConnectionFactory";
			exit(1);
		}

		webrtc::PeerConnectionFactoryInterface::Options factory_options;
		factory_options.disable_sctp_data_channels = false;
		factory_options.disable_encryption = false;
		factory_options.ssl_max_version = rtc::SSL_PROTOCOL_DTLS_12;
		factory_->SetOptions(factory_options);
	}
}

RTCManager::~RTCManager() {
	audio_track_ = nullptr;
	video_track_ = nullptr;
	factory_ = nullptr;
	network_thread_->Stop();
	worker_thread_->Stop();
	signaling_thread_->Stop();

	rtc::CleanupSSL();
}

void RTCManager::SetDataManager(RTCDataManager* data_manager) {
	data_manager_ = data_manager;
}

std::string RTCManager::GenerateRandomChars() {
	std::string result;
	rtc::CreateRandomString(kStreamIdLength, &result);
	return result;
}

std::shared_ptr<RTCConnection> RTCManager::CreateConnection(webrtc::PeerConnectionInterface::RTCConfiguration rtc_config, RTCMessageSender* sender) {
	rtc_config.enable_dtls_srtp = true;
	rtc_config.sdp_semantics = webrtc::SdpSemantics::kUnifiedPlan;

	webrtc::PeerConnectionInterface::IceServers servers;

	const char* pszStunServers[] = { 
		"stun:stun.voiparound.com", 
		"stun:stun.voipbuster.com", 
		"stun:stun.voipstunt.com",   
		"stun:stun.xten.com" 
	};

	for (int i = 0; i < ARRAY_SIZE(pszStunServers); i++) {
		webrtc::PeerConnectionInterface::IceServer ice_server;
		ice_server.uri = pszStunServers[i];
		servers.push_back(ice_server);
	}

	rtc_config.servers = servers;
	
	std::unique_ptr<PeerConnectionObserver> observer(new PeerConnectionObserver(sender, receiver_, data_manager_));
	webrtc::PeerConnectionDependencies dependencies(observer.get());
	
	rtc::scoped_refptr<webrtc::PeerConnectionInterface> connection = factory_->CreatePeerConnection(rtc_config, std::move(dependencies));
	if (!connection) {
		RTC_LOG(LS_ERROR) << __FUNCTION__ << ": CreatePeerConnection failed";
		return nullptr;
	}
	return std::make_shared<RTCConnection>(sender, std::move(observer), connection);
}

void RTCManager::CreateLocalMediaStream(std::shared_ptr<RTCConnection> conn) {
	if (!config_.no_audio_device) {
		cricket::AudioOptions ao;
		if (config_.disable_echo_cancellation)
			ao.echo_cancellation = false;
		if (config_.disable_auto_gain_control)
			ao.auto_gain_control = false;
		if (config_.disable_noise_suppression)
			ao.noise_suppression = false;
		if (config_.disable_highpass_filter)
			ao.highpass_filter = false;
		if (config_.disable_typing_detection)
			ao.typing_detection = false;
		if (config_.disable_residual_echo_detector)
			ao.residual_echo_detector = false;
		ao.echo_cancellation = true;
		RTC_LOG(LS_INFO) << __FUNCTION__ << ": " << ao.ToString();
		audio_track_ = factory_->CreateAudioTrack(GenerateRandomChars(), factory_->CreateAudioSource(ao));
		if (!audio_track_) {
			RTC_LOG(LS_WARNING) << __FUNCTION__ << ": Cannot create audio_track";
		}
	}

	if (!config_.no_video_device) {
		rtc::scoped_refptr<webrtc::VideoTrackSourceInterface> video_source = webrtc::VideoTrackSourceProxy::Create(signaling_thread_.get(),
			worker_thread_.get(), CapturerTrackSource::Create());
		video_track_ = factory_->CreateVideoTrack(GenerateRandomChars(), video_source);
		if (video_track_) {
			if (config_.fixed_resolution) {
				video_track_->set_content_hint(webrtc::VideoTrackInterface::ContentHint::kText);
			}

			if (receiver_ != nullptr && config_.show_me) {
				receiver_->AddTrack(video_track_, true);
			}
		}
		else {
			RTC_LOG(LS_WARNING) << __FUNCTION__ << ": Cannot create video_track";
		}
	}

	auto connection = conn->GetConnection();
	std::string stream_id = GenerateRandomChars();
	if (audio_track_) {
		webrtc::RTCErrorOr<rtc::scoped_refptr<webrtc::RtpSenderInterface>> audio_add_result = connection->AddTrack(audio_track_, { stream_id });
		if (audio_add_result.ok()) {
			audio_sender_ = audio_add_result.value();
		} else {
			RTC_LOG(LS_WARNING) << __FUNCTION__ << ": Cannot add audio_track_";
		}
	}

	if (video_track_) {
		webrtc::RTCErrorOr<rtc::scoped_refptr<webrtc::RtpSenderInterface>> video_add_result = connection->AddTrack(video_track_, { stream_id });
		if (video_add_result.ok()) {
			video_sender_ = video_add_result.value();

			webrtc::RtpParameters parameters = video_sender_->GetParameters();
			parameters.degradation_preference = config_.GetPriority();

			webrtc::RtpEncodingParameters encoding;
			encoding.max_bitrate_bps = 1000;
			encoding.max_framerate = 25;
			parameters.encodings.push_back(encoding);

			video_sender_->SetParameters(parameters);
		} else {
			RTC_LOG(LS_WARNING) << __FUNCTION__ << ": Cannot add video_track_";
		}
	}
}

void RTCManager::DestroyLocalMediaStream(std::shared_ptr<RTCConnection> conn) {
	if (conn) {
		if (video_track_) {
			if (receiver_ != nullptr) {
				receiver_->RemoveTrack(video_track_);
			}

			video_track_->set_enabled(false);
			video_track_ = nullptr;
		}

		if (audio_track_) {
			audio_track_->set_enabled(false);
			audio_track_ = nullptr;
		}

		auto connection = conn->GetConnection();
		if (connection && audio_sender_) {
			connection->RemoveTrack(audio_sender_);
			audio_sender_ = nullptr;
		}

		if (connection != nullptr && video_sender_) {
			connection->RemoveTrack(video_sender_);
			video_sender_ = nullptr;
		}
	}
}