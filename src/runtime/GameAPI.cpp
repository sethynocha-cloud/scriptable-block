#include "GameAPI.hpp"

namespace scriptable {

void GameAPI::init(PlayLayer* layer) {
    m_layer = layer;

    // --- Object operations (group-based) ---
    m_callbacks.moveObject = [this](int groupID, float dx, float dy) {
        if (!m_layer) return;
        auto* group = m_layer->m_groups[groupID];
        if (!group) return;
        for (auto* obj : CCArrayExt<GameObject*>(group)) {
            obj->setPosition({obj->getPositionX() + dx, obj->getPositionY() + dy});
        }
    };

    m_callbacks.moveObjectTo = [this](int groupID, float x, float y) {
        if (!m_layer) return;
        auto* group = m_layer->m_groups[groupID];
        if (!group) return;
        for (auto* obj : CCArrayExt<GameObject*>(group)) {
            obj->setPosition({x, y});
        }
    };

    m_callbacks.rotateObject = [this](int groupID, float degrees) {
        if (!m_layer) return;
        auto* group = m_layer->m_groups[groupID];
        if (!group) return;
        for (auto* obj : CCArrayExt<GameObject*>(group)) {
            obj->setRotation(obj->getRotation() + degrees);
        }
    };

    m_callbacks.scaleObject = [this](int groupID, float scale) {
        if (!m_layer) return;
        auto* group = m_layer->m_groups[groupID];
        if (!group) return;
        for (auto* obj : CCArrayExt<GameObject*>(group)) {
            obj->setScale(scale);
        }
    };

    m_callbacks.scaleObjectXY = [this](int groupID, float sx, float sy) {
        if (!m_layer) return;
        auto* group = m_layer->m_groups[groupID];
        if (!group) return;
        for (auto* obj : CCArrayExt<GameObject*>(group)) {
            obj->setScaleX(sx);
            obj->setScaleY(sy);
        }
    };

    m_callbacks.toggleObject = [this](int groupID, bool visible) {
        if (!m_layer) return;
        auto* group = m_layer->m_groups[groupID];
        if (!group) return;
        for (auto* obj : CCArrayExt<GameObject*>(group)) {
            obj->setVisible(visible);
        }
    };

    m_callbacks.setObjectAlpha = [this](int groupID, float alpha) {
        if (!m_layer) return;
        auto* group = m_layer->m_groups[groupID];
        if (!group) return;
        for (auto* obj : CCArrayExt<GameObject*>(group)) {
            obj->setOpacity(static_cast<unsigned char>(alpha * 255.f));
        }
    };

    m_callbacks.setObjectColor = [this](int groupID, int r, int g, int b) {
        if (!m_layer) return;
        auto* group = m_layer->m_groups[groupID];
        if (!group) return;
        for (auto* obj : CCArrayExt<GameObject*>(group)) {
            obj->setColor({
                static_cast<unsigned char>(r),
                static_cast<unsigned char>(g),
                static_cast<unsigned char>(b)
            });
        }
    };

    // --- Player operations ---
    m_callbacks.setPlayerSpeed = [this](float multiplier) {
        if (!m_layer || !m_layer->m_player1) return;
        m_layer->m_player1->m_playerSpeed = multiplier;
    };

    m_callbacks.setPlayerGravity = [this](bool flipped) {
        if (!m_layer || !m_layer->m_player1) return;
        m_layer->m_player1->m_isUpsideDown = flipped;
    };

    m_callbacks.pulsePlayer = [this](int r, int g, int b, float duration) {
        if (!m_layer || !m_layer->m_player1) return;
        auto* player = m_layer->m_player1;
        auto tintTo = CCTintTo::create(duration / 2.f,
            static_cast<unsigned char>(r),
            static_cast<unsigned char>(g),
            static_cast<unsigned char>(b));
        auto tintBack = CCTintTo::create(duration / 2.f, 255, 255, 255);
        player->runAction(CCSequence::create(tintTo, tintBack, nullptr));
    };

    m_callbacks.enableTrail = [this](bool enable) {
        if (!m_layer || !m_layer->m_player1) return;
        m_layer->m_player1->m_hasGlow = enable;
    };

    m_callbacks.killPlayer = [this]() {
        if (!m_layer || !m_layer->m_player1) return;
        m_layer->destroyPlayer(m_layer->m_player1, nullptr);
    };

    m_callbacks.forceJump = [this]() {
        if (!m_layer || !m_layer->m_player1) return;
        m_layer->m_player1->pushButton(PlayerButton::Jump);
        m_jumpReleaseTimer = 0.016f;
    };

    // --- Camera operations ---
    m_callbacks.moveCamera = [this](float dx, float dy) {
        if (!m_layer) return;
        m_layer->m_gameState.m_cameraOffset.x += dx;
        m_layer->m_gameState.m_cameraOffset.y += dy;
    };

    m_callbacks.zoomCamera = [this](float zoom) {
        if (!m_layer) return;
        m_layer->m_objectLayer->setScale(zoom);
    };

    m_callbacks.shakeCamera = [this](float strength, float duration) {
        if (!m_layer) return;
        m_layer->shakeCamera(strength, 0.f, duration);
    };

    m_callbacks.rotateCamera = [this](float degrees) {
        if (!m_layer) return;
        m_layer->m_objectLayer->setRotation(
            m_layer->m_objectLayer->getRotation() + degrees
        );
    };

    // --- Color channel operations ---
    m_callbacks.setColorChannel = [this](int channel, int r, int g, int b) {
        if (!m_layer) return;
        ccColor3B color = {
            static_cast<unsigned char>(r),
            static_cast<unsigned char>(g),
            static_cast<unsigned char>(b)
        };
        cocos2d::ccHSVValue hsv = {0, 0, 0, false, false};
        m_layer->updateColor(color, 1.0f, channel, false, 1.0f, hsv, 0, false, nullptr, 0, 0);
    };

    m_callbacks.pulseColorChannel = [this](int channel, int r, int g, int b, float duration) {
        if (!m_layer) return;
        ccColor3B color = {
            static_cast<unsigned char>(r),
            static_cast<unsigned char>(g),
            static_cast<unsigned char>(b)
        };
        cocos2d::ccHSVValue hsv = {0, 0, 0, false, false};
        m_layer->updateColor(color, duration, channel, true, 1.0f, hsv, 0, false, nullptr, 0, 0);
    };

    // --- Read-only player state ---
    m_callbacks.getPlayerX = [this]() -> float {
        if (!m_layer || !m_layer->m_player1) return 0.f;
        return m_layer->m_player1->getPositionX();
    };

    m_callbacks.getPlayerY = [this]() -> float {
        if (!m_layer || !m_layer->m_player1) return 0.f;
        return m_layer->m_player1->getPositionY();
    };

    m_callbacks.getPlayerXVel = [this]() -> float {
        if (!m_layer || !m_layer->m_player1) return 0.f;
        return static_cast<float>(m_layer->m_player1->m_speedMultiplier);
    };

    m_callbacks.getPlayerYVel = [this]() -> float {
        if (!m_layer || !m_layer->m_player1) return 0.f;
        return m_layer->m_player1->m_yVelocity;
    };

    m_callbacks.getLevelTime = [this]() -> float {
        if (!m_layer) return 0.f;
        return m_layer->m_gameState.m_levelTime;
    };

    m_callbacks.getAttempt = [this]() -> int {
        if (!m_layer) return 1;
        return m_layer->m_attempts;
    };
}

} // namespace scriptable
