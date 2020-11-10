#include "pch.h"
#include "WsClient.h"
#include <utility>
#include <iostream>
#include <assert.h>

#define WS_MESSAGE_MAX_SIZE			32768
#define WS_KEEPALIVE_INTERVAL		10

WsClient::WsClient(std::string address, int port, std::string url, std::string ca_path, WsCallback* callback) {
	this->address_ = address;
	this->port_    = port;
	this->path_    = url;
	this->ca_path_ = ca_path;

	TRACE("[ws Reconnection] Connecting to ==> %s:%d%s\n", this->address_.c_str(), this->port_, this->path_.c_str());

	this->lws_thread_ = nullptr;
	this->stop_ = false;
	this->no_msg_ = true;
	this->callback_ = callback;
	this->last_ping_tick_ = 0;
	this->message_ = "";
	this->lws_client_ = nullptr;

	// please check https://libwebsockets.org/lws-api-doc-master/html/structlws__protocols.html
	// adjust to suit your needs
	protocols_[0] = {
			"protoo",
			lwsCallback,
			0,
			1024,
			0,
			this,
			10
	};

	protocols_[1] = {
			nullptr,
			nullptr,
			0,
			0
	};

	buffer_ = new unsigned char[WS_MESSAGE_MAX_SIZE];
}

//Join operation to ensure that the thread exits
WsClient::~WsClient() {
	// Try to stop
	stop();

	if (buffer_) {
		delete[] buffer_;
		buffer_ = nullptr;
	}
}

int WsClient::lwsCallback(struct lws* wsi, enum lws_callback_reasons reason, void* user_data, void* in, size_t len) {
	WsClient* client = (WsClient*)user_data;

	switch (reason) {
	case LWS_CALLBACK_CLIENT_ESTABLISHED: {
		TRACE("[ws connection succeeded]\n");
		if (client->callback_) {
			client->callback_->onConnected();
		}
		break;
	}

	case LWS_CALLBACK_CLIENT_CONNECTION_ERROR: {
		TRACE("[ws error connecting]\n");
		if (client->callback_) {
			client->callback_->onConnectionError();
		}
		break;
	}

	case LWS_CALLBACK_CLOSED: {
		TRACE("[ws Connection closed]");
		client->lws_client_ = nullptr;
		if (client->callback_) {
			client->callback_->onDisconnected();
		}
		break;
	}

	case LWS_CALLBACK_CLIENT_RECEIVE: {
		if (len > 0) {
			client->message_.append((const char *)in, len);
			if (lws_is_final_fragment(wsi)) {
				if (client->callback_) {
					client->callback_->onReceiveMessage(client->message_);
				}
				client->message_.clear();
			}
		}
		break;
	}

	case LWS_CALLBACK_CLIENT_WRITEABLE: {
		client->drainMessageQueue(wsi);
		break;
	}

	case LWS_CALLBACK_WSI_DESTROY: {
		TRACE("[ws Connection destruction]\n");
		client->lws_client_ = nullptr;
		break;
	}

	case LWS_SERVER_OPTION_FALLBACK_TO_RAW: {
		TRACE("[ws Connection destruction]\n");
		client->lws_client_ = nullptr;
		break;
	}

	case LWS_CALLBACK_CLIENT_RECEIVE_PONG: {
		TRACE("[ws Connection PONG]\n");
		break;
	}

	default: {
		TRACE("on_websocket_callback:%d\n", reason);
		break;
	}
	}
	return 0;
}

void WsClient::drainMessageQueue(struct lws* wsi) {
	std::lock_guard<std::recursive_mutex> lock_guard(lock_);
	if (!message_queue_.empty()) {
		std::shared_ptr<std::string> message = message_queue_.front();

		unsigned char* p = buffer_ + LWS_PRE;
		size_t len = static_cast<size_t>(snprintf((char *)p, WS_MESSAGE_MAX_SIZE, "%s", message->c_str()));

		int offset = 0;
		while (offset < len) {
			int ret = lws_write(wsi, p + offset, len - offset, LWS_WRITE_TEXT);
			if (ret <= 0) {
				break;
			}
			offset += ret;
		}

		message_queue_.pop();
		no_msg_ = message_queue_.empty();
	} else if (GetTickCount64() - last_ping_tick_ >= WS_KEEPALIVE_INTERVAL*1000) {
		uint8_t ping[LWS_PRE + 125];
		lws_write(wsi, ping + LWS_PRE, 0, LWS_WRITE_PING);
		TRACE("[ws Connection PING]\n");
		last_ping_tick_ = GetTickCount64();
	}
}

