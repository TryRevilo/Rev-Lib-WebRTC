#include <variant>
#include <string>
#include <iostream>

#include <jni.h>
#include <android/log.h>

#include <rtc/rtc.hpp>

#include <nlohmann/json.hpp>

#include <algorithm>
#include <future>
#include <memory>
#include <random>
#include <stdexcept>
#include <thread>
#include <unordered_map>

#include "../Rev-Lib-WebRTC.cpp"

using namespace std::chrono_literals;
using std::shared_ptr;
using std::weak_ptr;

template<class T>
weak_ptr<T> make_weak_ptr(shared_ptr<T> ptr) { return ptr; }

using nlohmann::json;

std::string localId;
std::string revTargetId;

std::unordered_map<std::string, std::string> revTestData;

std::unordered_map<std::string, shared_ptr<rtc::PeerConnection>> revPeerConnectionMap;
std::unordered_map<std::string, shared_ptr<rtc::DataChannel>> revDataChannelMap;

std::shared_ptr<rtc::DataChannel> revInitDataChannel(std::string localId, std::string revTargetId);

std::shared_ptr<rtc::PeerConnection> createPeerConnection(const rtc::Configuration &config, std::weak_ptr<rtc::WebSocket> wws, std::string revTargetId);

std::string randomId(size_t length);

shared_ptr<rtc::WebSocket> ws = std::make_shared<rtc::WebSocket>();

std::string revGetTestStr(std::string revKey) {
    return revTestData.at(revKey);
}

std::string revSetTestStr(std::string revKey, std::string revVal) {
    revTestData.emplace(revKey, revVal);

    return (revTestData.at(revKey));
}

rtc::Configuration revGetConfig() {
    rtc::Configuration config;

    std::string stunServer = "stun.l.google.com";
    stunServer += stunServer + ":" + std::to_string(19302);

    __android_log_print(ANDROID_LOG_INFO, "MyApp", "STUN server is  %s\n", stunServer.c_str());

    config.iceServers.emplace_back(stunServer);

    return config;
}

rtc::Configuration config = revGetConfig();

