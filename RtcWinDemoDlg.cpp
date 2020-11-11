
// RtcWinDemoDlg.cpp: 实现文件
//

#include "pch.h"
#include "framework.h"
#include "RtcWinDemo.h"
#include "RtcWinDemoDlg.h"
#include "afxdialogex.h"
#include "RtcWinUtils.h"

#include "json/cJSON.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// 用于应用程序“关于”菜单项的 CAboutDlg 对话框

class CAboutDlg : public CDialogEx
{
public:
	CAboutDlg();

// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_ABOUTBOX };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

// 实现
protected:
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialogEx(IDD_ABOUTBOX)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
END_MESSAGE_MAP()


// CRtcWinDemoDlg 对话框



CRtcWinDemoDlg::CRtcWinDemoDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_RTCWINDEMO_DIALOG, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CRtcWinDemoDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_RICHEDIT_CHAT, m_richeditChat);
	DDX_Control(pDX, IDC_EDIT_MESSAGE, m_editMessage);
	DDX_Control(pDX, IDC_EDIT_ROOM_ID, m_editRoomId);
	DDX_Control(pDX, IDC_EDIT_ROOM_ID2, m_editUserId);
}

BEGIN_MESSAGE_MAP(CRtcWinDemoDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_BTN_JOIN, &CRtcWinDemoDlg::OnBnClickedBtnJoin)
	ON_BN_CLICKED(IDC_BTN_LEAVE, &CRtcWinDemoDlg::OnBnClickedBtnLeave)
	ON_BN_CLICKED(IDC_BTN_SEND, &CRtcWinDemoDlg::OnBnClickedBtnSend)
	ON_WM_DESTROY()
END_MESSAGE_MAP()


// CRtcWinDemoDlg 消息处理程序

BOOL CRtcWinDemoDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// 将“关于...”菜单项添加到系统菜单中。

	// IDM_ABOUTBOX 必须在系统命令范围内。
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != nullptr)
	{
		BOOL bNameValid;
		CString strAboutMenu;
		bNameValid = strAboutMenu.LoadString(IDS_ABOUTBOX);
		ASSERT(bNameValid);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// 设置此对话框的图标。  当应用程序主窗口不是对话框时，框架将自动
	//  执行此操作
	SetIcon(m_hIcon, TRUE);			// 设置大图标
	SetIcon(m_hIcon, FALSE);		// 设置小图标

	// TODO: 在此添加额外的初始化代码
	RTCManagerConfig config;
	config.priority = "RESOLUTION";
	// RTCManager will call AddTrack() and RemoveTrack()
	rtc_manager_ = std::make_shared<RTCManager>(config, this);

	CString strCAFilePath = GetAbsFilePath(WSS_CA_CERT);
	CreateCACert(strCAFilePath);
	websocket_ = std::make_shared<WsClient>(WSS_SERVER_ADDR, WSS_SERVER_PORT, WSS_SERVER_URL, CStringToStdString(strCAFilePath), this);
	websocket_->start();
	
	joined_room_ = false;
	local_published_ = false;
	auto_publish_ = true;

	user_id_ = GenerateUUID();
	srand(GetTickCount());
	session_id_ = rand()%0xFFFF;
	m_editRoomId.SetWindowText(_T("bytertc"));
	m_editUserId.SetWindowText(_T("pc"));
	return TRUE;  // 除非将焦点设置到控件，否则返回 TRUE
}

void CRtcWinDemoDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{ 
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialogEx::OnSysCommand(nID, lParam);
	}
}

// 如果向对话框添加最小化按钮，则需要下面的代码
//  来绘制该图标。  对于使用文档/视图模型的 MFC 应用程序，
//  这将由框架自动完成。

void CRtcWinDemoDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // 用于绘制的设备上下文

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// 使图标在工作区矩形中居中
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// 绘制图标
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

//当用户拖动最小化窗口时系统调用此函数取得光标
//显示。
HCURSOR CRtcWinDemoDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

void CRtcWinDemoDlg::onConnected() {
	TRACE("%s", __FUNCTION__);
}

void CRtcWinDemoDlg::onDisconnected() {
	TRACE("%s", __FUNCTION__);
}

void CRtcWinDemoDlg::onConnectionError() {
	TRACE("%s", __FUNCTION__);
}

