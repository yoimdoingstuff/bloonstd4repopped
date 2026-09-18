#pragma once

#include "Economy.hpp"
#include <cstdint>
#include <string>

namespace btd4 {

class Player {
public:
    static constexpr uint8_t MaxPlayers = 4;

    Player() = default;
    explicit Player(uint8_t id, std::string name = {});

    uint8_t id() const { return m_id; }
    const std::string& name() const { return m_name; }
    void setName(std::string name);

    bool active() const { return m_active; }
    void setActive(bool active) { m_active = active; }

    int32_t controllerInstanceId() const { return m_controllerInstanceId; }
    void setControllerInstanceId(int32_t instanceId) { m_controllerInstanceId = instanceId; }
    bool hasController() const { return m_controllerInstanceId >= 0; }

    Economy& economy() { return m_economy; }
    const Economy& economy() const { return m_economy; }

    void reset(int cash = 650, int lives = 100);

private:
    uint8_t m_id{0};
    std::string m_name{"Player 1"};
    bool m_active{true};
    int32_t m_controllerInstanceId{-1};
    Economy m_economy{};
};

} // namespace btd4