void revInitWS(std::string url, std::string revLocalId) {
    localId = revLocalId;

    __android_log_print(ANDROID_LOG_INFO, "MyApp", ">>> url %s\n", url.c_str());

    std::promise<void> wsPromise;
    auto wsFuture = wsPromise.get_future();

    ws->onOpen([&wsPromise]() {
        __android_log_print(ANDROID_LOG_INFO, "MyApp", ">>> WebSocket connected, signaling ready\n");
        wsPromise.set_value();
    });

    ws->onError([&wsPromise](std::string s) {
        std::cout << "WebSocket error" << std::endl;
        wsPromise.set_exception(std::make_exception_ptr(std::runtime_error(s)));

        if (revDataChannelMap.size()) {
            for (auto i = revDataChannelMap.begin(); i != revDataChannelMap.end(); i++) {
                auto revCurrDC = i->second;

                if (revCurrDC)
                    revCurrDC->close();
            }
        }

        if (revPeerConnectionMap.size()) {
            for (auto i = revPeerConnectionMap.begin(); i != revPeerConnectionMap.end(); i++) {
                auto revCurrPC = i->second;

                if (revCurrPC)
                    revCurrPC->close();
            }
        }

        ws->close();
    });

    ws->onClosed([]() { __android_log_print(ANDROID_LOG_INFO, "MyApp", ">>> WebSocket closed\n"); });

    ws->onMessage([wws = make_weak_ptr(ws), revLocalId](auto data) {
        __android_log_print(ANDROID_LOG_WARN, "MyApp", ">>> WS -> MESSAGE <<< %s\n", std::get<std::string>(data).c_str());

        // data holds either std::string or rtc::binary
        if (!std::holds_alternative<std::string>(data))
            return;

        json message = json::parse(std::get<std::string>(data));

        std::string s = message.dump();
        __android_log_print(ANDROID_LOG_WARN, "MyApp", ">>> WS -> MESSAGE <<< %s\n", s.c_str());

        auto it = message.find("id");
        if (it == message.end())
            return;

        std::string id = it->get<std::string>();

        // Set the id in React
        if (!id.empty()) {
            revInitNativeEvent("revSetOnlineUserId", id);
        }

        __android_log_print(ANDROID_LOG_WARN, "MyApp", ">>> onMessage ID : %s\n", id.c_str());

        it = message.find("type");

        if (it == message.end())
            return;

        auto type = it->get<std::string>();

        shared_ptr<rtc::PeerConnection> pc;

        if (auto jt = revPeerConnectionMap.find(id); jt != revPeerConnectionMap.end()) {
            pc = jt->second;

            __android_log_print(ANDROID_LOG_INFO, "MyApp", ">>> WS Peer Conn (%d) already established : %s\n", pc->gatheringState(), id.c_str());
        } else if (type == "offer") {
            __android_log_print(ANDROID_LOG_INFO, "MyApp", ">>> Answering to : %s\n", id.c_str());
            pc = createPeerConnection(config, wws, id);
        } else if (type == "revConnEntity") {
            revTargetId = (message.find("revTargetId"))->get<std::string>();
            std::string revLocal = (message.find("id"))->get<std::string>();

            __android_log_print(ANDROID_LOG_INFO, "MyApp", ">>> revLocalId : %s >>> revTargetId : %s\n", revLocalId.c_str(), revTargetId.c_str());

            revInitDataChannel(revLocalId, revTargetId);
        } else {
            return;
        }

        if (type == "offer" || type == "answer") {
            auto sdp = message["description"].get<std::string>();
            pc->setRemoteDescription(rtc::Description(sdp, type));
        } else if (type == "candidate") {
            auto sdp = message["candidate"].get<std::string>();
            auto mid = message["mid"].get<std::string>();
            pc->addRemoteCandidate(rtc::Candidate(sdp, mid));
        }
    });

    std::string revURL = url + ":" + std::to_string(8000) + "/" + revLocalId;

    __android_log_print(ANDROID_LOG_INFO, "MyApp", ">>> WebSocket URL is : %s\n", url.c_str());

    ws->open(revURL);

    __android_log_print(ANDROID_LOG_INFO, "MyApp", ">>> Waiting for signaling to be connected...\n");

    wsFuture.get();
}

int revSendMessage(std::string _revTargetId, std::string revMessage) {
    int revMsgStatus = 0;

    std::shared_ptr<rtc::DataChannel> dc;

    if (auto jt = revDataChannelMap.find(revTargetId); jt != revDataChannelMap.end()) {
        dc = jt->second;
    } else {
        dc = revInitDataChannel(localId, revTargetId);
    }

    if (dc && dc->isOpen()) {
        dc->send(revMessage);

        revMsgStatus = 1;
    } else if (dc->isClosed()) {
        dc = revInitDataChannel(localId, revTargetId);

        dc->onOpen([wdc = make_weak_ptr(dc), revMessage]() {
            if (auto dc = wdc.lock())
                dc->send(revMessage);
        });
    }

    return revMsgStatus;
}

