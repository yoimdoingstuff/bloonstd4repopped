Project: Cross-Platform BTD4 Game, Asset Importer and Game Builder

Project Goal

Create a high-performance, cross-platform re-implementation of Bloons Tower Defense 4 (BTD4).

The goal is 1:1 fidelity with the original game wherever possible:

* Same tower roster, names, costs, stats, and upgrade paths as BTD4
* Same bloon types, speeds, RBE values, and child hierarchies
* Same 50 rounds of wave data with identical bloon groups and spawn timing
* Same map layouts, paths, buildable/blocked regions, and names
* Same economy values (starting cash, round rewards, sell ratios)
* Same achievement conditions and IDs
* Same special abilities and tower behaviours

The base UI target is an evolution of the BTD4 mobile (IPA) port, which has
a more universal layout. The Flash UI is considered secondary. Both visual
styles (Flash and Mobile) are selectable at build time.

The project must support:

* PSP
* Windows
* Linux
* Xbox 360
* Future platforms through a backend architecture

The project must also include a native desktop application called the Game Builder.

The Game Builder allows the user to:

1. Select a user-provided SWF file.
2. Optionally select a user-provided IPA containing the mobile version.
3. Import and convert supported assets.
4. Choose a graphics style (Flash or Mobile).
5. Select which platform to build for.
6. Configure available features.
7. Build the game automatically.
8. Produce the appropriate executable/package for the selected platform.

The repository must NOT contain proprietary BTD4 assets, source code, sounds,
music, maps, or other copyrighted game content.

The user supplies their own source files.

Open-source placeholder assets (CC0/MIT-licensed) are included in
assets/placeholder/ so the engine can compile, launch, and be tested without
a user-supplied SWF or IPA. When real assets are present they override the
placeholders at load time.

⸻

1. High-Level Architecture

Use three major components:

                    PROJECT
                       |
        +--------------+--------------+
        |              |              |
     Game Engine    Asset Tools    Game Builder
        |              |              |
        +--------------+--------------+
                       |
                 Platform Backends
                       |
       +-------+-------+-------+-------+
       |       |       |       |       |
      PSP   Windows  Linux  Xbox360  Future

The game engine must never depend on the Game Builder.

The Game Builder is a development/packaging tool that prepares assets and invokes the appropriate platform build.

⸻

2. Repository Layout

Use:

project/
├── CMakeLists.txt
├── CMakePresets.json
├── README.md
├── LICENSE
├── docs/
│   ├── architecture.md
│   ├── building.md
│   ├── asset-format.md
│   ├── modding.md
│   ├── map-editor.md
│   ├── multiplayer.md
│   └── platforms.md
│
├── engine/
│   ├── core/
│   ├── game/
│   ├── rendering/
│   ├── audio/
│   ├── input/
│   ├── assets/
│   ├── save/
│   ├── achievements/
│   ├── multiplayer/
│   └── map/
│
├── platform/
│   ├── common/
│   ├── windows/
│   ├── linux/
│   ├── psp/
│   └── xbox360/
│
├── builder/
│   ├── app/
│   ├── ui/
│   ├── project/
│   ├── importer/
│   ├── converter/
│   ├── build/
│   └── platform/
│
├── tools/
│   ├── asset_importer/
│   ├── swf/
│   ├── ipa/
│   └── asset_converter/
│
├── assets/
│   └── example/
│
├── tests/
│
└── .github/
    └── workflows/
        ├── build-windows.yml
        ├── build-linux.yml
        ├── build-psp.yml
        ├── build-xbox360.yml
        └── build-all.yml

⸻

3. Programming Language

Use C++ as the primary language.

Prioritize:

* performance
* deterministic behaviour
* low memory usage
* portability
* predictable compilation
* minimal dependencies

Avoid unnecessary runtime frameworks.

Do NOT use:

* Electron
* Chromium
* WebView
* HTML/CSS UI
* JavaScript-based application UI
* browser-based game rendering

The game must be native.

The Game Builder must also be native.

⸻

4. Game Builder UI

Create a native Windows/Linux desktop application.

Preferred UI technology:

Dear ImGui for the builder UI.

Use native C++.

The builder should remain lightweight and start quickly.

Do not use Electron.

Do not use a web frontend.

Do not embed a browser.

The builder should contain:

