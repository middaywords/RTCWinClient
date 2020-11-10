#include "video_render_gdi.h"
#include "third_party/libyuv/include/libyuv.h"

namespace webrtc {
	VideoRenderGDI::VideoRenderGDI(webrtc::VideoTrackInterface* track) : track_(nullptr), i420_buffer_(nullptr), rgb_buffer_(nullptr){
		mirror_mode_ = false;
		video_width_ = video_height_ = 0;
		video_window_ = NULL;
		SetTrack(track);
	}

	VideoRenderGDI::~VideoRenderGDI() {
		DeallocateVideoBuffer();
		if (video_window_) {
			::InvalidateRect(video_window_, NULL, TRUE);
		}
	}

	void VideoRenderGDI::SetTrack(webrtc::VideoTrackInterface* track) {
		std::unique_lock<std::recursive_mutex> lock(lock_);
		if (track_ != track) {
			if (track_) {
				track_->RemoveSink(this);
			}
			rtc::VideoSinkWants wants;
			track_ = track;
			if (track_) {
				track_->AddOrUpdateSink(this, wants);
			}
		}
	}

	void VideoRenderGDI::SetWindow(HWND hWindow) {
		std::unique_lock<std::recursive_mutex> lock(lock_);
		video_window_ = hWindow;
	}

	HWND VideoRenderGDI::GetWindow() {
		std::unique_lock<std::recursive_mutex> lock(lock_);
		return video_window_;
	}

	void VideoRenderGDI::SetMirrorMode(bool mirror_mode) {
		std::unique_lock<std::recursive_mutex> lock(lock_);
		mirror_mode_ = mirror_mode;
	}

	bool VideoRenderGDI::AllocatVideoBuffer(int width, int height) {
		std::unique_lock<std::recursive_mutex> lock(lock_);

		memset(&bmi_, 0, sizeof(bmi_));

		bmi_.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
		bmi_.bmiHeader.biPlanes = 1;
		bmi_.bmiHeader.biBitCount = 32;
		bmi_.bmiHeader.biCompression = BI_RGB;
		bmi_.bmiHeader.biWidth = width;
		bmi_.bmiHeader.biHeight = -height;
		bmi_.bmiHeader.biSizeImage = width * height * (bmi_.bmiHeader.biBitCount >> 3);
		rgb_buffer_.reset(new uint8_t[bmi_.bmiHeader.biSizeImage]);

		if (i420_buffer_) {
			delete[] i420_buffer_;
			i420_buffer_ = nullptr;
		}
		i420_buffer_ = new uint8_t[width * height * 3 / 2];
		return (rgb_buffer_ && i420_buffer_);
	}

	void VideoRenderGDI::DeallocateVideoBuffer() {
		std::unique_lock<std::recursive_mutex> lock(lock_);
		rgb_buffer_ = nullptr;
		if (i420_buffer_) {
			delete[] i420_buffer_;
			i420_buffer_ = nullptr;
		}
	}

