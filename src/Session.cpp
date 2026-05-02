#include "Session.hpp"

Session* Session::get() {
    static Session instance;
    return &instance;
}

bool Session::connect(std::string const& serverUrl) {
    m_serverUrl = serverUrl;
    m_ws.setUrl(serverUrl);
    m_ws.setOnMessageCallback([this](ix::WebSocketMessagePtr const& msg) {
        onMessage(msg);
    });
    m_ws.start();
    return true;
}

void Session::disconnect() {
    leaveRoom();
    m_ws.stop();
    m_connected = false;
}

bool Session::isConnected() const {
    return m_connected.load();
}

void Session::hostRoom(std::string const& levelId, std::string const& levelData) {
    m_isHost = true;
    sendOp({
        {"op",         "host"},
        {"level_id",   levelId},
        {"level_data", levelData},
        {"name",       Mod::get()->getSettingValue<std::string>("username")}
    });
}

void Session::joinRoom(std::string const& code) {
    m_isHost  = false;
    m_roomCode = code;
    sendOp({
        {"op",   "join"},
        {"code", code},
        {"name", Mod::get()->getSettingValue<std::string>("username")}
    });
}

void Session::leaveRoom() {
    if (!m_roomCode.empty()) {
        sendOp({{"op", "leave"}, {"code", m_roomCode}});
    }
    m_roomCode.clear();
    m_isHost = false;
    std::lock_guard<std::mutex> lock(m_usersMutex);
    m_users.clear();
}

void Session::sendOp(json const& payload) {
    json out = payload;
    if (!out.contains("code") && !m_roomCode.empty()) {
        out["code"] = m_roomCode;
    }
    std::string msg = out.dump();

    if (!m_connected) {
        // Queue for when connection opens
        std::lock_guard<std::mutex> lock(m_queueMutex);
        m_sendQueue.push(msg);
        return;
    }
    m_ws.send(msg);
}

void Session::flushQueue() {
    std::lock_guard<std::mutex> lock(m_queueMutex);
    while (!m_sendQueue.empty()) {
        m_ws.send(m_sendQueue.front());
        m_sendQueue.pop();
    }
}

void Session::sendCursor(float x, float y) {
    if (!Mod::get()->getSettingValue<bool>("cursor-sync")) return;
    sendOp({{"op", "cursor"}, {"x", x}, {"y", y}});
}

void Session::onMessage(ix::WebSocketMessagePtr const& msg) {
    using MsgType = ix::WebSocketMessageType;

    if (msg->type == MsgType::Open) {
        m_connected = true;
        log::info("[BuilderSync] Connected to {}", m_serverUrl);
        flushQueue();
        return;
    }
    if (msg->type == MsgType::Close || msg->type == MsgType::Error) {
        m_connected = false;
        log::warn("[BuilderSync] Disconnected: {}", msg->errorInfo.reason);
        return;
    }
    if (msg->type != MsgType::Message) return;

    json pkt;
    try { pkt = json::parse(msg->str); }
    catch (...) { log::warn("[BuilderSync] Bad JSON"); return; }

    std::string op = pkt.value("op", "");

    auto dispatch = [](std::function<void()> fn) {
        Loader::get()->queueInMainThread(std::move(fn));
    };

    if (op == "place") {
        dispatch([this, pkt]{ if (onPlace)  onPlace(pkt);  });
    } else if (op == "delete") {
        dispatch([this, pkt]{ if (onDelete) onDelete(pkt); });
    } else if (op == "move") {
        dispatch([this, pkt]{ if (onMove)   onMove(pkt);   });
    } else if (op == "rotate") {
        dispatch([this, pkt]{ if (onRotate) onRotate(pkt); });
    } else if (op == "cursor") {
        dispatch([this, pkt]{ if (onCursor) onCursor(pkt); });
    } else if (op == "level_sync") {
        std::string data = pkt.value("level_data", "");
        dispatch([this, data]{ if (onLevelSync) onLevelSync(data); });
    } else if (op == "hosted") {
        m_roomCode = pkt.value("code", "");
        m_localId  = pkt.value("user_id", "");
        log::info("[BuilderSync] Hosting room: {}", m_roomCode);
    } else if (op == "joined") {
        std::string id   = pkt.value("user_id", "");
        std::string name = pkt.value("name", "?");
        if (pkt.value("self", false)) {
            m_localId  = id;
            m_roomCode = pkt.value("code", m_roomCode);
        }
        ConnectedUser u;
        u.id    = id;
        u.name  = name;
        u.color = USER_COLORS[m_colorIndex++ % USER_COLORS.size()];
        {
            std::lock_guard<std::mutex> lock(m_usersMutex);
            m_users.push_back(u);
        }
        dispatch([this, id, name]{ if (onUserJoined) onUserJoined(id, name); });
    } else if (op == "left") {
        std::string id = pkt.value("user_id", "");
        {
            std::lock_guard<std::mutex> lock(m_usersMutex);
            m_users.erase(
                std::remove_if(m_users.begin(), m_users.end(),
                    [&](auto const& u){ return u.id == id; }),
                m_users.end());
        }
        dispatch([this, id]{ if (onUserLeft) onUserLeft(id); });
    } else if (op == "user_list") {
        std::vector<ConnectedUser> list;
        for (auto const& u : pkt.value("users", json::array())) {
            ConnectedUser cu;
            cu.id    = u.value("user_id", "");
            cu.name  = u.value("name", "?");
            cu.color = USER_COLORS[m_colorIndex++ % USER_COLORS.size()];
            list.push_back(cu);
        }
        {
            std::lock_guard<std::mutex> lock(m_usersMutex);
            m_users = list;
        }
        dispatch([this, list]{ if (onUserListUpdate) onUserListUpdate(list); });
    }
}
