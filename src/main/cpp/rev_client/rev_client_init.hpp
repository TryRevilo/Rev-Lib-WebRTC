//
// Created by rev on 11/13/22.
//

#ifndef OWKI_REV_CLIENT_INIT_H
#define OWKI_REV_CLIENT_INIT_H

#include <memory>

std::string revGetTestStr(std::string revKey);

std::string revSetTestStr(std::string revKey, std::string revVal);

void revInitWS(std::string url, std::string revLocalId);

std::shared_ptr<rtc::PeerConnection> createPeerConnection(const rtc::Configuration &config, std::weak_ptr<rtc::WebSocket> wws, std::string revTargetId);

std::shared_ptr<rtc::DataChannel> revInitDataChannel(std::string localId, std::string revTargetId);

int revSendMessage(std::string _revTargetId, std::string revMessage);

#endif //OWKI_REV_CLIENT_INIT_H
