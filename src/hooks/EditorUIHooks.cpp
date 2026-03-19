#include <Geode/Geode.hpp>
#include <Geode/modify/EditorUI.hpp>
#include "../objects/ScriptableObjectData.hpp"
#include "../ui/CodeEditorPopup.hpp"

using namespace geode::prelude;

class $modify(ScriptEditorUI, EditorUI) {
    bool init(LevelEditorLayer* layer) {
        if (!EditorUI::init(layer)) return false;

        // Load scripts for this level
        if (layer && layer->m_level) {
            scriptable::LevelScriptStore::loadForLevel(layer->m_level->m_levelID);
            geode::log::info("Loaded scripts for level {}", layer->m_level->m_levelID);
        }

        // Create our own floating menu for the script button
        auto winSize = CCDirector::get()->getWinSize();

        auto menu = CCMenu::create();
        menu->setPosition({0, 0});
        menu->setZOrder(100);
        this->addChild(menu);

        auto spr = CCSprite::createWithSpriteFrameName("GJ_hammerIcon_001.png");
        spr->setScale(0.65f);

        auto btn = CCMenuItemSpriteExtra::create(
            spr, this,
            menu_selector(ScriptEditorUI::onOpenScriptManager)
        );
        btn->setID("script-manager-btn"_spr);
        // Position in top-right corner of the editor
        btn->setPosition({winSize.width - 30, winSize.height - 30});
        menu->addChild(btn);

        geode::log::info("Script manager button added to editor");

        return true;
    }

    void onOpenScriptManager(CCObject*) {
        geode::log::info("Opening script manager");
        auto popup = scriptable::ScriptListPopup::create();
        if (popup) {
            popup->show();
        } else {
            geode::log::error("Failed to create ScriptListPopup");
        }
    }
};
