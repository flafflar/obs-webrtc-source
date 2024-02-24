/*
OBS WebRTC Source
Copyright (C) 2024 Achilleas Michailidis <achmichail@gmail.com>

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License along
with this program. If not, see <https://www.gnu.org/licenses/>
*/
#include "webrtc.h"

#include <string>
#include <rtc/rtc.hpp>

#include <obs/obs-module.h>
#include "plugin-support.h"

class WebRTCConnection {
    std::shared_ptr<rtc::WebSocketServer> wsServer;
    std::shared_ptr<rtc::WebSocket> activeSocket;
    std::shared_ptr<rtc::PeerConnection> peerConnection;
    std::shared_ptr<rtc::Track> videoTrack;
    std::shared_ptr<rtc::RtcpReceivingSession> session;
    bool clientReady = false;


public:
    WebRTCConnection(uint16_t port);

    webrtc_video_callback_t videoCallback;
    void *videoCallbackData;
private:
    /**
     * Tries to send the local session description to the client, if possible.
     *
     * @return Whether the session description was sent or not.
     */
    bool sendLocalDescription();

    void onSocket(std::shared_ptr<rtc::WebSocket> socket);
    void onMessage(rtc::message_variant data);
};

WebRTCConnection::WebRTCConnection(uint16_t port) {
    obs_log(LOG_INFO, "WebRTCConnection constructor");
    rtc::WebSocketServerConfiguration wsServerConf = {
        .port = port,
        .enableTls = false,
    };

    this->wsServer = std::make_shared<rtc::WebSocketServer>(wsServerConf);
    this->wsServer->onClient([this](std::shared_ptr<rtc::WebSocket> socket) {
        this->onSocket(socket);
    });

    this->peerConnection = std::make_shared<rtc::PeerConnection>();
    this->peerConnection->onGatheringStateChange(
        [this](rtc::PeerConnection::GatheringState state) {
            if (state == rtc::PeerConnection::GatheringState::Complete) {
                this->sendLocalDescription();
            }
        }
    );

    rtc::Description::Video media (
        "video",
        rtc::Description::Direction::RecvOnly
    );

    media.addH264Codec(96);
    media.setBitrate(9000);

    this->videoTrack = this->peerConnection->addTrack(media);

    this->session = std::make_shared<rtc::RtcpReceivingSession>();
    this->videoTrack->setMediaHandler(this->session);

    this->videoTrack->onMessage(
        [this](rtc::binary message) {
            this->videoCallback(
                (uint8_t *) message.data(),
                message.size(),
                this->videoCallbackData
            );
        },
        nullptr
    );

    this->peerConnection->setLocalDescription(rtc::Description::Type::Offer);
}

bool WebRTCConnection::sendLocalDescription() {
    auto description = this->peerConnection->localDescription();
    if (this->clientReady && description.has_value()) {
        this->activeSocket->send(description.value());
        return true;
    } else {
        return false;
    }
}

void WebRTCConnection::onSocket(std::shared_ptr<rtc::WebSocket> socket) {
    if (this->activeSocket == nullptr) {
        this->activeSocket = socket;
        socket->onMessage([this](rtc::message_variant data) {
            this->onMessage(data);
        });
    } else {
        socket->close();
    }
}

void WebRTCConnection::onMessage(rtc::message_variant data) {
    if (std::holds_alternative<std::string>(data)) {
        std::string strData = std::get<std::string>(data);
        obs_log(LOG_INFO, "%s", strData.c_str());
        if (strData == "ready") {
            this->clientReady = true;
            this->sendLocalDescription();
        } else {
            rtc::Description answer (strData, "answer");
            this->peerConnection->setRemoteDescription(answer);
        }
    }
}

static void webrtc_log_callback(rtc::LogLevel level, std::string message) {
    switch (level) {
        case rtc::LogLevel::Fatal:
        case rtc::LogLevel::Error:
            obs_log(LOG_ERROR, "%s", message.c_str());
            break;

        case rtc::LogLevel::Warning:
            obs_log(LOG_WARNING, "%s", message.c_str());
            break;

        case rtc::LogLevel::Info:
            obs_log(LOG_INFO, "%s", message.c_str());
            break;

        case rtc::LogLevel::Debug:
        case rtc::LogLevel::Verbose:
            obs_log(LOG_DEBUG, "%s", message.c_str());
            break;
    }
}

struct webrtc_connection *webrtc_connection_create(
    webrtc_connection_config *config
) {
    rtc::InitLogger(rtc::LogLevel::Debug, webrtc_log_callback);

    WebRTCConnection *connection = new WebRTCConnection(config->port);

    connection->videoCallback = config->video_callback;
    connection->videoCallbackData = config->video_callback_data;

    return (struct webrtc_connection *) connection;
}

void webrtc_connection_delete(struct webrtc_connection **pconn) {
    WebRTCConnection *conn = (WebRTCConnection*) *pconn;
    delete conn;
    *pconn = nullptr;
}