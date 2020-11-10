#ifndef __CAPTURE_TRACK_SOURCE_H__
#define __CAPTURE_TRACK_SOURCE_H__

#include <pc/video_track_source.h>
#include "vcm_capture.h"

class CapturerTrackSource : public webrtc::VideoTrackSource {
public:
	static rtc::scoped_refptr<CapturerTrackSource> Create() {
		const size_t kWidth = 640;
		const size_t kHeight = 480;
		const size_t kFps = 30;
		const size_t kDeviceIndex = 0;

		std::unique_ptr<webrtc::VcmCapturer> capturer = absl::WrapUnique(webrtc::VcmCapturer::Create(kWidth, kHeight, kFps, kDeviceIndex));
		if (!capturer) {
			return nullptr;
		}
		return new rtc::RefCountedObject<CapturerTrackSource>(std::move(capturer));
	}

protected:
	explicit CapturerTrackSource(std::unique_ptr<webrtc::VcmCapturer> capturer)
		: VideoTrackSource(/*remote=*/false), capturer_(std::move(capturer)) {
	}

private:
	rtc::VideoSourceInterface<webrtc::VideoFrame>* source() override {
		return capturer_.get();
	}

	std::unique_ptr<webrtc::VcmCapturer> capturer_;
};


#endif