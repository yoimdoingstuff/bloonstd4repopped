# Internal round format, version 1

The runtime consumes original or imported internal data through `parseRounds`
or `loadRounds` in `engine/game/Rounds.hpp`. Loading requires the selected Map
so invalid path references fail before gameplay. Failed loading leaves the
previous RoundSet unchanged. No SWF/APK/IPA parsing is involved.

```json
{
  "version": 1,
  "rounds": [
    {
      "groups": [
        {"type": "red", "count": 3, "spacing_ms": 500},
        {"type": "blue", "count": 2, "spacing_ms": 250,
         "delay_ms": 1000, "path": 0}
      ]
    }
  ]
}
```

This is original example data, not the original game's round sequence.
Rounds are numbered from one by array order. Groups are sequential. `delay_ms`
is the delay from the preceding group's last scheduled spawn (or from round
start for the first group). `spacing_ms` separates spawns within that group.
The example spawns reds at 0, 500 and 1000 ms, then blues at 2000 and 2250 ms.
Zero spacing is supported for simultaneous groups. Delay defaults to zero;
path defaults to zero. Type, count and spacing are required.

Supported type IDs follow the existing engine roster: red, blue, green, yellow,
pink, black, white, lead, rainbow, ceramic and moab. IDs are case-sensitive.
Unknown types/fields, duplicate fields, unsupported versions, fractional or
negative integer values and trailing data fail validation.

Limits are 1 MiB UTF-8 JSON, 1024 rounds, 1024 groups per round, 100000 scheduled
bloons per round and a one-hour scheduled duration per round. Each count must
be positive. Each timing value is an integer from 0 to 3600000 milliseconds.
JSON path indices are 0..63 and must exist on the selected map. At least one
round and one group per round are required. File reads use IFileSystem and
currently buffer the file before checking the parser's size limit.

## Simulation integration

1. Load a Map and RoundSet.
2. Call `GameSimulation::setRounds(std::move(rounds), error)` before starting.
3. Call `startNextRound()` when the user requests a round.
4. Call `update()` with the existing fixed simulation timestep.

RoundScheduler uses only indices and counters during scheduling; it does not
allocate a list of individual spawn events. Time-zero spawns happen at start.
Later spawns happen at the end of the first update whose elapsed time meets
their deadline. Their positions start at the selected path's first waypoint;
they do not move for time before they existed. This quantizes timing to the
simulation tick; it is not a claim of cross-platform lockstep determinism.

When the bloon pool fills, the next spawn stays pending and is retried without
changing its scheduled order. A round ends only after all groups are emitted
and the entire pool, including child bloons, is empty. The simulation awards
`Economy::calculateRoundReward(completedRound)` exactly once, clears lingering
projectiles and waits for the next explicit start. Clearing the final round
sets Victory. Defeat takes precedence over a completion reward.

Pause stops both simulation and spawn time. Reset clears entities/progression
and restarts the configured round set from round one. Round data cannot be
replaced during an active or already-progressed run without reset. The existing
manual-spawn simulation remains usable with no RoundSet configured. The map
must remain unchanged during an active round; path references are rechecked
before every new round starts. `setCurrentRound` remains a legacy display
setter; it does not seek the scheduler or change managed completion rewards.

This adds simulation APIs, not game-window controls. Freeplay, original round
content and wiring the TestScreen application to gameplay remain future tasks.
Existing combat and leak rules are unchanged.

## Validation

On Windows, the portable Zig 0.14.1 C++17 compiler built the map, gameplay and
round modules together. All 19 standalone regression suites passed (7 existing
simulation, 5 map-loader and 7 round suites). The new tests are also registered
in the normal CMake `btd4_tests` target. Full SDL application/CMake builds, Linux
and PSP were not exercised in this task.