void CRtcWinDemoDlg::onReceiveMessage(std::string& message) {
	LogPrintf(message.c_str());

	cJSON *root_json = cJSON_Parse(message.c_str());
	if (NULL == root_json) {
		cJSON_Delete(root_json);
		return;
	}

	cJSON* response = cJSON_GetObjectItem(root_json, "response");
	if (response) {
		cJSON* sid = cJSON_GetObjectItem(root_json, "id");
		if (sid) {
			std::shared_ptr<RequestRecord> request = FindSession((long)cJSON_GetNumberValue(sid));
			OnWebSocketResponse(request, root_json);
		}
	} else {
		cJSON* method = cJSON_GetObjectItem(root_json, "method");
		char* psz = cJSON_GetStringValue(method);
		if (psz) {
			OnWebSocketNotify(ConvertMethodToCommand(psz), root_json);
		}
	}
	cJSON_Delete(root_json);
}

// Todo(kangjx)
//		AllocateWindow for remote track is set to IDC_REMOTE_WIN_1 by default.
void CRtcWinDemoDlg::AddTrack(webrtc::VideoTrackInterface* track, bool is_local) {
	if (track && track->kind() == webrtc::MediaStreamTrackInterface::kVideoKind) {
		std::string track_id = track->id();
		auto it = map_of_video_renders_.find(track_id);
		if (it == map_of_video_renders_.end()) {
			map_of_video_renders_[track_id] = std::make_shared<webrtc::VideoRenderGDI>(track);
			map_of_video_renders_[track_id]->SetWindow(AllocateWindow(is_local, track_id));
			//map_of_video_renders_[track_id]->SetMirrorMode(is_local);
		} else {
			it->second->SetTrack(track);
		}
	}
}

// Todo(kangjx)
//		The window is not removed.
void CRtcWinDemoDlg::RemoveTrack(webrtc::VideoTrackInterface* track) {
	if (track && track->kind() == webrtc::MediaStreamTrackInterface::kVideoKind) {
		std::string track_id = track->id();
		auto it = map_of_video_renders_.find(track_id);
		if (it != map_of_video_renders_.end()) {
			it->second->SetTrack(nullptr);
			map_of_video_renders_.erase(it);
		}
	}
}

void CRtcWinDemoDlg::OnIceCandidate(std::string uid, const std::string mid, const std::string sdp) {
	std::shared_ptr<RtcClient> client = GetClient(uid);
	if (client) {
		if (client->ShouldSendOffer()) {
			std::string sdp;
			if (client->GetOffer(sdp)) {
				if (client->IsLocalUser()) {
					Publish(user_id_, sdp);
				} else {
					Subscribe(client->GetUserId(), client->GetMid(), sdp);
				}
				client->SetSendOffer(false);
			}
		}
		Trickle(uid, client->GetMid(), sdp);
	}
}

HWND CRtcWinDemoDlg::AllocateWindow(bool is_local, std::string track_id) {
	CWnd* pWnd = NULL;
	if (is_local) {
		pWnd = GetDlgItem(IDC_LOCAL_WIN);
	} else {
		pWnd = GetDlgItem(IDC_REMOTE_WIN_1);
		if (pWnd) {
			auto it = map_of_video_renders_.begin();
			while (it != map_of_video_renders_.end()) {
				if (it->second->GetWindow() == pWnd->GetSafeHwnd()) {
					return NULL;
				}
				it++;
			}
		}
	}
	return pWnd ? pWnd->GetSafeHwnd() : NULL;
}

void CRtcWinDemoDlg::OnBnClickedBtnJoin()
{
	CString strRoomId, strUserName;
	if (!joined_room_) {
		m_editRoomId.GetWindowText(strRoomId);
		m_editUserId.GetWindowText(strUserName);

		if (strRoomId.IsEmpty() || strUserName.IsEmpty()) {
			AfxMessageBox(_T("Invalid Room Name or User Id"));
			return;
		}

		room_id_   = CStringToStdString(strRoomId);
		user_name_ = CStringToStdString(strUserName);

		JoinRoom();

		GetDlgItem(IDC_BTN_JOIN)->EnableWindow(FALSE);
		GetDlgItem(IDC_BTN_LEAVE)->EnableWindow(TRUE);
		GetDlgItem(IDC_BTN_SEND)->EnableWindow(TRUE);
		local_published_ = false;
		joined_room_ = true;
	}
}

void CRtcWinDemoDlg::OnBnClickedBtnLeave()
{
	if (joined_room_) {
		if (local_published_) {
			UnPublish(user_id_);
			local_published_ = false;
		}
		LeaveRoom();
		RemoveAllClients();
		GetDlgItem(IDC_BTN_JOIN)->EnableWindow(TRUE);
		GetDlgItem(IDC_BTN_LEAVE)->EnableWindow(FALSE);
		GetDlgItem(IDC_BTN_SEND)->EnableWindow(FALSE);
		joined_room_ = false;
	}
}

