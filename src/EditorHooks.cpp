#include "Session.hpp"
#include "CursorOverlay.hpp"
#include "CollabUI.hpp"

#include <Geode/modify/LevelEditorLayer.hpp>
#include <Geode/modify/EditorUI.hpp>
#include <Geode/modify/EditorPauseLayer.hpp>
#include <Geode/loader/NodeIDs.hpp>

using namespace geode::prelude;

// ── Guard that blocks re-broadcasting incoming remote ops ─────────────────────
static bool s_applyingRemote = false;
struct RemoteGuard {
    RemoteGuard()  { s_applyingRemote = true;  }
    ~RemoteGuard() { s_applyingRemote = false; }
};

// ─────────────────────────────────────────────────────────────────────────────
// LevelEditorLayer hooks
// ─────────────────────────────────────────────────────────────────────────────
class $modify(BSEditorLayer, LevelEditorLayer) {

    bool init(GJGameLevel* level, bool p1) {
        if (!LevelEditorLayer::init(level, p1)) return false;

        auto* s = Session::get();

        // ── Incoming: place ───────────────────────────────────────────────
        s->onPlace = [this](json const& pkt) {
            RemoteGuard g;
            std::string data = pkt.value("data", "");
            if (!data.empty()) createObjectFromString(data);
        };

        // ── Incoming: delete ──────────────────────────────────────────────
        s->onDelete = [this](json const& pkt) {
            RemoteGuard g;
            int uid = pkt.value("unique_id", -1);
            if (uid < 0) return;
            for (int i = 0; i < m_objects->count(); ++i) {
                auto* o = static_cast<GameObject*>(m_objects->objectAtIndex(i));
                if (o->m_uniqueID == uid) {
                    removeObject(o, false);
                    break;
                }
            }
        };

        // ── Incoming: move ────────────────────────────────────────────────
        s->onMove = [this](json const& pkt) {
            RemoteGuard g;
            int   uid = pkt.value("unique_id", -1);
            float x   = pkt.value("x", 0.f);
            float y   = pkt.value("y", 0.f);
            for (int i = 0; i < m_objects->count(); ++i) {
                auto* o = static_cast<GameObject*>(m_objects->objectAtIndex(i));
                if (o->m_uniqueID == uid) { o->setPosition({x, y}); break; }
            }
        };

        // ── Incoming: rotate ──────────────────────────────────────────────
        s->onRotate = [this](json const& pkt) {
            RemoteGuard g;
            for (auto const& entry : pkt.value("objects", json::array())) {
                int   uid = entry.value("unique_id", -1);
                float rot = entry.value("rot", 0.f);
                for (int i = 0; i < m_objects->count(); ++i) {
                    auto* o = static_cast<GameObject*>(m_objects->objectAtIndex(i));
                    if (o->m_uniqueID == uid) { o->setRotation(rot); break; }
                }
            }
        };

        // ── Incoming: full level sync (late join) ─────────────────────────
        s->onLevelSync = [this](std::string const& levelData) {
            RemoteGuard g;
            // TODO: full level reload requires re-initialising the editor.
            // For now, log the sync so we know it arrived. A proper impl
            // would call LevelEditorLayer::init() with updated m_level->m_levelString,
            // or use GJGameLevel::setLevelString and reload the scene.
            log::warn("[BuilderSync] Level sync received ({} bytes) — full reload not yet implemented.",
                levelData.size());
        };

        // ── Incoming: cursor ──────────────────────────────────────────────
        s->onCursor = [](json const& pkt) {
            CursorOverlay::get()->updateCursor(
                pkt.value("user_id", ""),
                pkt.value("x", 0.f),
                pkt.value("y", 0.f));
        };

        // ── Incoming: user joined ─────────────────────────────────────────
        s->onUserJoined = [](std::string const& id, std::string const& name) {
            if (Mod::get()->getSettingValue<bool>("show-notifications")) {
                Notification::create(
                    fmt::format("{} joined", name),
                    NotificationIcon::Success)->show();
            }
        };

        // ── Incoming: user left ───────────────────────────────────────────
        s->onUserLeft = [](std::string const& id) {
            CursorOverlay::get()->removeCursor(id);
        };

        CursorOverlay::get()->attachTo(this);
        return true;
    }

