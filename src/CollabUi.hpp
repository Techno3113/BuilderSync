#pragma once
#include <Geode/Geode.hpp>
#include "Session.hpp"

using namespace geode::prelude;

// ── Host / Join popup ────────────────────────────────────────────────────────
class BuilderSyncPopup : public geode::Popup<> {
public:
    static BuilderSyncPopup* create();
protected:
    bool setup() override;
private:
    TextInput* m_codeInput = nullptr;
    void onConnect(CCObject*);
    void onHost(CCObject*);
    void onJoin(CCObject*);
    void onLeave(CCObject*);
};

// ── User list popup ──────────────────────────────────────────────────────────
class BuilderSyncUsersPopup : public geode::Popup<> {
public:
    static BuilderSyncUsersPopup* create();
protected:
    bool setup() override;
private:
    void buildList();
};
