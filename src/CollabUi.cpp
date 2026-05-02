#include "CollabUI.hpp"

// ══════════════════════════════════════════════════════════════════════════════
// BuilderSyncPopup
// ══════════════════════════════════════════════════════════════════════════════

BuilderSyncPopup* BuilderSyncPopup::create() {
    auto* p = new BuilderSyncPopup;
    if (p->initAnchored(360.f, 280.f)) { p->autorelease(); return p; }
    delete p; return nullptr;
}

bool BuilderSyncPopup::setup() {
    this->setTitle("BuilderSync");

    auto* s = Session::get();

    // ── Connection row ────────────────────────────────────────────────────
    auto* connMenu = CCMenu::create();
    connMenu->setLayout(RowLayout::create()->setGap(8.f));

    auto* connectBtn = CCMenuItemSpriteExtra::create(
        ButtonSprite::create(
            s->isConnected() ? "Reconnect" : "Connect",
            "bigFont.fnt", "GJ_button_02.png", 0.55f),
        this, menu_selector(BuilderSyncPopup::onConnect));
    connMenu->addChild(connectBtn);

    if (s->isConnected()) {
        auto* badge = CCLabelBMFont::create("● Online", "chatFont.fnt");
        badge->setColor({80, 220, 80});
        badge->setScale(0.55f);
        connMenu->addChild(badge);
    } else {
        auto* badge = CCLabelBMFont::create("● Offline", "chatFont.fnt");
        badge->setColor({220, 80, 80});
        badge->setScale(0.55f);
        connMenu->addChild(badge);
    }

    connMenu->setContentWidth(320.f);
    connMenu->updateLayout();
    this->m_mainLayer->addChildAtPosition(connMenu, Anchor::Center, {0.f, 80.f});

    // ── Code input ────────────────────────────────────────────────────────
    auto* inputLabel = CCLabelBMFont::create("Room Code:", "bigFont.fnt");
    inputLabel->setScale(0.45f);
    this->m_mainLayer->addChildAtPosition(inputLabel, Anchor::Center, {-80.f, 30.f});

    m_codeInput = TextInput::create(180.f, "e.g. ABC123", "chatFont.fnt");
    if (!s->getRoomCode().empty()) m_codeInput->setString(s->getRoomCode());
    this->m_mainLayer->addChildAtPosition(m_codeInput, Anchor::Center, {50.f, 30.f});

    // ── Current room display ──────────────────────────────────────────────
    if (!s->getRoomCode().empty()) {
        auto* codeLabel = CCLabelBMFont::create(
            fmt::format("Active room: {}{}", s->getRoomCode(),
                s->isHost() ? " (host)" : " (guest)").c_str(),
            "chatFont.fnt");
        codeLabel->setColor({200, 255, 200});
        codeLabel->setScale(0.55f);
        this->m_mainLayer->addChildAtPosition(codeLabel, Anchor::Center, {0.f, -10.f});
    }

    // ── Action buttons ────────────────────────────────────────────────────
    auto* btnMenu = CCMenu::create();
    btnMenu->setLayout(RowLayout::create()->setGap(12.f));

    auto* hostBtn = CCMenuItemSpriteExtra::create(
        ButtonSprite::create("Host", "bigFont.fnt", "GJ_button_01.png", 0.7f),
        this, menu_selector(BuilderSyncPopup::onHost));
    btnMenu->addChild(hostBtn);

    auto* joinBtn = CCMenuItemSpriteExtra::create(
        ButtonSprite::create("Join", "bigFont.fnt", "GJ_button_02.png", 0.7f),
        this, menu_selector(BuilderSyncPopup::onJoin));
    btnMenu->addChild(joinBtn);

    if (!s->getRoomCode().empty()) {
        auto* leaveBtn = CCMenuItemSpriteExtra::create(
            ButtonSprite::create("Leave", "bigFont.fnt", "GJ_button_06.png", 0.7f),
            this, menu_selector(BuilderSyncPopup::onLeave));
        btnMenu->addChild(leaveBtn);
    }

    btnMenu->setContentWidth(320.f);
    btnMenu->updateLayout();
    this->m_mainLayer->addChildAtPosition(btnMenu, Anchor::Center, {0.f, -60.f});

    // ── Server URL hint ───────────────────────────────────────────────────
    auto* hint = CCLabelBMFont::create(
        fmt::format("Server: {}", Mod::get()->getSettingValue<std::string>("server-url")).c_str(),
        "chatFont.fnt");
    hint->setColor({150, 150, 150});
    hint->setScale(0.38f);
    this->m_mainLayer->addChildAtPosition(hint, Anchor::Center, {0.f, -105.f});

    return true;
}

