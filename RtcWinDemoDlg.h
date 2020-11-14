
// RtcWinDemoDlg.h: 头文件
//

#pragma once

#include "rtc_manager.h"
#include "WsClient.h"
#include "RtcClient.h"

#include "rtc/video_track_receiver.h"
#include "rtc/rtc_data_manager.h"
#include "rtc/rtc_message_sender.h"
#include "rtc/video_render_gdi.h"
#include "json/cJSON.h"

#include <atomic>

enum WebSocketCommand {
	kWebSocketCommandInvalid = 0,

	kWebSocketRequestJoinRoom,
	kWebSocketRequestLeaveRoom,
	kWebSocketRequestPublish,
	kWebSocketRequestUnPublish,
	kWebSocketRequestSubscribe,
	kWebSocketRequestUnSubscribe,
	kWebSocketRequestTrickle,

	kWebSocketNotifyUserJoin = 0x10,
	kWebSocketNotifyUserLeave,
	kWebSocketNotifyStreamAdd,
	kWebSocketNotifyStreamRemove,
	kWebSocketNotifyBroadcast,
};

struct RequestRecord {
	RequestRecord(int request, std::string uid) {
		command_ = request;
		uid_ = uid;
	}

	int command_;
	std::string uid_;
};

// CRtcWinDemoDlg 对话框
class CRtcWinDemoDlg : public CDialogEx, public WsCallback, public VideoTrackReceiver, public RtcClientStateObserver
{
// 构造
public:
	CRtcWinDemoDlg(CWnd* pParent = nullptr);	// 标准构造函数

// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_RTCWINDEMO_DIALOG };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV 支持

	// WsCallback
	virtual void onConnected();
	virtual void onDisconnected();
	virtual void onConnectionError();
	virtual void onReceiveMessage(std::string& message);

	// VideoTrackReceiver - RTCManager
	virtual void AddTrack(webrtc::VideoTrackInterface* track, bool is_local);
	virtual void RemoveTrack(webrtc::VideoTrackInterface* track);

	// RtcClientStateObserver - RTCClient
	virtual void OnIceCandidate(std::string uid, const std::string mid, const std::string sdp);

	// onReceiveMessage() will call these two functions
	void OnWebSocketResponse(std::shared_ptr<RequestRecord> request, cJSON *json);
	void OnWebSocketNotify(int notify, cJSON *json);

	void JoinRoom();
	void LeaveRoom();

	void Publish(std::string uid, std::string &offer_sdp);
	void UnPublish(std::string uid);

	void Subscribe(std::string uid, std::string mid, std::string &offer_sdp);
	void UnSubscribe(std::string uid, std::string mid);

	void Trickle(std::string uid, std::string mid, std::string sdp);

	void OnPeerNotifyEvent(bool join, cJSON *json);
	void OnStreamNotifyEvent(bool add, cJSON *json);
	void OnChatMessage(cJSON* json);

	long AllocateSession(int request, std::string uid);
	std::shared_ptr<RequestRecord> FindSession(long sessionId);
	void DeallocateSession(long sessionId);

	int ConvertMethodToCommand(std::string method);

	HWND AllocateWindow(bool is_local, std::string track_id);

	std::shared_ptr<RtcClient> AddClient(std::string uid, std::string name);
	std::shared_ptr<RtcClient> GetClient(std::string uid);
	void RemoveClient(std::string uid);
	void RemoveAllClients();

	void PublishLocalStream(bool publish);
	void AppendMessage(CString strMsg, bool title);

// 实现
protected:
	HICON m_hIcon;
	std::shared_ptr<RTCManager> rtc_manager_;
	std::shared_ptr<WsClient>   websocket_;

	std::map<long, std::shared_ptr<RequestRecord>> map_of_send_requests_;
	std::recursive_mutex lock_request_;

	std::map<std::string, std::shared_ptr<RtcClient>> map_of_users_;
	std::map<std::string, std::shared_ptr<webrtc::VideoRenderGDI>> map_of_video_renders_;
	std::recursive_mutex lock_;

	std::queue<size_t> win_spare_;
	std::map<std::string, size_t> map_of_video_wins_;

	std::string room_id_;
	std::string user_name_;
	std::string user_id_;
	std::string mid_;

	std::atomic_bool joined_room_;
	std::atomic_bool local_published_;
	std::atomic_bool auto_publish_;
	long session_id_;

	// 生成的消息映射函数
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()

public:
	afx_msg void OnBnClickedBtnJoin();
	afx_msg void OnBnClickedBtnLeave();
	afx_msg void OnBnClickedBtnSend();
	CRichEditCtrl m_richeditChat;
	CEdit m_editMessage;
	CEdit m_editRoomId;
	CEdit m_editUserId;
	afx_msg void OnDestroy();

	void SendChatMessage(std::string msg);
};
