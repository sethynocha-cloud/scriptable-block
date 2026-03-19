#pragma once
#include <Geode/Geode.hpp>
#include "../dsl/Interpreter.hpp"

using namespace geode::prelude;

namespace scriptable {

class GameAPI {
    PlayLayer* m_layer = nullptr;
    GameCallbacks m_callbacks;
    float m_jumpReleaseTimer = 0.f;

public:
    void init(PlayLayer* layer);
    GameCallbacks* getCallbacks() { return &m_callbacks; }

    void update(float dt) {
        if (m_jumpReleaseTimer > 0.f) {
            m_jumpReleaseTimer -= dt;
            if (m_jumpReleaseTimer <= 0.f && m_layer && m_layer->m_player1) {
                m_layer->m_player1->releaseButton(PlayerButton::Jump);
                m_jumpReleaseTimer = 0.f;
            }
        }
    }
};

} // namespace scriptable