	void VideoRenderGDI::OnFrame(const webrtc::VideoFrame& frame) {
		std::unique_lock<std::recursive_mutex> lock(lock_, std::defer_lock);
		if (!lock.try_lock()) {
			return;
		}

		int32_t src_w = frame.width();
		int32_t src_h = frame.height();
		int32_t dst_w = src_w, dst_h = src_h;

		libyuv::RotationMode rotation_mode = (libyuv::RotationMode)frame.rotation();
		if (rotation_mode == libyuv::kRotate90 || rotation_mode == libyuv::kRotate270) {
			dst_w = src_h;
			dst_h = src_w;
		}

		if (dst_w != video_width_ || dst_h != video_height_) {
			AllocatVideoBuffer(dst_w, dst_h);
			video_width_  = dst_w;
			video_height_ = dst_h;
		}

		uint8_t* dst_y = i420_buffer_;
		uint8_t* dst_u = dst_y + dst_w * dst_h;
		uint8_t* dst_v = dst_u + dst_w * dst_h / 4;

		int32_t dst_stride_y = dst_w;
		int32_t dst_stride_u = dst_w / 2;
		int32_t dst_stride_v = dst_w / 2;

		rtc::scoped_refptr<webrtc::VideoFrameBuffer> frameBuffer = frame.video_frame_buffer();
		if (frameBuffer && frameBuffer->type() == VideoFrameBuffer::Type::kI420) {
			const I420BufferInterface* i420 = frameBuffer->GetI420();
			if (i420) {
				if (rotation_mode == libyuv::kRotate0) {
					dst_y = (uint8_t *)i420->DataY();
					dst_u = (uint8_t*)i420->DataU();
					dst_v = (uint8_t*)i420->DataV();

					dst_stride_y = i420->StrideY();
					dst_stride_u = i420->StrideU();
					dst_stride_v = i420->StrideV();
				} else {
					libyuv::I420Rotate(i420->DataY(), i420->StrideY(), i420->DataU(), i420->StrideU(),
						i420->DataV(), i420->StrideV(), dst_y, dst_stride_y, dst_u, dst_stride_u, dst_v,
						dst_stride_v, src_w, src_h, rotation_mode);
				}
			}
		}

		libyuv::I420ToARGB(dst_y, dst_w, dst_u, dst_w / 2, dst_v, dst_w / 2, rgb_buffer_.get(),
			dst_w * bmi_.bmiHeader.biBitCount / 8, dst_w, dst_h);

		if (video_window_) {
			HDC hdc = GetDC(video_window_);
			RECT rc;
			::GetClientRect(video_window_, &rc);

			int height = abs(bmi_.bmiHeader.biHeight);
			int width = bmi_.bmiHeader.biWidth;

			if (rgb_buffer_ != nullptr) {
				HDC dc_mem = ::CreateCompatibleDC(hdc);
				::SetStretchBltMode(dc_mem, HALFTONE);

				// Set the map mode so that the ratio will be maintained for us.
				HDC all_dc[] = { hdc, dc_mem };
				for (int i = 0; i < ARRAYSIZE(all_dc); ++i) {
					SetMapMode(all_dc[i], MM_ISOTROPIC);
					SetWindowExtEx(all_dc[i], width, height, NULL);
					SetViewportExtEx(all_dc[i], rc.right, rc.bottom, NULL);
				}

				HBITMAP bmp_mem = ::CreateCompatibleBitmap(hdc, rc.right, rc.bottom);
				HGDIOBJ bmp_old = ::SelectObject(dc_mem, bmp_mem);

				POINT logical_area = { rc.right, rc.bottom };
				DPtoLP(hdc, &logical_area, 1);

				HBRUSH brush = ::CreateSolidBrush(RGB(0, 0, 0));
				RECT logical_rect = { 0, 0, logical_area.x, logical_area.y };
				::FillRect(dc_mem, &logical_rect, brush);
				::DeleteObject(brush);

				int x = (logical_area.x / 2) - (width / 2);
				int y = (logical_area.y / 2) - (height / 2);

				if (mirror_mode_) {
					StretchDIBits(dc_mem, x + width, y, -width, height, 0, 0, width, height, rgb_buffer_.get(), &bmi_,
						DIB_RGB_COLORS, SRCCOPY);
				}
				else {
					StretchDIBits(dc_mem, x, y, width, height, 0, 0, width, height, rgb_buffer_.get(), &bmi_,
						DIB_RGB_COLORS, SRCCOPY);
				}

				BitBlt(hdc, 0, 0, logical_area.x, logical_area.y, dc_mem, 0, 0, SRCCOPY);

				// Cleanup.
				::SelectObject(dc_mem, bmp_old);
				::DeleteObject(bmp_mem);
				::DeleteDC(dc_mem);
			}
			ReleaseDC(video_window_, hdc);
		}
	}
}