#include "CodeEditorPopup.hpp"
#include "../dsl/Lexer.hpp"
#include "../dsl/Parser.hpp"
#include "../dsl/DSLError.hpp"

namespace scriptable {

// ==================== CodeEditorPopup ====================

CodeEditorPopup* CodeEditorPopup::create(ScriptEntry* entry) {
    auto* ret = new CodeEditorPopup();
    if (ret->initWithEntry(entry)) {
        ret->autorelease();
        return ret;
    }
    delete ret;
    return nullptr;
}

bool CodeEditorPopup::initWithEntry(ScriptEntry* entry) {
    if (!Popup::init(420.f, 320.f)) return false;
    m_entry = entry;
    if (!m_entry) return false;

    this->setTitle(m_entry->name.c_str());

    auto winSize = m_mainLayer->getContentSize();

    // --- Trigger mode selector ---
    auto modeMenu = CCMenu::create();
    modeMenu->setPosition({0, 0});
    m_mainLayer->addChild(modeMenu);

    auto modeBg = CCScale9Sprite::create("square02_small.png");
    modeBg->setContentSize({150, 25});
    modeBg->setOpacity(100);

    m_triggerModeBtn = CCMenuItemSpriteExtra::create(
        modeBg,
        this,
        menu_selector(CodeEditorPopup::onCycleTriggerMode)
    );
    m_triggerModeBtn->setPosition({winSize.width / 2, winSize.height - 45});
    modeMenu->addChild(m_triggerModeBtn);

    m_triggerModeLabel = CCLabelBMFont::create("", "bigFont.fnt");
    m_triggerModeLabel->setScale(0.35f);
    m_triggerModeLabel->setPosition(m_triggerModeBtn->getContentSize() / 2);
    m_triggerModeBtn->addChild(m_triggerModeLabel);
    updateTriggerModeLabel();

    // --- Code input area ---
    m_codeInput = TextInput::create(380.f, "Enter script code...", "chatFont.fnt");
    m_codeInput->setPosition({winSize.width / 2, winSize.height / 2 - 10});
    m_codeInput->setFilter("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789 .,;:!?(){}[]<>=+-*/%&|^~_'\"@#$\n\t");
    m_codeInput->setString(m_entry->scriptCode);
    m_codeInput->setMaxCharCount(4096);
    m_mainLayer->addChild(m_codeInput);

    // --- Error label ---
    m_errorLabel = CCLabelBMFont::create("", "chatFont.fnt");
    m_errorLabel->setScale(0.5f);
    m_errorLabel->setColor({255, 80, 80});
    m_errorLabel->setPosition({winSize.width / 2, 55});
    m_errorLabel->setVisible(false);
    m_mainLayer->addChild(m_errorLabel);

    // --- Bottom buttons ---
    auto btnMenu = CCMenu::create();
    btnMenu->setPosition({0, 0});
    m_mainLayer->addChild(btnMenu);

    auto validateSpr = ButtonSprite::create("Validate", "goldFont.fnt", "GJ_button_01.png", 0.8f);
    auto validateBtn = CCMenuItemSpriteExtra::create(
        validateSpr, this, menu_selector(CodeEditorPopup::onValidate)
    );
    validateBtn->setPosition({winSize.width / 2 - 70, 30});
    btnMenu->addChild(validateBtn);

    auto saveSpr = ButtonSprite::create("Save", "goldFont.fnt", "GJ_button_02.png", 0.8f);
    auto saveBtn = CCMenuItemSpriteExtra::create(
        saveSpr, this, menu_selector(CodeEditorPopup::onSave)
    );
    saveBtn->setPosition({winSize.width / 2 + 70, 30});
    btnMenu->addChild(saveBtn);

    return true;
}

void CodeEditorPopup::onSave(CCObject*) {
    std::string code = m_codeInput->getString();

    try {
        Lexer lexer(code);
        auto tokens = lexer.tokenize();
        Parser parser(tokens);
        parser.parse();
    } catch (const DSLError& e) {
        m_errorLabel->setString(e.what());
        m_errorLabel->setColor({255, 80, 80});
        m_errorLabel->setVisible(true);
        return;
    }

    m_entry->scriptCode = code;
    LevelScriptStore::save();
    m_errorLabel->setVisible(false);
    this->onClose(nullptr);
}

void CodeEditorPopup::onValidate(CCObject*) {
    std::string code = m_codeInput->getString();

    try {
        Lexer lexer(code);
        auto tokens = lexer.tokenize();
        Parser parser(tokens);
        parser.parse();

        m_errorLabel->setString("Script is valid!");
        m_errorLabel->setColor({80, 255, 80});
        m_errorLabel->setVisible(true);
    } catch (const DSLError& e) {
        m_errorLabel->setString(e.what());
        m_errorLabel->setColor({255, 80, 80});
        m_errorLabel->setVisible(true);
    }
}

void CodeEditorPopup::onCycleTriggerMode(CCObject*) {
    m_entry->triggerMode = nextTriggerMode(m_entry->triggerMode);
    updateTriggerModeLabel();
    LevelScriptStore::save();
}

void CodeEditorPopup::updateTriggerModeLabel() {
    std::string text = fmt::format("Mode: {}", triggerModeName(m_entry->triggerMode));
    m_triggerModeLabel->setString(text.c_str());
}

// ==================== ScriptListPopup ====================

ScriptListPopup* ScriptListPopup::create() {
    auto* ret = new ScriptListPopup();
    if (ret->initPopup()) {
        ret->autorelease();
        return ret;
    }
    delete ret;
    return nullptr;
}

bool ScriptListPopup::initPopup() {
    if (!Popup::init(340.f, 280.f)) return false;

    this->setTitle("Script Manager");

    auto winSize = m_mainLayer->getContentSize();

    // Add script button
    auto btnMenu = CCMenu::create();
    btnMenu->setPosition({0, 0});
    m_mainLayer->addChild(btnMenu);

    auto addSpr = ButtonSprite::create("+ Add Script", "goldFont.fnt", "GJ_button_01.png", 0.7f);
    auto addBtn = CCMenuItemSpriteExtra::create(
        addSpr, this, menu_selector(ScriptListPopup::onAddScript)
    );
    addBtn->setPosition({winSize.width / 2, 30});
    btnMenu->addChild(addBtn);

    // Scroll area for script list
    m_scrollLayer = ScrollLayer::create({300.f, 190.f});
    m_scrollLayer->setPosition({20.f, 45.f});
    m_mainLayer->addChild(m_scrollLayer);

    auto bg = CCScale9Sprite::create("square02_small.png");
    bg->setContentSize({300.f, 190.f});
    bg->setPosition({20.f + 150.f, 45.f + 95.f});
    bg->setOpacity(60);
    m_mainLayer->addChild(bg, -1);

    refreshList();
    return true;
}

void ScriptListPopup::refreshList() {
    auto* content = m_scrollLayer->m_contentLayer;
    content->removeAllChildren();

    auto& scripts = LevelScriptStore::scripts();
    float itemHeight = 40.f;
    float totalHeight = std::max(190.f, itemHeight * static_cast<float>(scripts.size()));
    content->setContentSize({300.f, totalHeight});

    for (int i = 0; i < static_cast<int>(scripts.size()); i++) {
        auto& entry = scripts[i];
        float y = totalHeight - (i + 1) * itemHeight;

        auto itemMenu = CCMenu::create();
        itemMenu->setPosition({0, 0});
        content->addChild(itemMenu);

        // Script name label
        auto nameLabel = CCLabelBMFont::create(entry.name.c_str(), "bigFont.fnt");
        nameLabel->setScale(0.35f);
        nameLabel->setAnchorPoint({0, 0.5f});
        nameLabel->setPosition({10, y + itemHeight / 2 + 5});
        if (!entry.enabled) nameLabel->setColor({150, 150, 150});
        content->addChild(nameLabel);

        // Mode label
        auto modeStr = fmt::format("[{}]", triggerModeName(entry.triggerMode));
        auto modeLabel = CCLabelBMFont::create(modeStr.c_str(), "chatFont.fnt");
        modeLabel->setScale(0.4f);
        modeLabel->setAnchorPoint({0, 0.5f});
        modeLabel->setPosition({10, y + itemHeight / 2 - 10});
        modeLabel->setColor({180, 180, 255});
        content->addChild(modeLabel);

        // Edit button
        auto editSpr = ButtonSprite::create("Edit", "goldFont.fnt", "GJ_button_04.png", 0.5f);
        auto editBtn = CCMenuItemSpriteExtra::create(
            editSpr, this, menu_selector(ScriptListPopup::onEditScript)
        );
        editBtn->setPosition({220, y + itemHeight / 2});
        editBtn->setTag(i);
        itemMenu->addChild(editBtn);

        // Delete button
        auto delSpr = CCSprite::createWithSpriteFrameName("GJ_trashBtn_001.png");
        delSpr->setScale(0.5f);
        auto delBtn = CCMenuItemSpriteExtra::create(
            delSpr, this, menu_selector(ScriptListPopup::onDeleteScript)
        );
        delBtn->setPosition({275, y + itemHeight / 2});
        delBtn->setTag(i);
        itemMenu->addChild(delBtn);

        // Toggle button
        auto toggleName = entry.enabled ? "GJ_checkOn_001.png" : "GJ_checkOff_001.png";
        auto toggleSpr = CCSprite::createWithSpriteFrameName(toggleName);
        toggleSpr->setScale(0.5f);
        auto toggleBtn = CCMenuItemSpriteExtra::create(
            toggleSpr, this, menu_selector(ScriptListPopup::onToggleScript)
        );
        toggleBtn->setPosition({185, y + itemHeight / 2});
        toggleBtn->setTag(i);
        itemMenu->addChild(toggleBtn);
    }

    m_scrollLayer->moveToTop();
}

void ScriptListPopup::onAddScript(CCObject*) {
    LevelScriptStore::addScript();
    refreshList();
}

void ScriptListPopup::onEditScript(CCObject* sender) {
    int idx = static_cast<CCMenuItemSpriteExtra*>(sender)->getTag();
    auto& scripts = LevelScriptStore::scripts();
    if (idx < 0 || idx >= static_cast<int>(scripts.size())) return;

    auto popup = CodeEditorPopup::create(&scripts[idx]);
    if (popup) popup->show();
}

void ScriptListPopup::onDeleteScript(CCObject* sender) {
    int idx = static_cast<CCMenuItemSpriteExtra*>(sender)->getTag();
    LevelScriptStore::removeScript(idx);
    refreshList();
}

void ScriptListPopup::onToggleScript(CCObject* sender) {
    int idx = static_cast<CCMenuItemSpriteExtra*>(sender)->getTag();
    auto& scripts = LevelScriptStore::scripts();
    if (idx < 0 || idx >= static_cast<int>(scripts.size())) return;

    scripts[idx].enabled = !scripts[idx].enabled;
    LevelScriptStore::save();
    refreshList();
}

} // namespace scriptable
