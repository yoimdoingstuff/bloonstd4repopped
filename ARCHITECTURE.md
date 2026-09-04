# Architecture

## Overview

This project is a native C++ cross-platform game.

The project is split into:

- Game Engine
- Platform Backends
- Game Builder
- Asset Pipeline
- Tools
- Tests

The game engine contains platform-independent code.
Platform-specific functionality is implemented through platform backends.

## Repository Structure

engine/
    core/
    game/
    rendering/
    audio/
    input/
    assets/
    save/
    achievements/
    map/

platform/
    windows/
    linux/
    psp/
    xbox360/

builder/
    app/
    ui/
    importer/
    converter/
    build/

tools/
    asset_importer/
    swf/
    ipa/
    asset_converter/

tests/

## Dependency Structure

The architecture should follow:

Game
 ↓
Engine Interfaces
 ↓
Platform Backend
 ↓
Operating System / Console APIs

The game engine must not directly use PSP, Windows, Linux,
or Xbox-specific APIs.

## Game Engine

### Core

Handles:

- Game loop
- Timing
- Events
- Logging
- Configuration

Filesystem access is expressed through `IFileSystem`. Desktop backends share
`NativeFileSystem`; console backends provide their own implementation. The
interface reads and writes byte buffers, creates directories, and enumerates
files without exposing operating-system APIs to game code.

Audio is expressed through `IAudio` using manifest-defined sound identifiers.
Platform backends own decoding and playback; `NullAudio` is an explicit silent
fallback for unavailable backends and test environments.

Input backends establish their previous-state snapshot before the platform
event pump. This makes pressed and released actions deterministic within a
frame; physical input sources are combined only after their individual states
are updated.

Renderers may be initialized with a platform-owned window or may create one
themselves. Reinitializing an already-bound renderer is a no-op, which lets
the engine safely receive a renderer prepared by its platform host.

Platform build operations return success only after they execute and verify
their requested action. Until the Game Builder owns a real build executor,
backend configure, build, and package requests fail explicitly with guidance
instead of reporting fictional artifacts.

### Game

Handles:

- Towers
- Bloons
- Projectiles
- Rounds
- Economy
- Upgrades
- Maps

Game logic must not depend on rendering or platform APIs.

### Rendering

Handles:

- Sprites
- Textures
- Animation
- Fonts
- Cameras
- UI rendering

The renderer uses platform-specific backends.

### Input

The engine uses abstract actions such as:

- Confirm
- Cancel
- Pause
- Upgrade
- Sell
- Move

Each platform converts its physical controls into these actions.

## Platform Backends

### Windows

Provides:

- Window
- Rendering
- Input
- Audio
- Filesystem

### Linux

Provides:

- Window
- Rendering
- Input
- Audio
- Filesystem

### PSP

Provides:

- GU rendering
- Controller input
- Audio
- Filesystem
- Timing
- EBOOT packaging

The PSP backend must not contain game logic.

### Xbox 360

Reserved for future platform-specific functionality.

## Asset Pipeline

Source assets are never loaded directly by the game.

Instead:

Source File
 ↓
Importer
 ↓
Converter
 ↓
Internal Asset Format
 ↓
Game

Supported import sources may include:

- SWF
- IPA
- APK
- Images
- Audio

The runtime only understands the project's internal asset format.

## Game Builder

The Game Builder is a separate native C++ application.

It uses the same project and asset formats as the game.

It provides:

- Project management
- Asset importing
- Asset conversion
- Map editing
- Configuration
- Building
- Packaging

The Builder must not duplicate game logic that already exists in
the engine.

## Maps

Maps are data-driven.

A map contains:

- Paths
- Waypoints
- Spawn points
- Exit points
- Buildable areas
- Objects
- Backgrounds

The game and map editor use the same map format.

## Data-Driven Systems

Where practical, game content should be data-driven.

This includes:

- Towers
- Bloons
- Upgrades
- Rounds
- Maps
- Achievements

Game code should provide the systems that interpret this data.

## Saves

Save data uses a platform-independent format where possible.

Platform-specific storage is handled by the platform backend.

## Multiplayer

Multiplayer will be implemented after the single-player
simulation is stable.

The simulation should be designed with deterministic behavior
where practical so it can later support multiplayer.

## Performance

The PSP is a major performance constraint.

The architecture should favor:

- Low memory usage
- Reusable objects
- Texture atlases
- Batching
- Minimal allocations
- Efficient data structures

Performance decisions should be based on profiling rather than
guesswork.

## Architectural Rules

1. Shared engine code must remain platform-independent.
2. Platform APIs belong in platform backends.
3. Game logic must not depend on rendering.
4. Rendering must not contain game rules.
5. The Builder should reuse engine/data systems where possible.
6. Importers must be separate from runtime code.
7. New systems should reuse existing abstractions instead of
   creating duplicates.

---

## Placeholder Asset System

The repository ships open-source placeholder assets (CC0 / MIT-licensed) so
the engine can compile, launch, and be tested without a user-supplied SWF or
IPA.

Directory layout:

```
assets/
├── placeholder/
│   ├── manifest.json          ← lists every asset the engine expects
│   ├── textures/              ← placeholder sprite sheets / textures
│   ├── audio/                 ← silent or royalty-free placeholder sounds
│   ├── music/                 ← royalty-free placeholder music
│   ├── maps/                  ← hand-authored open-data map files
│   ├── towers/                ← placeholder tower data JSON
│   ├── bloons/                ← placeholder bloon data JSON
│   └── rounds/                ← hand-authored round data JSON (50 rounds)
└── (real assets go here after import)
```

