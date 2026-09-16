#include "Freeplay.hpp"

#include <algorithm>
#include <cmath>

namespace btd4 {

FreeplayGenerator::FreeplayGenerator(FreeplayConfig config) : m_config(config) {
    if (m_config.startRound == 0) m_config.startRound = 1;
    if (m_config.maxRounds < m_config.startRound) m_config.maxRounds = m_config.startRound;
    m_config.healthScalePerRound = std::max(0.0f, m_config.healthScalePerRound);
    m_config.speedScalePerRound = std::max(0.0f, m_config.speedScalePerRound);
    m_config.minSpacingMs = std::max<uint32_t>(1, m_config.minSpacingMs);
}

bool FreeplayGenerator::canGenerate(size_t roundNumber) const {
    return roundNumber >= m_config.startRound && roundNumber <= m_config.maxRounds;
}

bool FreeplayGenerator::generate(size_t roundNumber, const RoundSet& campaign,
                                 FreeplayRound& output) const {
    if (!canGenerate(roundNumber) || campaign.rounds.empty()) return false;

    const size_t sourceIndex = (roundNumber - m_config.startRound) % campaign.rounds.size();
    const float progress = static_cast<float>(roundNumber - m_config.startRound);
    const float healthScale = 1.0f + progress * m_config.healthScalePerRound;
    const float speedScale = 1.0f + progress * m_config.speedScalePerRound;

    FreeplayRound generated;
    generated.roundNumber = roundNumber;
    generated.healthScale = healthScale;
    generated.speedScale = speedScale;
    generated.definition = campaign.rounds[sourceIndex];

    for (auto& group : generated.definition.groups) {
        // Increase pressure without changing the bloon type set. This keeps
        // freeplay compatible with the existing scheduler and map validation.
        const double scaledCount = std::ceil(static_cast<double>(group.count) * speedScale);
        group.count = static_cast<uint32_t>(std::min<double>(100000.0, scaledCount));
        if (group.count == 0) group.count = 1;

        const double scaledSpacing = static_cast<double>(group.spacingMs) / speedScale;
        group.spacingMs = static_cast<uint32_t>(std::max<double>(m_config.minSpacingMs, scaledSpacing));
    }

    output = std::move(generated);
    return true;
}

} // namespace btd4