+------------------------------------------------+
| Cross-Platform Game Builder                    |
+------------------------------------------------+
| Source Files                                   |
|                                                |
| SWF:  [ Select SWF... ]                        |
| IPA:  [ Select IPA... ]  Optional              |
|                                                |
| Imported Features                              |
| [x] Base game                                  |
| [x] Towers                                     |
| [x] Bloons                                     |
| [x] Maps                                       |
| [x] Sounds                                     |
| [x] Achievements                               |
| [ ] Mobile-only content                        |
|                                                |
| Target Platform                                |
|                                                |
| ( ) Windows                                    |
| ( ) Linux                                      |
| ( ) PSP                                        |
| ( ) Xbox 360                                   |
| ( ) Other                                      |
|                                                |
| [ Configure ]                                  |
| [ Import Assets ]                              |
| [ Build Game ]                                 |
+------------------------------------------------+
| Build output / logs                             |
+------------------------------------------------+

The exact visual design can be improved later.

⸻

5. Source File Workflow

The Game Builder must support:

SWF only

and:

SWF + IPA

The IPA is optional.

SWF-only builds provide the base game content supported by the importer.

If an IPA is supplied, the importer attempts to identify and enable additional mobile-specific content.

The builder must clearly show what was detected.

Example:

SWF detected
Base game data: YES
IPA detected
Mobile content: YES
Detected:
✓ Mobile maps
✓ Beekeeper
✓ Mobile assets
✓ Mobile achievements/data
✓ Additional UI

If the IPA is missing:

Mobile content unavailable
The game can still be built using SWF content.

Never make the IPA mandatory.

⸻

6. Asset Import Pipeline

Create:

Input
  ↓
Detection
  ↓
Extraction
  ↓
Parsing
  ↓
Normalization
  ↓
Conversion
  ↓
Validation
  ↓
Internal Game Package

Internal package:

game_data/
├── textures/
├── sprites/
├── animations/
├── audio/
├── music/
├── maps/
├── towers/
├── bloons/
├── achievements/
├── ui/
├── localization/
└── manifest.json

The engine only reads the internal format.

It must not depend on SWF/APK/IPA formats at runtime.

⸻

7. SWF Import

Implement a SWF importer capable of:

* reading SWF structure
* extracting bitmap assets
* extracting vector graphics where possible
* extracting animation frames
* extracting sounds
* identifying text
* identifying useful game data
* generating normalized assets

The importer should be tolerant of different SWF versions.

Unsupported tags should produce warnings rather than crashing.

Example:

SWF Import
File: game.swf
Textures: 312
Sprites: 148
Animations: 97
Sounds: 84
Data objects: 63
Unsupported:
- Tag 123
- Tag 456
Import completed with warnings.

⸻

8. IPA Import

IPA importing is OPTIONAL.

The importer should accept:

game.ipa

and inspect the application bundle.

The tool should identify:

* textures
* sounds
* music
* maps
* configuration
* gameplay data
* mobile-exclusive content
* achievements
* UI resources

The importer must not assume every IPA has the same structure.

Different game versions should be supported through version detection.

Example:

IPA detected.
Version:
X.Y.Z
Mobile features detected:
✓ Beekeeper
✓ Mobile maps
✓ Mobile UI
✓ Mobile achievements
✓ Mobile assets

If a feature cannot be found:

Beekeeper:
Not detected

Do not fabricate missing content.

⸻

9. Mobile-Exclusive Features

When the IPA is successfully imported, expose mobile-specific content supported by that version.

This includes, where present:

* Beekeeper
* mobile-exclusive maps
* mobile-exclusive assets
* mobile-specific UI
* mobile achievements
* other mobile-specific gameplay/content

The engine must represent these as ordinary data-driven game content.

Do not hardcode the Beekeeper directly into the engine.

Example:

{
    "id": "beekeeper",
    "type": "tower",
    "platform_content": "mobile"
}

The importer determines whether the content exists.

⸻

10. Game Engine

The engine must be completely independent of the original Flash, Android or iOS runtime.

Implement original native systems for:

GameState
Entity
Tower
Bloon
Projectile
Round
Map
Economy
Upgrade
UI
Audio
Achievement
Save
Network

Everything should be data-driven where practical.

⸻

11. Rendering

Use a renderer abstraction.

Renderer
{
    initialize();
    beginFrame();
    drawTexture();
    drawSprite();
    drawText();
    drawRect();
    drawBatch();
    endFrame();
}

Backends:

Windows → native graphics backend / SDL2 where useful
Linux   → native graphics backend / SDL2 where useful
PSP     → PSP GU
Xbox360 → Xbox 360 graphics backend

The engine must never directly call platform graphics APIs.

⸻

12. Performance

Performance is a primary requirement.

Avoid unnecessary abstraction overhead in hot loops.

Prioritize:

