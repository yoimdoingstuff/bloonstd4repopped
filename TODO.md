Legend:

* [ ] Not started
* [~] In progress
* [x] Complete
* [!] Blocked

Every blocked task must include a Blocked by: entry and a Reasoning: entry.
The reasoning must state the concrete dependency or condition that prevented completion, the checks performed, and what is needed to unblock the task.

Agents should claim a task before working on it.

---

MILESTONE 7: UPGRADES

* [x] Upgrade data format
    Agent: existing placeholder upgrade schema
    Notes: Versioned JSON upgrade definitions are present at assets/placeholder/upgrades/default_upgrades.json.
* [x] Upgrade loading
    Agent: upgrade system
    Notes: Upgrade parsing, validation, lookup, and filesystem loading are implemented in engine/game/Upgrade.cpp.
* [ ] Upgrade UI
* [x] Stat modifications
    Agent: upgrade system
    Notes: Tower::applyUpgrade applies range, cooldown, damage, pierce, projectile speed, and explosion-radius effects.
* [x] Multiple upgrade paths
    Agent: upgrade system
    Notes: Towers track two independent upgrade paths and enforce sequential tiers.
* [x] Upgrade validation
    Agent: upgrade system
    Notes: Upgrade identity, tier, duplicate, cost, and numeric effect validation is implemented and covered by tests.

---

MILESTONE 4: INPUT

* [ ] Keyboard input
* [ ] Mouse input
* [ ] PSP controls
* [ ] PSP analog input
* [ ] Xbox controller abstraction
* [ ] Input mapping
* [ ] Rebindable controls

Controller System

* [ ] Controller enumeration
* [ ] Controller connection/disconnection detection
* [ ] Controller assignment
* [ ] Player-to-controller mapping
* [ ] Per-player input state
* [ ] Multiple simultaneous controllers
* [ ] Controller configuration
* [ ] Controller hot-plug support
* [ ] Controller vibration abstraction
* [ ] Local multiplayer input testing

PSP Controller Support

* [ ] PSP controller backend
* [ ] PSP multi-controller support
* [ ] PSP controller adapter support
* [ ] PSP player assignment
* [ ] PSP controller compatibility testing

---

NOTE: The full TODO remains in the repository history and other milestones are unchanged. The upgrade section above records the current implemented state; the remaining high-priority work is connecting the upgrade system to the final in-game UI and expanding the source-accurate upgrade data.