void WsClient::sendMessage(std::shared_ptr<std::string> msg) {
	std::lock_guard<std::recursive_mutex> lock_guard(lock_);
	message_queue_.push(msg);
	no_msg_ = false;
	if (lws_client_) {
		lws_callback_on_writable(lws_client_);
	}
}

void WsClient::lwsThread(WsClient* pClient) {
	pClient->lwsTask();
}

void lws_log_func(int level, const char* line) {
	TRACE(line);
}

void WsClient::lwsTask() {
	TRACE("[ws Thread start]\n");

	struct lws_context_creation_info ctxInfo {};
	memset(&ctxInfo, 0, sizeof(ctxInfo));

	ctxInfo.port = CONTEXT_PORT_NO_LISTEN;
	ctxInfo.iface = NULL;
	ctxInfo.protocols = protocols_;
	ctxInfo.ssl_cert_filepath = NULL;
	ctxInfo.ssl_private_key_filepath = NULL;

	ctxInfo.ssl_ca_filepath = ca_path_.c_str();
	ctxInfo.extensions = NULL;
	ctxInfo.gid = -1;
	ctxInfo.uid = -1;
	ctxInfo.options |= LWS_SERVER_OPTION_DO_SSL_GLOBAL_INIT;
	ctxInfo.fd_limit_per_thread = 1024;
	ctxInfo.max_http_header_pool = 1024;
	ctxInfo.ws_ping_pong_interval = WS_KEEPALIVE_INTERVAL;
	ctxInfo.ka_time = WS_KEEPALIVE_INTERVAL;
	ctxInfo.ka_probes = WS_KEEPALIVE_INTERVAL;
	ctxInfo.ka_interval = WS_KEEPALIVE_INTERVAL;
	struct lws_context* wsContext = lws_create_context(&ctxInfo);

	if (wsContext == nullptr) {
		TRACE("[ERROR] lws_create_context() failure\n");
		TRACE("[ws Thread exit]\n");
		return;
	}

	//lws_set_log_level(LLL_DEBUG, lws_log_func);

	struct lws_client_connect_info clientInfo;
	memset(&clientInfo, 0, sizeof(clientInfo));

	clientInfo.context = wsContext;
	clientInfo.ssl_connection = LCCSCF_USE_SSL | LCCSCF_ALLOW_SELFSIGNED | LCCSCF_SKIP_SERVER_CERT_HOSTNAME_CHECK/* | LCCSCF_ALLOW_INSECURE*/;
	clientInfo.host = this->address_.c_str();
	clientInfo.address = this->address_.c_str();
	clientInfo.port = this->port_;
	clientInfo.path = this->path_.c_str();
	clientInfo.origin = "origin";
	clientInfo.ietf_version_or_minus_one = -1;
	clientInfo.protocol = protocols_[0].name;
	clientInfo.userdata = this;
	lws_client_ = lws_client_connect_via_info(&clientInfo);

	int countdown = 5;
	while (!stop_ || !no_msg_) {
		if (!lws_client_) {
			std::this_thread::sleep_for(std::chrono::milliseconds(1));
			--countdown;
			if (stop_) break;
			if (countdown == 0) {
				TRACE("[ws Reconnection] Reconnection to ==> %s:%d%s\n", this->address_.c_str(), this->port_, this->path_.c_str());
				lws_client_ = lws_client_connect_via_info(&clientInfo);
				countdown = 5;
			} else {
				continue;
			}
		}
		lws_service(wsContext, 50);
	}

	lws_context_destroy(wsContext);
	lws_client_ = nullptr;
	TRACE("[ws Thread exit]\n");
}

void WsClient::start() {
	if (lws_thread_ != nullptr) {
		TRACE("[ERROR] ws Thread is already running\n");
	} else {
		lws_thread_ = new std::thread(lwsThread, this);
	}
}

void WsClient::stop() {
	stop_ = true;
	if (lws_thread_ && lws_thread_->joinable()) {
		lws_thread_->join();
	}

	if (lws_thread_) {
		delete lws_thread_;
		lws_thread_ = nullptr;
	}
}