void BuilderSyncPopup::onConnect(CCObject*) {
    std::string url = Mod::get()->getSettingValue<std::string>("server-url");
    Session::get()->connect(url);
    this->onClose(nullptr);
    BuilderSyncPopup::create()->show();
}

void BuilderSyncPopup::onHost(CCObject*) {
    auto* s = Session::get();
    if (!s->isConnected()) {
        FLAlertLayer::create("Not Connected",
            "Press <cr>Connect</c> first.", "OK")->show();
        return;
    }
    auto* el    = LevelEditorLayer::get();
    auto* level = el ? el->m_level : nullptr;
    std::string data = level ? std::string(level->m_levelString) : "";
    std::string id   = level ? std::to_string(level->m_levelID.value()) : "0";
    s->hostRoom(id, data);
    this->onClose(nullptr);
    Notification::create("Session started! Share the room code.", NotificationIcon::Success)->show();
}

void BuilderSyncPopup::onJoin(CCObject*) {
    auto* s = Session::get();
    if (!s->isConnected()) {
        FLAlertLayer::create("Not Connected",
            "Press <cr>Connect</c> first.", "OK")->show();
        return;
    }
    std::string code = m_codeInput->getString();
    if (code.empty()) {
        FLAlertLayer::create("No Code", "Enter a room code.", "OK")->show();
        return;
    }
    s->joinRoom(code);
    this->onClose(nullptr);
}

void BuilderSyncPopup::onLeave(CCObject*) {
    Session::get()->leaveRoom();
    this->onClose(nullptr);
    Notification::create("Left the session.", NotificationIcon::Info)->show();
}

// ══════════════════════════════════════════════════════════════════════════════
// BuilderSyncUsersPopup
// ══════════════════════════════════════════════════════════════════════════════

BuilderSyncUsersPopup* BuilderSyncUsersPopup::create() {
    auto* p = new BuilderSyncUsersPopup;
    if (p->initAnchored(280.f, 340.f)) { p->autorelease(); return p; }
    delete p; return nullptr;
}

bool BuilderSyncUsersPopup::setup() {
    this->setTitle("Session Users");
    buildList();
    return true;
}

void BuilderSyncUsersPopup::buildList() {
    auto const& users = Session::get()->getUsers();

    if (users.empty()) {
        auto* empty = CCLabelBMFont::create("No active session.", "chatFont.fnt");
        empty->setScale(0.6f);
        this->m_mainLayer->addChildAtPosition(empty, Anchor::Center);
        return;
    }

    auto* col = CCMenu::create();
    col->setLayout(ColumnLayout::create()
        ->setGap(6.f)
        ->setAxisReverse(true)
        ->setAxisAlignment(AxisAlignment::End));
    col->setContentSize({240.f, 260.f});

    for (auto const& u : users) {
        auto* row = CCMenu::create();
        row->setLayout(RowLayout::create()
            ->setGap(8.f)
            ->setAxisAlignment(AxisAlignment::Start));
        row->setContentWidth(230.f);

        // Colour dot
        auto* dot = CCLayerColor::create(
            {u.color.r, u.color.g, u.color.b, 255}, 10.f, 10.f);
        row->addChild(dot);

        bool isMe = u.id == Session::get()->getLocalId();
        std::string label = isMe ? u.name + " (you)" : u.name;
        if (Session::get()->isHost() && isMe) label += " ★";

        auto* nameLabel = CCLabelBMFont::create(label.c_str(), "chatFont.fnt");
        nameLabel->setScale(0.6f);
        row->addChild(nameLabel);

        row->updateLayout();
        col->addChild(row);
    }

    col->updateLayout();
    this->m_mainLayer->addChildAtPosition(col, Anchor::Center, {0.f, -10.f});
}