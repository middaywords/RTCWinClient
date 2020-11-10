#ifndef __VIDEO_TRACK_RECEIVER_H__
#define __VIDEO_TRACK_RECEIVER_H__

#include <string>
#include <api/media_stream_interface.h>

class VideoTrackReceiver {
public:
	virtual void AddTrack(webrtc::VideoTrackInterface* track, bool is_local) = 0;
	virtual void RemoveTrack(webrtc::VideoTrackInterface* track) = 0;
};

#endif