std::shared_ptr<rtc::DataChannel> revInitDataChannel(std::string localId, std::string revTargetId) try {
    if (revTargetId == localId) {
        return NULL;
    }

    shared_ptr<rtc::PeerConnection> pc;

    if (auto jt = revPeerConnectionMap.find(revTargetId); jt != revPeerConnectionMap.end()) {
        pc = jt->second;
    } else {
        pc = createPeerConnection(config, ws, revTargetId);
    }

    const std::string label = revTargetId;

    std::shared_ptr<rtc::DataChannel> dc;

    if (auto jt = revDataChannelMap.find(revTargetId); jt != revDataChannelMap.end()) {
        dc = jt->second;

        return dc;
    } else {
        dc = pc->createDataChannel(label);
    }

    dc->onOpen([revTargetId, wdc = make_weak_ptr(dc), localId, label]() {
        if (auto dc = wdc.lock())
            dc->send("Hello from " + localId);
    });

    dc->onClosed([revTargetId]() { __android_log_print(ANDROID_LOG_WARN, "MyApp", ">>> DataChannel from id : %s closed\n", revTargetId.c_str()); });

    dc->onMessage([revTargetId, wdc = make_weak_ptr(dc)](auto data) {
        // data holds either std::string or rtc::binary
        if (std::holds_alternative<std::string>(data))
            __android_log_print(ANDROID_LOG_WARN, "MyApp", ">>> Message from id : %s received : %s", revTargetId.c_str(), std::get<std::string>(data).c_str());
        else
            __android_log_print(ANDROID_LOG_INFO, "MyApp", ">>> Binary message from id : %s received, size = %ld\n", revTargetId.c_str(), std::get<rtc::binary>(data).size());
    });

    revDataChannelMap.emplace(revTargetId, dc);

    return dc;

} catch (const std::exception &e) {
    revDataChannelMap.clear();
    revPeerConnectionMap.clear();
    return NULL;
}

// Create and setup a PeerConnection
std::shared_ptr<rtc::PeerConnection> createPeerConnection(const rtc::Configuration &config, std::weak_ptr<rtc::WebSocket> wws, std::string revTargetId) {
    std::shared_ptr<rtc::PeerConnection> pc = std::make_shared<rtc::PeerConnection>(config);

    pc->onStateChange([](rtc::PeerConnection::State state) { __android_log_print(ANDROID_LOG_INFO, "MyApp", ">>> State: %d\n", state); });

    pc->onGatheringStateChange([](rtc::PeerConnection::GatheringState state) {
        __android_log_print(ANDROID_LOG_INFO, "MyApp", ">>> Gathering State: %d\n", state);
    });

    pc->onLocalDescription([wws, revTargetId](rtc::Description description) {
        json message = {{"id",          revTargetId},
                        {"type",        description.typeString()},
                        {"description", std::string(description)}};

        ws->send(message.dump());
    });

    pc->onLocalCandidate([wws, revTargetId](rtc::Candidate candidate) {
        json message = {{"id",        revTargetId},
                        {"type",      "candidate"},
                        {"candidate", std::string(candidate)},
                        {"mid",       candidate.mid()}};

        ws->send(message.dump());
    });

    pc->onDataChannel([revTargetId](std::shared_ptr<rtc::DataChannel> dc) {
        dc->onOpen([wdc = make_weak_ptr(dc), &dc]() {
            __android_log_print(ANDROID_LOG_INFO, "MyApp", ">>> D. Channel Open <<<\n");

            dc->send("Hello from " + localId);
        });

        dc->onClosed([revTargetId]() { __android_log_print(ANDROID_LOG_INFO, "MyApp", "DataChannel from %s closed\n", revTargetId.c_str()); });

        dc->onMessage([revTargetId](auto data) {
            // data holds either std::string or rtc::binary
            if (std::holds_alternative<std::string>(data))
                __android_log_print(ANDROID_LOG_WARN, "MyApp", ">>> Message from  id : %s received : %s\n", revTargetId.c_str(), std::get<std::string>(data).c_str());
            else
                __android_log_print(ANDROID_LOG_WARN, "MyApp", ">>> Binary message from id : %s received size = %ld\n", revTargetId.c_str(), std::get<rtc::binary>(data).size());
        });

        revDataChannelMap.emplace(revTargetId, dc);
    });

    revPeerConnectionMap.emplace(revTargetId, pc);

    return pc;
};

// Helper function to generate a random ID
std::string randomId(size_t length) {
    static const std::string characters(
            "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz");
    std::string id(length, '0');
    std::default_random_engine rng(std::random_device{}());
    std::uniform_int_distribution<int> dist(0, int(characters.size() - 1));
    std::generate(id.begin(), id.end(), [&]() { return characters.at(dist(rng)); });
    return id;
}
