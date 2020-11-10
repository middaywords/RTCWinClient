#ifndef __RTC_VIDEO_CAPTURER_H__
#define __RTC_VIDEO_CAPTURER_H__

#include <stddef.h>
#include <memory>
#include <mutex>

#include "api/video/video_frame.h"
#include "api/video/video_source_interface.h"
#include "media/base/video_adapter.h"
#include "media/base/video_broadcaster.h"

namespace webrtc {

	class RtcVideoCapturer : public rtc::VideoSourceInterface<VideoFrame> {
	public:
		~RtcVideoCapturer() override;

		void AddOrUpdateSink(rtc::VideoSinkInterface<VideoFrame>* sink, const rtc::VideoSinkWants& wants) override;
		void RemoveSink(rtc::VideoSinkInterface<VideoFrame>* sink) override;

	protected:
		void OnFrame(const VideoFrame& frame);
		rtc::VideoSinkWants GetSinkWants();

	private:
		void UpdateVideoAdapter();

		std::mutex lock_;
		rtc::VideoBroadcaster broadcaster_;
		cricket::VideoAdapter video_adapter_;
	};
}

#endif