void CRtcWinDemoDlg::OnBnClickedBtnSend()
{
	if (joined_room_) {
		CString chatMsg;
		m_editMessage.GetWindowText(chatMsg);
		if (chatMsg.IsEmpty()) {
			AfxMessageBox(_T("Empty input message"));
			return;
		}
		SendChatMessage(CStringToStdString(chatMsg));
		m_editMessage.SetWindowText(_T(""));
	}
	else {
		AfxMessageBox(_T("Cannot send chat message since you have not joined a room."));
	}
}

int CRtcWinDemoDlg::ConvertMethodToCommand(std::string method) {
	if (method == "join") {
		return kWebSocketRequestJoinRoom;
	} else if (method == "leave") {
		return kWebSocketRequestLeaveRoom;
	} else if (method == "publish") {
		return kWebSocketRequestPublish;
	} else if (method == "unpublish") {
		return kWebSocketRequestUnPublish;
	} else if (method == "subscribe") {
		return kWebSocketRequestSubscribe;
	} else if (method == "unsubscribe") {
		return kWebSocketRequestUnSubscribe;
	} else if (method == "peer-join") {
		return kWebSocketNotifyUserJoin;
	} else if (method == "peer-leave") {
		return kWebSocketNotifyUserLeave;
	} else if (method == "stream-add") {
		return kWebSocketNotifyStreamAdd;
	} else if (method == "stream-remove") {
		return kWebSocketNotifyStreamRemove;
	} else if (method == "broadcast") {
		return kWebSocketNotifyBroadcast;
	} else {
		return kWebSocketCommandInvalid;
	}
}

long CRtcWinDemoDlg::AllocateSession(int request, std::string uid) {
	std::lock_guard<std::recursive_mutex> lock_guard(lock_request_);
	InterlockedIncrement(&session_id_);
	map_of_send_requests_[session_id_] = std::make_shared<RequestRecord>(request, uid);
	return session_id_;
}

void CRtcWinDemoDlg::DeallocateSession(long sessionId) {
	std::lock_guard<std::recursive_mutex> lock_guard(lock_request_);
	map_of_send_requests_.erase(sessionId);
}

std::shared_ptr<RequestRecord> CRtcWinDemoDlg::FindSession(long sessionId) {
	std::lock_guard<std::recursive_mutex> lock_guard(lock_request_);
	auto it = map_of_send_requests_.find(sessionId);
	if (it != map_of_send_requests_.end()) {
		return it->second;
	} else {
		return nullptr;
	}
}

void CRtcWinDemoDlg::OnWebSocketResponse(std::shared_ptr<RequestRecord> request, cJSON* root_json) {
	int command = request->command_;

	switch (command) {
	case kWebSocketRequestJoinRoom: {
		cJSON* is_ok = cJSON_GetObjectItem(root_json, "ok");
		if (cJSON_IsTrue(is_ok)) {
			LogPrintf("Join room sucess");
			if (auto_publish_) {
				PublishLocalStream(true);
			}
		} else {
			AfxMessageBox(_T("Join room failed"));
		}
	}
		break;

	case kWebSocketRequestLeaveRoom:
		break;

	case kWebSocketRequestPublish: {
		cJSON* is_ok = cJSON_GetObjectItem(root_json, "ok");
		if (cJSON_IsTrue(is_ok)) {
			cJSON* data_json = cJSON_GetObjectItem(root_json, "data");
			if (data_json) {
				cJSON* mid_json = cJSON_GetObjectItem(data_json, "mid");
				if (mid_json) {
					mid_ = cJSON_GetStringValue(mid_json);
				}

				cJSON* jsep_json = cJSON_GetObjectItem(data_json, "jsep");
				if (jsep_json) {
					cJSON* sdp_json = cJSON_GetObjectItem(jsep_json, "sdp");
					char* sdp = cJSON_GetStringValue(sdp_json);
					if (sdp) {
						LogPrintf("publish_answer:%s, %s", request->uid_.c_str(), sdp);
						std::shared_ptr<RtcClient> client = GetClient(request->uid_);
						if (client) {
							client->SetMid(mid_);

							std::shared_ptr<RTCConnection> conn = client->GetConnection();
							if (conn) {
								conn->SetAnswer(sdp);
							}
						}
					}
				}
			}
			LogPrintf("Publish sucess");
		}
		else {
			AfxMessageBox(_T("Publish failed"));
		}
	}
		break;

	case kWebSocketRequestUnPublish: {
		cJSON* is_ok = cJSON_GetObjectItem(root_json, "ok");
		if (cJSON_IsTrue(is_ok)) {
			LogPrintf("UnPublish success");
		}
		else {
			AfxMessageBox(_T("UnPublish failed"));
		}
	}
		break;

	case kWebSocketRequestSubscribe: {
		cJSON* is_ok = cJSON_GetObjectItem(root_json, "ok");
		if (cJSON_IsTrue(is_ok)) {
			cJSON* data_json = cJSON_GetObjectItem(root_json, "data");
			if (data_json) {
				cJSON* mid_json = cJSON_GetObjectItem(data_json, "mid");
				char *mid = cJSON_GetStringValue(mid_json);

				cJSON* jsep_json = cJSON_GetObjectItem(data_json, "jsep");
				if (jsep_json) {
					cJSON* sdp_json = cJSON_GetObjectItem(jsep_json, "sdp");
					char* sdp = cJSON_GetStringValue(sdp_json);
					if (sdp) {
						LogPrintf("subscribe_answer:%s, %s", mid, sdp);
						std::shared_ptr<RtcClient> client = GetClient(request->uid_);
						if (client) {
							std::shared_ptr<RTCConnection> conn = client->GetConnection();
							if (conn) {
								conn->SetAnswer(sdp);
							}
						}
					}
				}
			}
			LogPrintf("Subscribe sucess");
		}
		else {
			AfxMessageBox(_T("Subscribe failed"));
		}
	}
		break;

	case kWebSocketRequestTrickle: {
	}
		break;

	default:
		break;
	}
}