At startup the asset loader checks for the real asset package produced by the
importer. If it is absent, the placeholder package is used instead. The engine
never hard-codes a path to a specific asset — it always resolves through the
manifest.

The placeholder round data, bloon stats, and tower stats are hand-authored to
match publicly available BTD4 game data as accurately as possible without
copying proprietary files.

---

## 1:1 Accuracy Data Layer

The engine targets full fidelity with the original BTD4. All game content is
represented through the internal data format:

### Tower data (`data/towers/<id>.json`)
- Name, cost, sell value, range, attack speed, damage, pierce
- Upgrade paths (two paths, four tiers each)
- Special ability definition where applicable
- `platform_content` flag for mobile-only towers (e.g. Beekeeper)

### Bloon data (`data/bloons/<type>.json`)
- Type enum, speed, RBE, health
- Children on pop (type → count)
- Immunity flags (Sharp, Explosive, Energy)

### Round data (`data/rounds.json`)
- 50 rounds matching original BTD4 wave sequences
- Each round: array of bloon groups (type, count, spacing ms)

### Map data (`data/maps/<name>.json`)
- Path waypoints matching original map layouts
- Buildable / blocked regions
- Spawn and exit points
- Background asset reference

All of the above files live in the internal game data package. The importer
fills them from the SWF/IPA. If the importer has not been run, the placeholder
versions are used.

---

## UI Architecture

### UI Baseline: Mobile Port Evolution

The primary UI target is an evolution of the BTD4 mobile (IPA) port UI.
Reasons:
- Mobile UI is designed for a variety of screen sizes and touch targets.
- It adapts more naturally to desktop, PSP, and controller input.
- Flash UI assumed mouse and fixed browser canvas dimensions.

The Flash UI layout is a secondary "graphics style" selection, affecting
visuals only — it does not change the underlying UI logic.

### UIAdapter / IPlatformUI

The engine exposes a `UIAdapter` interface. Each platform backend provides a
concrete `PlatformUI` that returns layout constants, control mappings, and
interaction models appropriate for that platform.

```
engine/ui/
├── UIAdapter.hpp          ← abstract interface
├── UILayout.hpp           ← layout constants and widget descriptors
├── HUD.hpp / HUD.cpp      ← shared HUD logic (cash, lives, round)
├── TowerPanel.hpp         ← tower selection / upgrade panel logic
├── MainMenu.hpp           ← main menu state rendering
└── PauseMenu.hpp

platform/windows/ui/       ← desktop mouse-driven layout
platform/linux/ui/         ← same as windows
platform/psp/ui/           ← PSP D-pad/button layout
platform/xbox360/ui/       ← controller-driven layout (reserved)
```

`UIAdapter` methods include:

```cpp
class UIAdapter {
public:
    virtual UILayout    getLayout() const = 0;
    virtual InputScheme getInputScheme() const = 0;  // Mouse, DPad, Gamepad
    virtual bool        isPointerDriven() const = 0;
    virtual ~UIAdapter() = default;
};
```

### Platform UI Layouts

#### Desktop (Windows / Linux)
- Mouse-driven
- Tower selection via sidebar panel (mobile-port style)
- Scalable to any resolution while preserving 480×272 logical units
- Keyboard shortcuts available

#### PSP
- D-pad / analog to navigate cursor
- Cross / Circle / Square / Triangle mapped to confirm / cancel / sell / upgrade
- L / R buttons cycle targeting mode and tower selection
- HUD elements positioned to avoid PSP button cluster
- No on-screen keyboard (use preset names)
- Matches original BTD PSP control scheme

#### Xbox 360 (future)
- Left stick navigation
- A / B / X / Y mapped to confirm / cancel / sell / upgrade
- LB / RB cycle tower selection
- Guide button → pause

### Graphics Style

Two visual styles are supported. The style is chosen in the Game Builder at
build time and baked into the packaged game's asset set.

| Style  | Source   | Requirement            |
|--------|----------|------------------------|
| Flash  | SWF      | SWF must be provided   |
| Mobile | IPA      | IPA must be provided   |

If only a SWF is supplied, Flash style is the only available option.
If an IPA is also supplied, Mobile style becomes available.

The engine itself is style-agnostic — it loads whichever texture atlas the
build package contains. The `manifest.json` records which style was built.

The Game Builder exposes:

```
Graphics Style:
  ( ) Flash  — use SWF-derived sprites and UI art
  ( ) Mobile — use IPA-derived sprites and UI art
```

Selecting Mobile style when no IPA is loaded is disabled with an explanation:

```
Mobile style requires an IPA file.
Please add an IPA in the Source Files panel.
```

---

## Repository Layout (Updated)

```
engine/
    core/
    game/
    rendering/
    audio/
    input/
    assets/
    save/
    achievements/
    map/
    ui/                ← new: UIAdapter, UILayout, HUD, panels

platform/
    windows/
        ui/            ← desktop PlatformUI
    linux/
        ui/            ← desktop PlatformUI
    psp/
        ui/            ← PSP D-pad PlatformUI
    xbox360/
        ui/            ← controller PlatformUI (reserved)

builder/
    app/
    ui/
    importer/
    converter/
    build/

assets/
    placeholder/       ← CC0/MIT placeholder assets shipped with repo
        manifest.json
        textures/
        audio/
        music/
        maps/
        towers/
        bloons/
        rounds/

tools/
    asset_importer/
    swf/
    ipa/
    asset_converter/

tests/
```
