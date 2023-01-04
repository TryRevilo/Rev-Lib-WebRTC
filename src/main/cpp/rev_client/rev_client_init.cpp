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

void revHandleLogin(std::string revMessage);

std::shared_ptr<rtc::DataChannel> revInitDataChannel(std::string localId, std::string revTargetId);

std::shared_ptr<rtc::PeerConnection> createPeerConnection(std::string revTargetId);

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

std::string revGetData(json revData, std::string revKey) {
    auto it = revData.find(strdup(revKey.c_str()));

    if (it == revData.end())
        return nullptr;

    std::string revVal = it->get<std::string>();

    return revVal;
}

shared_ptr<rtc::WebSocket> revInitWS(shared_ptr<rtc::WebSocket> revWS, std::string url, std::string revLocalId, void (*rev_call_back_func)(char *_revRetStr)) {
    localId = revLocalId;

    revWS->onOpen([&rev_call_back_func, &revWS]() {
        char *revRetStr = ">>> WS OPEN <<<";
        rev_call_back_func(revRetStr);
    });

    revWS->onError([&rev_call_back_func](std::string s) {
        rev_call_back_func(s.data());

        revDataChannelMap.clear();
        revPeerConnectionMap.clear();
    });

    revWS->onClosed([&rev_call_back_func]() {
        char *revData = "data: \'onClosed\'}";
        rev_call_back_func(revData);

        revDataChannelMap.clear();
        revPeerConnectionMap.clear();
    });

    revWS->onMessage([wws = make_weak_ptr(ws), revLocalId, &rev_call_back_func](auto data) {
        // data holds either std::string or rtc::binary
        if (!std::holds_alternative<std::string>(data))
            return;

        std::string revDataStr = std::get<std::string>(data);

        __android_log_print(ANDROID_LOG_WARN, "MyApp", ">>> WS -> MESSAGE <<< %s\n", revDataStr.c_str());

        json message = json::parse(revDataStr);

        auto it = message.find("id");
        if (it == message.end())
            return;

        std::string id = it->get<std::string>();

        it = message.find("type");

        if (it == message.end())
            return;

        auto type = it->get<std::string>();

        if (type != "login")
            rev_call_back_func(revDataStr.data());

        shared_ptr<rtc::PeerConnection> pc;

        if (auto jt = revPeerConnectionMap.find(id); jt != revPeerConnectionMap.end()) {
            pc = jt->second;
        } else if (type == "login") {
            revHandleLogin(message.dump());
        } else if (type == "offer") {
            pc = createPeerConnection(id);
        } else if (type == "revConnEntity") {
            revTargetId = (message.find("revTargetId"))->get<std::string>();
            std::string revLocal = (message.find("id"))->get<std::string>();

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

    std::string revURL = url + ":" + std::to_string(4000);

    revWS->open(revURL);

    return revWS;
}

int revWebRTCLogIn(shared_ptr<rtc::WebSocket> revWS, std::string _revTargetId, std::string revMessage) {
    __android_log_print(ANDROID_LOG_INFO, "MyApp", ">>> revWebRTCLogIn : %s\n", revMessage.c_str());

    int revSendStatus = revWS->send(revMessage);

    return revSendStatus;
}

void revHandleLogin(std::string revMessage) {
    __android_log_print(ANDROID_LOG_INFO, "MyApp", ">>> revHandleLogin - Logged In . . . %s\n", revMessage.c_str());
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

std::shared_ptr<rtc::DataChannel> revInitDataChannel(std::string localId, std::string revTargetId) {
    try {
        if (revTargetId == localId) {
            // return NULL;
        }

        shared_ptr<rtc::PeerConnection> pc;

        if (auto jt = revPeerConnectionMap.find(revTargetId); jt != revPeerConnectionMap.end()) {
            pc = jt->second;
        } else {
            pc = createPeerConnection(revTargetId);
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
    }
    catch (const std::exception &e) {
        revDataChannelMap.clear();
        revPeerConnectionMap.clear();
    }

    return NULL;
}

// Create and setup a PeerConnection
std::shared_ptr<rtc::PeerConnection> createPeerConnection(std::string revTargetId) {
    std::shared_ptr<rtc::PeerConnection> pc = std::make_shared<rtc::PeerConnection>(config);

    pc->onStateChange([](rtc::PeerConnection::State state) { __android_log_print(ANDROID_LOG_INFO, "MyApp", ">>> State: %d\n", state); });

    pc->onGatheringStateChange([](rtc::PeerConnection::GatheringState state) { __android_log_print(ANDROID_LOG_INFO, "MyApp", ">>> Gathering State: %d\n", state); });

    pc->onLocalDescription([revTargetId](rtc::Description description) {
        json message = {{"id",          revTargetId},
                        {"type",        description.typeString()},
                        {"description", std::string(description)}};

        ws->send(message.dump());
    });

    pc->onLocalCandidate([revTargetId](rtc::Candidate candidate) {
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