* object pools
* contiguous data
* cache-friendly structures
* batched rendering
* texture atlases
* minimal draw calls
* minimal allocations
* precomputed paths
* fixed timestep simulation
* efficient collision detection
* efficient targeting
* asset streaming where appropriate

Do not allocate/deallocate game objects every frame.

Avoid unnecessary smart-pointer usage in performance-critical systems.

Use profiling builds.

The PSP build should be optimized specifically for PSP hardware.

The Xbox 360 build should likewise have platform-specific optimizations.

⸻

13. Logical Resolution

Use:

480x272

as the game’s logical PSP resolution.

Desktop versions should scale this appropriately.

Do not tie gameplay coordinates to the physical display resolution.

Example:

Game coordinate system
480x272
Windows:
960x544
1920x1088
etc.
PSP:
480x272

⸻

14. Tower System

Implement generic towers.

Properties:

position
range
attack cooldown
targeting mode
projectile
damage
pierce
upgrades
special ability

Targeting:

First
Last
Close
Strong

Use a modular targeting system.

⸻

15. Bloon System

Implement:

type
health
speed
path position
children
status effects

Support:

* movement
* popping
* splitting
* leaking
* special behaviour
* status effects
* spawning

Use pooling.

⸻

16. Round System

Rounds must be data-driven.

Example:

{
    "round": 1,
    "groups": [
        {
            "type": "red",
            "count": 10,
            "spacing": 30
        }
    ]
}

The importer can generate normalized round data when supported.

Do not require the engine to understand the original game’s internal format.

⸻

17. Map System

Maps should be data-driven.

Map
├── background
├── paths
├── buildable regions
├── tower restrictions
├── spawn points
└── exit points

Support multiple paths.

Support different map rules.

⸻

18. Revised Map Editor

Restore the map editor as a major feature.

Make it substantially better than the original concept.

The editor should allow:

* path creation
* waypoint editing
* tower placement regions
* tower blocking regions
* spawn points
* exit points
* map background
* decorative objects
* object scaling
* object rotation
* layer ordering
* map metadata
* preview mode
* test mode
* undo/redo
* copy/paste
* grid snapping
* zoom
* pan
* save/load
* validation

Editor layout:

+------------------------------------------------------+
| File Edit View Map Test                              |
+--------------------+---------------------------------+
| Tools              |                                 |
| Select             |                                 |
| Path               |       MAP CANVAS               |
| Build Area         |                                 |
| Spawn              |                                 |
| Exit               |                                 |
| Object             |                                 |
|                    |                                 |
+--------------------+---------------------------------+
| Properties         | Timeline / Layers / Objects    |
+--------------------+---------------------------------+

The map editor should use the same renderer and asset system as the game wherever practical.

Allow pressing a button to immediately test the current map in the game engine.

⸻

19. Achievements

Implement a complete achievement framework.

Example:

Achievement
├── ID
├── name
├── description
├── icon
├── requirement
├── progress
└── unlocked

Support:

* single-event achievements
* cumulative achievements
* progress achievements
* hidden achievements
* unlock notifications

Examples of achievement categories:

Tower usage
Bloon popping
Round completion
Map completion
Challenge conditions
Economy
Special towers
Map editor
Multiplayer

Achievements should be data-driven.

Platform backends may optionally integrate with platform achievement systems.

For platforms without native achievement services, maintain local achievements.

⸻

20. Save System

Use a platform-independent save format.

Save:

settings
progress
unlocks
achievements
statistics
custom maps

Use versioned save files.

Implement migration when save formats change.

⸻

21. Mod/Custom Content Support

Design the internal format so custom content can eventually be loaded.

Example:

mods/
├── mod.json
├── textures/
├── maps/
├── towers/
├── bloons/
└── sounds/

Do not allow arbitrary native code plugins by default.

Prefer data-driven modifications for security and portability.

⸻

22. Game Builder Platform Selection

The Game Builder must allow selecting:

Windows
Linux
PSP
Xbox 360

and future targets.

The platform list should be generated from registered platform backends rather than hardcoded throughout the UI.

Example:

PlatformBackend
├── name()
├── isAvailable()
├── configure()
├── build()
├── package()
└── validate()

Example UI:

Target Platform:
[x] Windows
[ ] Linux
[ ] PSP
[ ] Xbox 360

Unavailable toolchains should be clearly identified.

Example:

PSP
✓ Toolchain detected
Xbox 360
! Toolchain unavailable
Install/configure the required official development environment.

Never silently fail.

⸻

23. Windows Build

Windows builds must be native.

Do NOT use:

* Electron
* web UI
* browser runtime
* WebAssembly
* HTML application wrappers

The Windows game should compile to a normal native executable.

