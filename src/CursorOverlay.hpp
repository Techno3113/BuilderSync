#pragma once
#include <Geode/Geode.hpp>
#include <string>
#include <unordered_map>

using namespace geode::prelude;

class CursorOverlay {
public:
    static CursorOverlay* get();

    void attachTo(CCNode* editorLayer);
    void updateCursor(std::string const& userId, float x, float y);
    void removeCursor(std::string const& userId);

private:
    struct CursorNode {
        CCNode*        root  = nullptr;
        CCLabelBMFont* label = nullptr;
    };

    CCNode* m_parent = nullptr;
    std::unordered_map<std::string, CursorNode> m_cursors;

    CursorNode& getOrCreate(std::string const& userId);
};