#pragma once

#include "Rounds.hpp"
#include <cstddef>
#include <cstdint>

namespace btd4 {

// Deterministic post-campaign round generation. The generator deliberately
// works from gameplay data rather than platform/UI code so it can be shared by
// desktop and constrained targets such as PSP.
struct FreeplayConfig {
    size_t startRound{1};
    size_t maxRounds{10000};
    float healthScalePerRound{0.02f};
    float speedScalePerRound{0.005f};
    uint32_t minSpacingMs{25};
};

struct FreeplayRound {
    size_t roundNumber{0};
    float healthScale{1.0f};
    float speedScale{1.0f};
    RoundDefinition definition;
};

class FreeplayGenerator {
public:
    explicit FreeplayGenerator(FreeplayConfig config = {});

    const FreeplayConfig& config() const { return m_config; }
    bool canGenerate(size_t roundNumber) const;

    // Generates a deterministic round from the existing campaign round set.
    // The source round is selected cyclically, while pressure scales smoothly
    // as freeplay progresses. Returns false for invalid/out-of-range requests.
    bool generate(size_t roundNumber, const RoundSet& campaign,
                  FreeplayRound& output) const;

private:
    FreeplayConfig m_config;
};

} // namespace btd4