The Game Builder should also compile to a normal native executable.

⸻

24. Linux Build

Provide native Linux builds.

Prefer:

AppImage

or another practical portable package.

Also support building the native executable directly.

⸻

25. PSP Build

PSP target:

EBOOT.PBP

Use PSPSDK/PSPDEV.

Optimize:

* memory
* texture formats
* draw calls
* CPU usage
* object allocation
* audio memory

Provide a PSP-specific package layout.

⸻

26. Xbox 360 Port

Treat Xbox 360 as a later platform.

Do NOT attempt to implement it during the initial engine development.

The architecture must nevertheless reserve:

platform/xbox360/

for the backend.

Xbox 360 support should eventually include:

* native rendering backend
* controller support
* filesystem/save backend
* audio backend
* achievement backend
* networking
* packaging

Use the appropriate legitimate Xbox 360 development tools and SDKs.

Do not distribute proprietary Microsoft SDK components.

GitHub Actions should only build Xbox 360 when the required legitimate toolchain is available.

⸻

27. Multiplayer

Multiplayer is a final-stage feature.

Do not begin networking until the single-player simulation is deterministic and stable.

Design the simulation so that multiplayer can eventually support:

2 players
co-op tower defense
shared map
shared bloon waves
shared or separate cash
player-specific towers

Potential architecture:

Player 1
    |
Player 2
    |
    v
Network Layer
    |
Authoritative Simulation
    |
Game State

Prefer deterministic simulation and compact network messages.

Do not synchronize entire game frames.

Synchronize game events/state changes.

Potential transport:

LAN
Internet

Start with LAN multiplayer.

Then implement online networking.

Multiplayer should eventually work across compatible desktop platforms.

Cross-platform PSP multiplayer may be considered later depending on PSP networking limitations.

⸻

28. GitHub Actions

Create GitHub Actions workflows.

Required:

.github/workflows/
├── build-windows.yml
├── build-linux.yml
├── build-psp.yml
├── build-all.yml
└── release.yml

Windows workflow:

checkout
↓
install dependencies
↓
configure CMake
↓
build
↓
run tests
↓
package
↓
upload artifact

Linux workflow:

checkout
↓
install dependencies
↓
configure
↓
build
↓
test
↓
package
↓
upload artifact

PSP workflow:

checkout
↓
install/use PSPDEV toolchain
↓
build
↓
validate EBOOT.PBP
↓
upload artifact

Xbox 360 workflow:

Only run when the required legitimate toolchain is available.

Do not place proprietary SDKs in GitHub.

⸻

29. GitHub Release

A release should optionally contain:

Windows build
Linux build
PSP EBOOT
Game Builder
Asset Importer
Map Editor
Documentation

Do NOT include:

original BTD4 SWF
original BTD4 APK
original BTD4 IPA
copyrighted BTD4 assets

The user imports their own source files.

⸻

30. Game Builder Project Files

The builder should save project configuration.

Example:

project.btd4proj

Containing:

{
    "version": 1,
    "source_swf": "",
    "source_ipa": "",
    "mobile_content": true,
    "target_platform": "psp",
    "build_configuration": "release"
}

Do not embed proprietary source files into the project file.

Store paths/references instead.

⸻

31. Build Pipeline

The Game Builder should perform:

Load Project
     ↓
Validate Sources
     ↓
Import SWF
     ↓
Import IPA if provided
     ↓
Merge/Normalize Data
     ↓
Generate Internal Game Package
     ↓
Validate Content
     ↓
Select Platform
     ↓
Configure Platform
     ↓
Compile
     ↓
Package
     ↓
Output Build

Show progress in the UI.

Example:

[1/8] Loading project
[2/8] Importing SWF
[3/8] Importing IPA
[4/8] Converting assets
[5/8] Validating content
[6/8] Compiling
[7/8] Packaging
[8/8] Complete

⸻

32. Error Handling

Never crash on a malformed source file.

Display useful errors.

Example:

ERROR
The selected IPA could not be parsed.
Reason:
Unsupported bundle structure.
The SWF content can still be used.

For missing assets:

WARNING
12 textures could not be converted.
The build can continue, but affected content may be unavailable.

⸻

33. Security

Treat imported files as untrusted input.

The importer must:

* avoid arbitrary code execution
* validate file sizes
* validate archive paths
* prevent path traversal
* avoid blindly executing extracted files
* use bounded parsing
* reject malformed structures safely

Never execute code extracted from an SWF, APK or IPA merely because it was found in the file.

⸻

34. Testing

Create automated tests for:

