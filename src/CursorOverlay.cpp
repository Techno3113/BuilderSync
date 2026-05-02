#include "CursorOverlay.hpp"
#include "Session.hpp"

CursorOverlay* CursorOverlay::get() {
    static CursorOverlay instance;
    return &instance;
}

void CursorOverlay::attachTo(CCNode* editorLayer) {
    m_parent = editorLayer;
    m_cursors.clear();
}

CursorOverlay::CursorNode& CursorOverlay::getOrCreate(std::string const& userId) {
    auto it = m_cursors.find(userId);
    if (it != m_cursors.end()) return it->second;

    ccColor3B color = {255, 255, 255};
    std::string displayName = userId.substr(0, 8);
    for (auto const& u : Session::get()->getUsers()) {
        if (u.id == userId) { color = u.color; displayName = u.name; break; }
    }

    CursorNode cn;
    cn.root = CCNode::create();
    cn.root->setZOrder(100);

    // Crosshair: horizontal and vertical bars
    float len = 14.f, thick = 2.f;
    auto* h = CCLayerColor::create({color.r, color.g, color.b, 200}, len, thick);
    h->setPosition({-len / 2.f, -thick / 2.f});
    auto* v = CCLayerColor::create({color.r, color.g, color.b, 200}, thick, len);
    v->setPosition({-thick / 2.f, -len / 2.f});
    cn.root->addChild(h);
    cn.root->addChild(v);

    // Name tag
    cn.label = CCLabelBMFont::create(displayName.c_str(), "chatFont.fnt");
    cn.label->setScale(0.45f);
    cn.label->setColor(color);
    cn.label->setPosition({8.f, 10.f});
    cn.root->addChild(cn.label);

    if (m_parent) m_parent->addChild(cn.root);
    m_cursors[userId] = cn;
    return m_cursors[userId];
}

void CursorOverlay::updateCursor(std::string const& userId, float x, float y) {
    if (!m_parent) return;
    getOrCreate(userId).root->setPosition({x, y});
}

void CursorOverlay::removeCursor(std::string const& userId) {
    auto it = m_cursors.find(userId);
    if (it == m_cursors.end()) return;
    if (it->second.root) it->second.root->removeFromParent();
    m_cursors.erase(it);
}