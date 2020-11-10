#ifndef __VIDEO_RENDER_GDI_H__
#define __VIDEO_RENDER_GDI_H__

#include <api/scoped_refptr.h>
#include <api/video/video_frame.h>
#include <api/video/video_sink_interface.h>
#include <api/media_stream_interface.h>

#include <mutex>
#include <windows.h>

namespace webrtc {

	class VideoRenderGDI : public rtc::VideoSinkInterface<webrtc::VideoFrame> {
	public:
		VideoRenderGDI(webrtc::VideoTrackInterface* track);
		~VideoRenderGDI();

		HWND GetWindow();
		void SetWindow(HWND hWindow);
		void SetMirrorMode(bool mirror_mode);
		void SetTrack(webrtc::VideoTrackInterface* track);

		void OnFrame(const webrtc::VideoFrame& frame) override;

	protected:
		bool AllocatVideoBuffer(int width, int height);
		void DeallocateVideoBuffer();

	private:
		BITMAPINFO                 bmi_;
		std::unique_ptr<uint8_t[]> rgb_buffer_;
		uint8_t*                   i420_buffer_;
		bool                       mirror_mode_;
		int                        video_width_;
		int                        video_height_;
		HWND                       video_window_;
		std::recursive_mutex       lock_;

		rtc::scoped_refptr<webrtc::VideoTrackInterface> track_;
	};

}

#endif