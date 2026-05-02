#pragma once
#include <Geode/Geode.hpp>
#include <IXWebSocket.h>
#include <nlohmann/json.hpp>
#include <functional>
#include <string>
#include <vector>
#include <mutex>
#include <atomic>

using namespace geode::prelude;
using json = nlohmann::json;

// ─────────────────────────────────────────────────────────────────────────────
// ConnectedUser
// ─────────────────────────────────────────────────────────────────────────────
struct ConnectedUser {
    std::string id;
    std::string name;
    ccColor3B   color;
    float       cursorX = 0;
    float       cursorY = 0;
};

// ─────────────────────────────────────────────────────────────────────────────
// Session — singleton managing the WebSocket connection
// ─────────────────────────────────────────────────────────────────────────────
class Session {
public:
    static Session* get();

    // ── Connection ──────────────────────────────────────────────────────────
    bool connect(std::string const& serverUrl);
    void disconnect();
    bool isConnected() const;

    // ── Room ────────────────────────────────────────────────────────────────
    void hostRoom(std::string const& levelId, std::string const& levelData);
    void joinRoom(std::string const& code);
    void leaveRoom();

    std::string const& getRoomCode() const { return m_roomCode; }
    bool isHost() const { return m_isHost; }

    // ── Sending ─────────────────────────────────────────────────────────────
    void sendOp(json const& payload);
    void sendCursor(float x, float y);

    // ── Callbacks (fired on GD main thread) ─────────────────────────────────
    std::function<void(json const&)> onPlace;
    std::function<void(json const&)> onDelete;
    std::function<void(json const&)> onMove;
    std::function<void(json const&)> onRotate;
    std::function<void(json const&)> onCursor;
    std::function<void(std::string const&)> onLevelSync;
    std::function<void(std::vector<ConnectedUser> const&)> onUserListUpdate;
    std::function<void(std::string const&, std::string const&)> onUserJoined;
    std::function<void(std::string const&)> onUserLeft;

    // ── State ────────────────────────────────────────────────────────────────
    std::vector<ConnectedUser> const& getUsers() const { return m_users; }
    std::string const& getLocalId() const { return m_localId; }

private:
    Session() = default;
    void onMessage(ix::WebSocketMessagePtr const& msg);

    ix::WebSocket              m_ws;
    std::string                m_serverUrl;
    std::string                m_roomCode;
    std::string                m_localId;
    bool                       m_isHost = false;
    std::atomic<bool>          m_connected{false};

    std::vector<ConnectedUser> m_users;
    std::mutex                 m_usersMutex;
    int                        m_colorIndex = 0;

    static inline std::vector<ccColor3B> USER_COLORS = {
        {255, 100, 100}, {100, 200, 255}, {100, 255, 150},
        {255, 220,  80}, {200, 100, 255}, {255, 160,  50}
    };
};