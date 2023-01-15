//
// Created by rev on 11/13/22.
//

#ifndef OWKI_REV_CLIENT_INIT_H
#define OWKI_REV_CLIENT_INIT_H

#include <memory>

using std::shared_ptr;

std::string revGetTestStr(std::string revKey);

std::string revSetTestStr(std::string revKey, std::string revVal);

shared_ptr<rtc::WebSocket> revInitWS(shared_ptr<rtc::WebSocket> revWS, std::string url, std::string revLocalId, void (*rev_call_back_func)(char *_revRetStr));

/** START ON MESSAGE **/

void revHandleLogin(std::string revMessage);

/** END ON MESSAGE **/

std::shared_ptr<rtc::PeerConnection> createPeerConnection(std::string revTargetId);

std::shared_ptr<rtc::DataChannel> revInitDataChannel(std::string localId, std::string revTargetId);

int revSendMessage(std::string _revTargetId, std::string revMessage);

int revWebRTCLogIn(shared_ptr<rtc::WebSocket> revWS, std::string _revTargetId, std::string revMessage);

#endif //OWKI_REV_CLIENT_INIT_H
