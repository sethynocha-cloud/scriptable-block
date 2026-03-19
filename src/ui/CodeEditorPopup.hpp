#pragma once
#include <Geode/Geode.hpp>
#include <Geode/ui/Popup.hpp>
#include <Geode/ui/TextInput.hpp>
#include <Geode/ui/ScrollLayer.hpp>
#include "../objects/ScriptableObjectData.hpp"

using namespace geode::prelude;

namespace scriptable {

class ScriptListPopup;

class CodeEditorPopup : public geode::Popup {
protected:
    ScriptEntry* m_entry = nullptr;
    TextInput* m_codeInput = nullptr;
    CCLabelBMFont* m_errorLabel = nullptr;
    CCLabelBMFont* m_triggerModeLabel = nullptr;
    CCMenuItemSpriteExtra* m_triggerModeBtn = nullptr;

    bool initWithEntry(ScriptEntry* entry);
    void onSave(CCObject*);
    void onValidate(CCObject*);
    void onCycleTriggerMode(CCObject*);
    void updateTriggerModeLabel();

public:
    static CodeEditorPopup* create(ScriptEntry* entry);
};

class ScriptListPopup : public geode::Popup {
protected:
    ScrollLayer* m_scrollLayer = nullptr;

    bool initPopup();
    void refreshList();
    void onAddScript(CCObject*);
    void onEditScript(CCObject*);
    void onDeleteScript(CCObject*);
    void onToggleScript(CCObject*);

public:
    static ScriptListPopup* create();
};

} // namespace scriptable
