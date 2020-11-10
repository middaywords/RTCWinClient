#ifndef __VCM_CAPTURE_H__
#define __VCM_CAPTURE_H__

#include <memory>
#include <vector>

#include "api/scoped_refptr.h"
#include "api/video/video_frame.h"
#include "modules/video_capture/video_capture.h"
#include "rtc_video_capturer.h"

namespace webrtc {

	class VcmCapturer : public RtcVideoCapturer, public rtc::VideoSinkInterface<VideoFrame> {
	public:
		static VcmCapturer* Create(size_t width, size_t height, size_t target_fps, size_t capture_device_index);
		virtual ~VcmCapturer();

		void OnFrame(const VideoFrame& frame) override;

	private:
		VcmCapturer();

		bool Init(size_t width, size_t height, size_t target_fps, size_t capture_device_index);
		void Destroy();

		rtc::scoped_refptr<VideoCaptureModule> vcm_;
		VideoCaptureCapability capability_;
	};
};

#endif