void CRtcWinDemoDlg::OnWebSocketNotify(int notify, cJSON* root_json) {
	switch (notify) {
	case kWebSocketNotifyUserJoin:
		OnPeerNotifyEvent(true, root_json);
		break;

	case kWebSocketNotifyUserLeave:
		OnPeerNotifyEvent(false, root_json);
		break;

	case kWebSocketNotifyStreamAdd:
		OnStreamNotifyEvent(true, root_json);
		break;

	case kWebSocketNotifyStreamRemove:
		OnStreamNotifyEvent(false, root_json);
		break;

	case kWebSocketNotifyBroadcast:
		OnChatMessage(root_json);
		break;

	default:
		break;
	}
}

/*
{
	"request":true,
	"id":7371749,
	"method":"join",
	"data":{
		"rid":"room_abc",
		"uid":"ebf43195-f2fc-4825-b7b9-4ddf38390bb7",
		"info":{
			"name":"user_111"
		}
	}
}
*/
void CRtcWinDemoDlg::JoinRoom() {
	cJSON* root_json = cJSON_CreateObject();

	cJSON_AddItemToObject(root_json, "request", cJSON_CreateBool(true));
	cJSON_AddItemToObject(root_json, "id", cJSON_CreateNumber(AllocateSession(kWebSocketRequestJoinRoom, user_id_)));
	cJSON_AddItemToObject(root_json, "method", cJSON_CreateString("join"));

	cJSON* data_json = cJSON_CreateObject();
	cJSON_AddItemToObject(root_json, "data", data_json);
	cJSON_AddItemToObject(data_json, "rid", cJSON_CreateString(room_id_.c_str()));
	cJSON_AddItemToObject(data_json, "uid", cJSON_CreateString(user_id_.c_str()));

	cJSON* info_json = cJSON_CreateObject();
	cJSON_AddItemToObject(data_json, "info", info_json);
	cJSON_AddItemToObject(info_json, "name", cJSON_CreateString(user_name_.c_str()));

	char* request = cJSON_Print(root_json);
	cJSON_Delete(root_json);

	LogPrintf(request);
	if (websocket_ && request) {
		websocket_->sendMessage(std::make_shared<std::string>(request));
	}

	if (request) {
		cJSON_free(request);
	}
}

/*
* {
    "request":true,
    "id":1774518,
    "method":"leave",
    "data":{
        "rid":"room_abc",
        "uid":"d6a9126c-7326-4763-86d2-89a857a1f6af"
    }
}
*/
void CRtcWinDemoDlg::LeaveRoom() {
	cJSON* root_json = cJSON_CreateObject();

	cJSON_AddItemToObject(root_json, "request", cJSON_CreateBool(true));
	cJSON_AddItemToObject(root_json, "id", cJSON_CreateNumber(AllocateSession(kWebSocketRequestLeaveRoom, user_id_)));
	cJSON_AddItemToObject(root_json, "method", cJSON_CreateString("leave"));

	cJSON* data_json = cJSON_CreateObject();
	cJSON_AddItemToObject(root_json, "data", data_json);
	cJSON_AddItemToObject(data_json, "rid", cJSON_CreateString(room_id_.c_str()));
	cJSON_AddItemToObject(data_json, "uid", cJSON_CreateString(user_id_.c_str()));

	char* request = cJSON_Print(root_json);
	cJSON_Delete(root_json);

	LogPrintf(request);
	if (websocket_ && request) {
		websocket_->sendMessage(std::make_shared<std::string>(request));
	}
	if (request) {
		cJSON_free(request);
	}
}

