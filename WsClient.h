#include "libwebsockets.h"
#include <string>
#include <map>
#include <thread>
#include <mutex>
#include <queue>
#include <atomic>

class WsCallback {
public:
    virtual ~WsCallback() = default;

    virtual void onConnected() = 0;
    virtual void onDisconnected() = 0;
    virtual void onConnectionError() = 0;

    virtual void onReceiveMessage(std::string& message) = 0;
};

class WsClient {
public:
    WsClient(std::string address, int port, std::string url, std::string ca_path, WsCallback *callback);
    ~WsClient();

    void start();
    void stop();
    void sendMessage(std::shared_ptr<std::string> message);
    void drainMessageQueue(struct lws* wsi);

private:
    static int  lwsCallback(struct lws* wsi, enum lws_callback_reasons reason,
        void* userData, void* in, size_t len);
    static void lwsThread(WsClient* pClient);
    void        lwsTask();

    std::string address_;
    int         port_;
    std::string path_;
    std::string ca_path_;

    uint64_t    last_ping_tick_;

    // websocket sending and receiving thread
    std::thread *lws_thread_;

    std::recursive_mutex lock_;

    // It will be used when the websocket is reconnected.
    struct lws *lws_client_;

    // Thread exit flag stop && noMsg
    volatile bool stop_;

    // noMsg is used to ensure that messages leaving the room are sent out before the thread exits
    std::atomic_bool no_msg_;

    struct lws_protocols protocols_[2];

    unsigned char *buffer_;

    // Used to store messages received by the websocket
    std::string message_;

    // Pending message queue
    std::queue<std::shared_ptr<std::string>> message_queue_;

    // Callback, please do not handle time-consuming operations in callbacks
    WsCallback *callback_;
};