    // ── Outgoing: place ───────────────────────────────────────────────────────
    // GD 2.2074 uses createObjectFromString (addObjectFromString in older versions)
    GameObject* createObjectFromString(gd::string str) {
        auto* obj = LevelEditorLayer::createObjectFromString(str);
        if (obj && !s_applyingRemote && Session::get()->isConnected()) {
            Session::get()->sendOp({
                {"op",   "place"},
                {"data", std::string(str)}
            });
        }
        return obj;
    }

    // ── Outgoing: delete ──────────────────────────────────────────────────────
    void removeObject(GameObject* obj, bool unk) {
        if (!s_applyingRemote && Session::get()->isConnected()) {
            Session::get()->sendOp({
                {"op",        "delete"},
                {"unique_id", obj->m_uniqueID}
            });
        }
        LevelEditorLayer::removeObject(obj, unk);
    }

    // ── Outgoing: move ────────────────────────────────────────────────────────
    void moveObject(GameObject* obj, CCPoint delta) {
        LevelEditorLayer::moveObject(obj, delta);
        if (!s_applyingRemote && Session::get()->isConnected()) {
            Session::get()->sendOp({
                {"op",        "move"},
                {"unique_id", obj->m_uniqueID},
                {"x",         obj->getPositionX()},
                {"y",         obj->getPositionY()}
            });
        }
    }

    // ── Outgoing: rotate ──────────────────────────────────────────────────────
    void rotateObjects(CCArray* objs, float angle, CCPoint center) {
        LevelEditorLayer::rotateObjects(objs, angle, center);
        if (!s_applyingRemote && Session::get()->isConnected()) {
            json ids = json::array();
            for (int i = 0; i < objs->count(); ++i) {
                auto* o = static_cast<GameObject*>(objs->objectAtIndex(i));
                ids.push_back({{"unique_id", o->m_uniqueID}, {"rot", o->getRotation()}});
            }
            Session::get()->sendOp({{"op", "rotate"}, {"objects", ids}});
        }
    }
};

// ─────────────────────────────────────────────────────────────────────────────
// EditorUI — inject BuilderSync button using node IDs
// ─────────────────────────────────────────────────────────────────────────────
class $modify(BSEditorUI, EditorUI) {
    bool init(LevelEditorLayer* lel) {
        if (!EditorUI::init(lel)) return false;

        // node-ids: assign stable IDs to all children of EditorUI
        if (auto* ids = NodeIDs::get()) ids->provide(this);

        // Use the undo-menu row — stable ID provided by node-ids
        auto* menu = this->getChildByID("undo-menu");
        if (!menu) return true;

        auto* btn = CCMenuItemSpriteExtra::create(
            ButtonSprite::create("Sync", "bigFont.fnt", "GJ_button_01.png", 0.5f),
            this, menu_selector(BSEditorUI::onSyncButton));
        btn->setID("buildersync-button");
        menu->addChild(btn);
        static_cast<CCMenu*>(menu)->updateLayout();

        return true;
    }

    void onSyncButton(CCObject*) {
        BuilderSyncPopup::create()->show();
    }
};

// ─────────────────────────────────────────────────────────────────────────────
// EditorPauseLayer — add Users button using node IDs
// ─────────────────────────────────────────────────────────────────────────────
class $modify(BSPauseLayer, EditorPauseLayer) {
    bool init(LevelEditorLayer* el) {
        if (!EditorPauseLayer::init(el)) return false;

        // node-ids: assign stable IDs to all children of EditorPauseLayer
        if (auto* ids = NodeIDs::get()) ids->provide(this);

        auto* menu = this->getChildByID("settings-menu");
        if (!menu) return true;

        auto* btn = CCMenuItemSpriteExtra::create(
            ButtonSprite::create("Users", "bigFont.fnt", "GJ_button_05.png", 0.5f),
            this, menu_selector(BSPauseLayer::onUsersButton));
        btn->setID("buildersync-users-button");
        menu->addChild(btn);
        static_cast<CCMenu*>(menu)->updateLayout();

        return true;
    }

    void onUsersButton(CCObject*) {
        BuilderSyncUsersPopup::create()->show();
    }
};