/*
{
    "request":true,
    "id":86052,
    "method":"publish",
    "data":{
        "rid":"room_abc",
        "jsep":{
            "type":"offer",
            "sdp":"sdp_body"
		},
		"options":{
			"codec":"VP8",
			"bandwidth":1024
		}
	}
}
*/
void CRtcWinDemoDlg::Publish(std::string uid, std::string& offer_sdp) {
	cJSON* root_json = cJSON_CreateObject();

	cJSON_AddItemToObject(root_json, "request", cJSON_CreateBool(true));
	cJSON_AddItemToObject(root_json, "id", cJSON_CreateNumber(AllocateSession(kWebSocketRequestPublish, uid)));
	cJSON_AddItemToObject(root_json, "method", cJSON_CreateString("publish"));

	cJSON* data_json = cJSON_CreateObject();
	cJSON_AddItemToObject(root_json, "data", data_json);
	cJSON_AddItemToObject(data_json, "uid", cJSON_CreateString(uid.c_str()));
	cJSON_AddItemToObject(data_json, "rid", cJSON_CreateString(room_id_.c_str()));

	cJSON* jsep_json = cJSON_CreateObject();
	cJSON_AddItemToObject(data_json, "jsep", jsep_json);
	cJSON_AddItemToObject(jsep_json, "type", cJSON_CreateString("offer"));
	cJSON_AddItemToObject(jsep_json, "sdp", cJSON_CreateString(offer_sdp.c_str()));

	cJSON* option_json = cJSON_CreateObject();
	cJSON_AddItemToObject(data_json,   "options", option_json);
	cJSON_AddItemToObject(option_json, "codec", cJSON_CreateString("VP8"));
	cJSON_AddItemToObject(option_json, "bandwidth", cJSON_CreateNumber(1024));

	LogPrintf("publish_offer:%s", offer_sdp.c_str());

	char* request = cJSON_Print(root_json);
	cJSON_Minify(request);

	cJSON_Delete(root_json);
	LogPrintf("publish_request:%s", request);
	if (websocket_ && request) {
		websocket_->sendMessage(std::make_shared<std::string>(request));
	}

	if (request) {
		cJSON_free(request);
	}
}

/*
* {
    "request":true,
    "id":9527736,
    "method":"unpublish",
    "data":{
        "rid":"room_abc",
        "mid":"4919f8ad-a068-4b49-977f-9db3f858465f"
    }
}
*/
void CRtcWinDemoDlg::UnPublish(std::string uid) {
	cJSON* root_json = cJSON_CreateObject();

	cJSON_AddItemToObject(root_json, "request", cJSON_CreateBool(true));
	cJSON_AddItemToObject(root_json, "id", cJSON_CreateNumber(AllocateSession(kWebSocketRequestUnPublish, uid)));
	cJSON_AddItemToObject(root_json, "method", cJSON_CreateString("unpublish"));

	cJSON* data_json = cJSON_CreateObject();
	cJSON_AddItemToObject(root_json, "data", data_json);
	cJSON_AddItemToObject(data_json, "mid", cJSON_CreateString(mid_.c_str()));
	cJSON_AddItemToObject(data_json, "rid", cJSON_CreateString(room_id_.c_str()));

	char* request = cJSON_Print(root_json);
	cJSON_Minify(request);

	cJSON_Delete(root_json);
	LogPrintf("unpublish_request:%s", request);
	if (websocket_ && request) {
		websocket_->sendMessage(std::make_shared<std::string>(request));
	}

	if (request) {
		cJSON_free(request);
	}
}

/*
{
	"request":true,
	"id":6216563,
	"method":"subscribe",
	"data":{
		"rid":"room_abc",
		"jsep":{
			"type":"offer",
			"sdp":"sdp_body",
			},
		"mid":"4919f8ad-a068-4b49-977f-9db3f858465f"
	}
}
*/
void CRtcWinDemoDlg::Subscribe(std::string uid, std::string mid, std::string &offer_sdp) {
	cJSON* root_json = cJSON_CreateObject();

	cJSON_AddItemToObject(root_json, "request", cJSON_CreateBool(true));
	cJSON_AddItemToObject(root_json, "id", cJSON_CreateNumber(AllocateSession(kWebSocketRequestSubscribe, uid)));
	cJSON_AddItemToObject(root_json, "method", cJSON_CreateString("subscribe"));

	cJSON* data_json = cJSON_CreateObject();
	cJSON_AddItemToObject(root_json, "data", data_json);
	cJSON_AddItemToObject(data_json, "rid", cJSON_CreateString(room_id_.c_str()));

	cJSON* jsep_json = cJSON_CreateObject();
	cJSON_AddItemToObject(data_json, "jsep", jsep_json);
	cJSON_AddItemToObject(jsep_json, "type", cJSON_CreateString("offer"));
	cJSON_AddItemToObject(jsep_json, "sdp", cJSON_CreateString(offer_sdp.c_str()));

	cJSON_AddItemToObject(data_json, "mid", cJSON_CreateString(mid.c_str()));
	LogPrintf("subscribe_offer:%s", offer_sdp.c_str());
	char* request = cJSON_Print(root_json);
	cJSON_Delete(root_json);
	LogPrintf("subscribe_request:%s", request);
	if (websocket_ && request) {
		websocket_->sendMessage(std::make_shared<std::string>(request));
	}

	if (request) {
		cJSON_free(request);
	}
}