SWF parser
IPA parser
asset conversion
manifest generation
texture loading
audio loading
map parsing
tower data
bloon movement
projectiles
targeting
rounds
economy
upgrades
achievements
save/load
map editor serialization
builder project files

Create test fixtures using original/non-copyrighted sample data.

Do not commit proprietary BTD4 files as test fixtures.

⸻

35. Performance Testing

Create benchmarks for:

100 bloons
500 bloons
1000 bloons
100 towers
500 projectiles
large maps

Measure:

FPS
CPU time
memory usage
draw calls
entity count
allocation count

Add profiling support to development builds.

⸻

36. Development Phases

Phase 1: Infrastructure

Build:

* repository
* CMake
* CI
* shared engine
* platform abstraction
* desktop build
* PSP build

Goal:

Windows launches.
Linux launches.
PSP EBOOT launches.

⸻

Phase 2: Rendering/Input

Implement:

* renderer
* textures
* sprites
* input
* logical resolution
* debug overlay

⸻

Phase 3: Core Gameplay

Implement:

* maps
* paths
* bloons
* towers
* projectiles
* damage
* lives
* money
* rounds

Goal:

A completely playable basic tower-defense game.

⸻

Phase 4: Full Game Systems

Implement:

* upgrades
* special abilities
* targeting
* UI
* menus
* saving
* audio

⸻

Phase 5: Achievements

Implement the complete achievement framework.

⸻

Phase 6: Asset Importer

Implement:

* SWF extraction
* asset conversion
* data conversion
* IPA extraction
* mobile content detection
* manifest generation

⸻

Phase 7: Game Builder

Implement:

* native UI
* project management
* source selection
* asset importing
* platform selection
* build configuration
* build logs
* output packaging

⸻

Phase 8: Revised Map Editor

Implement:

* map creation
* path editor
* build areas
* objects
* layers
* undo/redo
* preview
* testing
* save/load

⸻

Phase 9: Optimization

Profile all platforms.

Optimize PSP aggressively.

Optimize desktop builds.

Remove unnecessary allocations.

Improve rendering batching.

Reduce memory usage.

⸻

Phase 10: Multiplayer

Implement:

1. deterministic simulation
2. LAN multiplayer
3. synchronization
4. co-op
5. reconnect handling
6. online multiplayer

⸻

Phase 11: Xbox 360

Implement the Xbox 360 backend.

Port:

* rendering
* input
* audio
* saves
* achievements
* networking

Then optimize specifically for Xbox 360 hardware.

⸻

37. Important Legal/Technical Boundary

The project must be a native engine and conversion tool.

Do not distribute proprietary game source code or proprietary game assets.

Do not automatically download copyrighted game files.

Do not include copyrighted assets in GitHub Actions artifacts.

Do not circumvent DRM or platform security mechanisms.

The user is responsible for providing source files they are legally entitled to use.

The project should remain useful with entirely original assets.

⸻

38. Codex Instructions

Codex must work incrementally.

Never attempt to implement the entire project in one response or commit.

For every task:

1. Inspect the repository.
2. Determine the current architecture.
3. Implement one logical feature.
4. Compile it.
5. Run tests.
6. Fix compilation errors.
7. Fix test failures.
8. Update documentation.
9. Keep the changes modular.

Never replace functioning code with a large rewrite without justification.

Do not introduce a heavyweight dependency when a lightweight implementation is sufficient.

Performance takes priority over convenience in the runtime engine.

The Game Builder may prioritize usability, but it must remain native and lightweight.

⸻

39. FIRST CODEX TASK

Start by creating the project infrastructure only.

Implement:

1. Repository structure.
2. CMake build system.
3. Desktop Windows build.
4. Desktop Linux build.
5. PSP toolchain configuration.
6. GitHub Actions for Windows/Linux/PSP.
7. Shared engine library.
8. Platform abstraction.
9. Basic renderer.
10. Basic input system.
11. 480x272 logical resolution.
12. Test screen.
13. FPS counter.
14. Basic logging.
15. Automated tests.
16. Initial native Game Builder application using Dear ImGui.
17. Platform selection screen in the builder.
18. Project file format.
19. Initial SWF/IPA source-file selection UI.
20. Build log panel.

Do NOT implement the actual game content yet.

Do NOT include copyrighted BTD4 assets.

Do NOT attempt multiplayer yet.

Do NOT attempt Xbox 360 implementation yet.

The first milestone is:

Windows:
Game launches.
Builder launches.
Tests pass.
Linux:
Game builds.
Builder builds.
PSP:
EBOOT.PBP builds.
GitHub:
Windows/Linux/PSP CI succeeds.

Only after this milestone is working should gameplay implementation begin.