/*
{
	"request":true,
	"id":9651334,
	"method":"unsubscribe",
	"data":{
		"mid":"ef9a6ee4-e0c1-4098-890c-4950c3775a82"
	}
}
*/
void CRtcWinDemoDlg::UnSubscribe(std::string uid, std::string mid) {
	cJSON* root_json = cJSON_CreateObject();

	cJSON_AddItemToObject(root_json, "request", cJSON_CreateBool(true));
	cJSON_AddItemToObject(root_json, "id", cJSON_CreateNumber(AllocateSession(kWebSocketRequestUnSubscribe, uid)));
	cJSON_AddItemToObject(root_json, "method", cJSON_CreateString("unsubscribe"));

	cJSON* data_json = cJSON_CreateObject();
	cJSON_AddItemToObject(root_json, "data", data_json);
	cJSON_AddItemToObject(data_json, "mid", cJSON_CreateString(mid.c_str()));

	char* request = cJSON_Print(root_json);
	cJSON_Delete(root_json);

	if (websocket_ && request) {
		websocket_->sendMessage(std::make_shared<std::string>(request));
	}

	if (request) {
		cJSON_free(request);
	}
}

void CRtcWinDemoDlg::Trickle(std::string uid, std::string mid, std::string sdp) {
	cJSON* root_json = cJSON_CreateObject();

	cJSON_AddItemToObject(root_json, "request", cJSON_CreateBool(true));
	cJSON_AddItemToObject(root_json, "id", cJSON_CreateNumber(AllocateSession(kWebSocketRequestTrickle, uid)));
	cJSON_AddItemToObject(root_json, "method", cJSON_CreateString("trickle"));

	cJSON* data_json = cJSON_CreateObject();
	cJSON_AddItemToObject(root_json, "data", data_json);
	cJSON_AddItemToObject(data_json, "rid", cJSON_CreateString(room_id_.c_str()));
	cJSON_AddItemToObject(data_json, "uid", cJSON_CreateString(uid.c_str()));
	cJSON_AddItemToObject(data_json, "mid", cJSON_CreateString(mid.c_str()));

	cJSON* trickle_json = cJSON_CreateObject();
	cJSON_AddItemToObject(data_json, "trickle", trickle_json);
	cJSON_AddItemToObject(trickle_json, "candidate", cJSON_CreateString(sdp.c_str()));

	LogPrintf("trickle_candidate:%s", sdp.c_str());
	char* request = cJSON_Print(root_json);
	cJSON_Delete(root_json);
	LogPrintf("trickle_request:%s", request);
	if (websocket_ && request) {
		websocket_->sendMessage(std::make_shared<std::string>(request));
	}

	if (request) {
		cJSON_free(request);
	}
}

/*
// JOIN
{
	"notification":true,
	"method" : "peer-join",
	"data" : {
		"info":{
			"name":"user_222"
		},
		"rid" : "room_abc",
		"uid" : "a5b71957-5b61-4c02-b761-0e9f26198d83"
	}
}

// LEAVE
{
	"notification":true,
	"method":"peer-leave",
	"data":{
		"rid":"room_abc",
		"uid":"2e056c3c-b945-4290-a8b5-3affe41c55a3"
	}
}
*/
void CRtcWinDemoDlg::OnPeerNotifyEvent(bool join, cJSON *root_json) {
	std::string room, name, uid;

	cJSON* data_json = cJSON_GetObjectItem(root_json, "data");
	if (data_json) {
		cJSON* uid_json = cJSON_GetObjectItem(data_json, "uid");
		uid = cJSON_GetStringValue(uid_json);

		cJSON* info_json = cJSON_GetObjectItem(data_json, "info");
		if (info_json) {
			cJSON* name_json = cJSON_GetObjectItem(info_json, "name");
			name = cJSON_GetStringValue(name_json);
		}

		if (uid.empty()) return;

		if (join) {
			AddClient(uid, name);
		} else {
			RemoveClient(uid);
		}
	}
}

/*
ADD 
{
	"notification":true,
	"method":"stream-add",
	"data":{
		"dc":"dc1",
		"nid":"sfu-vvDWSSrtq7rV",
		"rid":"room_abc",
		"uid":"ebf43195-f2fc-4825-b7b9-4ddf38390bb7",
		"mid":"4919f8ad-a068-4b49-977f-9db3f858465f",
		"info":{
			"name":"user_111"
		},
		"tracks":{
			"c4ea0ee0-5eb0-4979-ab29-c8fb14542c45 32e81f66-2fa4-4267-b30b-f6bec6a68fc0":[
				{
					"id":"32e81f66-2fa4-4267-b30b-f6bec6a68fc0",
					"ssrc":2084374747,
					"pt":111,
					"type":"audio",
					"codec":"OPUS",
					"fmtp":""
				}
			],
			"c4ea0ee0-5eb0-4979-ab29-c8fb14542c45 be8fdc28-a62b-443e-8f2b-53cda5d97946":[
				{
					"id":"be8fdc28-a62b-443e-8f2b-53cda5d97946",
					"ssrc":871188166,
					"pt":96,
					"type":"video",
					"codec":"VP8",
					"fmtp":""
				}
			]
		}
	}
}

REMOVE
{
	"notification":true,
	"method":"stream-remove",
	"data":{
		"dc":"dc1",
		"mid":"4919f8ad-a068-4b49-977f-9db3f858465f",
		"rid":"room_abc",
		"uid":"ebf43195-f2fc-4825-b7b9-4ddf38390bb7"
	}
}
*/
void CRtcWinDemoDlg::OnStreamNotifyEvent(bool add, cJSON * root_json) {
	std::string uid, mid, name;

	cJSON* data_json = cJSON_GetObjectItem(root_json, "data");
	if (data_json) {
		cJSON* uid_json = cJSON_GetObjectItem(data_json, "uid");
		uid = cJSON_GetStringValue(uid_json);

		cJSON* mid_json = cJSON_GetObjectItem(data_json, "mid");
		mid = cJSON_GetStringValue(mid_json);

		cJSON* info_json = cJSON_GetObjectItem(data_json, "info");
		if (info_json) {
			cJSON* name_json = cJSON_GetObjectItem(info_json, "name");
			name = cJSON_GetStringValue(name_json);
		}

		if (uid.empty() || mid.empty()) return;

		if (add) {
			std::shared_ptr<RtcClient> client = AddClient(uid, name);
			if (client) {
				client->SetSendOffer(true);
				client->SetMid(mid);

				auto on_create_offer = [this, uid, mid](webrtc::SessionDescriptionInterface* desc) {
					//std::string sdp;
					//desc->ToString(&sdp);
					//Subscribe(uid, mid, sdp);
				};

				std::shared_ptr<RTCConnection> conn = client->GetConnection();
				if (conn) {
					conn->CreateOffer(false, on_create_offer);
				}
			}
		} else {
			UnSubscribe(uid, mid);
		}
	}
}

/*
* {"notification":true,"method":"broadcast","data":{"info":{"msg":"hello","senderName":"vivo"},"rid":"byte","uid":"2556a3dc-32f1-44c5-85b0-4516fa92f8d1"}}
*/
void CRtcWinDemoDlg::OnChatMessage(cJSON* root_json) {
	std::string room, sender, message;
	cJSON* data_json = cJSON_GetObjectItem(root_json, "data");
	if (data_json) {
		cJSON* rid_json = cJSON_GetObjectItem(data_json, "rid");
		room = cJSON_GetStringValue(rid_json);
		if (room != room_id_) return;

		cJSON* info_json = cJSON_GetObjectItem(data_json, "info");
		if (info_json) {
			cJSON* sender_json = cJSON_GetObjectItem(info_json, "senderName");
			sender = cJSON_GetStringValue(sender_json);

			cJSON* msg_json = cJSON_GetObjectItem(info_json, "msg");
			message = cJSON_GetStringValue(msg_json);

			if (!sender.empty() && !message.empty()) {
				sender += ":\n";
				AppendMessage(StdStringToCString(sender), true);
				message += "\n";
				AppendMessage(StdStringToCString(message), false);
			}
		}
	}

}

/*
* {"notification":true,"method":"broadcast","data":{"info":{"msg":"hello","senderName":"vivo"},"rid":"byte","uid":"2556a3dc-32f1-44c5-85b0-4516fa92f8d1"}}
*/
void CRtcWinDemoDlg::SendChatMessage(std::string msg) {
	cJSON* root_json = cJSON_CreateObject();

	cJSON_AddItemToObject(root_json, "notification", cJSON_CreateBool(true));
	cJSON_AddItemToObject(root_json, "method", cJSON_CreateString("broadcast"));

	cJSON* data_json = cJSON_CreateObject();
	cJSON_AddItemToObject(root_json, "data", data_json);
	cJSON_AddItemToObject(data_json, "rid", cJSON_CreateString(room_id_.c_str()));
	cJSON_AddItemToObject(data_json, "uid", cJSON_CreateString(user_id_.c_str()));

	cJSON* info_json = cJSON_CreateObject();
	cJSON_AddItemToObject(data_json, "info", info_json);
	cJSON_AddItemToObject(info_json, "senderName", cJSON_CreateString(user_name_.c_str()));
	cJSON_AddItemToObject(info_json, "msg", cJSON_CreateString(msg.c_str()));

	char* request = cJSON_Print(root_json);
	cJSON_Delete(root_json);

	LogPrintf(request);
	if (websocket_ && request) {
		websocket_->sendMessage(std::make_shared<std::string>(request));
	}

	if (request) {
		cJSON_free(request);
	}
}

void CRtcWinDemoDlg::PublishLocalStream(bool publish) {
	if (local_published_ != publish) {
		if (publish) {
			std::shared_ptr<RtcClient> client = AddClient(user_id_, user_name_);
			if (client) {
				client->SetSendOffer(true);
				std::shared_ptr<RTCConnection> conn = client->GetConnection();
				if (conn) {
					auto on_create_offer = [this](webrtc::SessionDescriptionInterface* desc) {
						//std::string sdp;
						//desc->ToString(&sdp);
						//Publish(user_id_, sdp);
					};

					if (conn) {
						conn->CreateOffer(true, on_create_offer);
					}
				}
			}
		}
		else {
			UnPublish(user_id_);
			RemoveClient(user_id_);
		}

		local_published_ = publish;
	}
}

std::shared_ptr<RtcClient> CRtcWinDemoDlg::AddClient(std::string uid, std::string name) {
	std::lock_guard<std::recursive_mutex> lock_guard(lock_);
	auto it = map_of_users_.find(uid);
	if (it == map_of_users_.end()) {
		map_of_users_[uid] = std::make_shared<RtcClient>(uid, name, rtc_manager_, uid == user_id_);
		if (map_of_users_[uid]) {
			// RTCStateObserver will call OnIceCandidate()
			map_of_users_[uid]->SetStateObserver(this);
			map_of_users_[uid]->CreatePeerConnection();
		}
	}
	return map_of_users_[uid];
}

std::shared_ptr<RtcClient> CRtcWinDemoDlg::GetClient(std::string uid) {
	std::lock_guard<std::recursive_mutex> lock_guard(lock_);
	auto it = map_of_users_.find(uid);
	if (it != map_of_users_.end()) {
		return it->second;
	} else {
		return nullptr;
	}
}

void CRtcWinDemoDlg::RemoveClient(std::string uid) {
	std::lock_guard<std::recursive_mutex> lock_guard(lock_);
	auto it = map_of_users_.find(uid);
	if (it != map_of_users_.end()) {
		it->second = nullptr;
		map_of_users_.erase(it);
	}
}

void CRtcWinDemoDlg::RemoveAllClients() {
	std::lock_guard<std::recursive_mutex> lock_guard(lock_);
	map_of_users_.clear();
	map_of_video_renders_.clear();
}

void CRtcWinDemoDlg::AppendMessage(CString strMsg, bool title) {
	CHARFORMAT cf = {0};

	cf.cbSize      = sizeof(cf);
	cf.dwMask      = CFM_BOLD | CFM_ITALIC | CFM_COLOR;
	cf.dwEffects   = 0;//title ? CFE_BOLD : 0;
	cf.crTextColor = title ? RGB(0x1E, 0x90, 0xFF) : RGB(0, 0, 0);

	int nOldLines = m_richeditChat.GetLineCount();
	m_richeditChat.SetSel(m_richeditChat.GetWindowTextLength(), -1);
	m_richeditChat.SetSelectionCharFormat(cf);
	m_richeditChat.ReplaceSel(strMsg);
	m_richeditChat.LineScroll(m_richeditChat.GetLineCount() - nOldLines);
}

void CRtcWinDemoDlg::OnDestroy() {
	if (joined_room_) {
		if (local_published_) {
			UnPublish(user_id_);
			local_published_ = false;
		}
		LeaveRoom();
		RemoveAllClients();
		joined_room_ = false;
	}
	websocket_ = nullptr;
	rtc_manager_ = nullptr;
	__super::OnDestroy();
}
