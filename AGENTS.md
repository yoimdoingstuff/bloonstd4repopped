AGENTS.md

Project

This repository contains a high-performance, cross-platform tower-defense game engine and native Game Builder.

Primary targets:

* Windows
* Linux
* PSP
* Xbox 360
* Future platforms

The project is designed as a native C++ application.

The runtime must not use:

* Electron
* Chromium
* WebViews
* HTML/CSS UI
* JavaScript application frameworks
* browser-based rendering

The Game Builder is also a native desktop application.

Preferred builder UI technology:

* Dear ImGui

⸻

Core Principles

1. Performance First

Performance is a first-class requirement.

Avoid unnecessary:

* allocations
* copies
* virtual calls in hot loops
* draw calls
* texture switches
* synchronization
* expensive per-frame calculations

Prefer:

* object pools
* contiguous memory
* batching
* texture atlases
* precomputed data
* deterministic simulation
* data-oriented structures where useful

The PSP is a constrained platform and must receive specific optimization work.

Do not optimize blindly. Profile first whenever possible.

⸻

2. Cross-Platform Architecture

All shared game logic belongs in:

engine/

Platform-specific implementations belong in:

platform/

Never place platform-specific APIs inside shared game logic.

Architecture:

engine/
    |
    +--- platform abstraction
             |
       +-----+-----+------+------+
       |           |             |
    Windows      Linux         PSP
                                 |
                              Xbox 360

The engine must be usable without the original SWF, APK or IPA.

⸻

3. Source Content

The project may provide tools that process user-supplied:

* SWF
* APK
* IPA

The repository must NOT contain proprietary BTD4:

* assets
* sounds
* music
* maps
* source code
* binaries
* original game packages

Do not implement automatic downloading of proprietary game files.

Do not bypass DRM or platform security.

Treat imported files as untrusted input.

⸻

4. Runtime vs Importer

The runtime must NEVER depend on SWF/APK/IPA formats.

The importer converts external data into the project’s internal format.

SWF ──────┐
           │
APK ──────┼──> Importer ──> Internal Game Data ──> Engine
           │
IPA ──────┘

The engine only understands the internal format.

⸻

5. Mobile Content

IPA import is optional.

SWF-only projects must remain functional.

If an IPA is provided, the importer should detect additional mobile content where available.

Possible mobile content includes:

* Beekeeper
* mobile-exclusive maps
* mobile UI
* mobile assets
* mobile achievements
* other version-specific content

Never assume content exists.

The importer must detect it and report what was actually found.

⸻

6. Game Systems

The engine should contain independent modules for:

engine/
├── core/
├── game/
├── rendering/
├── audio/
├── input/
├── assets/
├── save/
├── achievements/
├── multiplayer/
└── map/

Major systems include:

* game state
* towers
* bloons
* projectiles
* rounds
* maps
* economy
* upgrades
* UI
* achievements
* saves
* multiplayer

Keep systems modular.

Avoid giant source files.

⸻

7. Data Driven Design

Game content should be data-driven.

Do not hardcode tower, bloon, map or achievement definitions into the engine when a data format is appropriate.

Example:

Tower
├── stats
├── targeting
├── projectile
├── upgrades
└── abilities

Content should be represented through the internal game data format.

⸻

8. Rendering

Use a renderer abstraction.

Shared engine:

Renderer

Possible backends:

Windows
Linux
PSP
Xbox 360

PSP uses PSPSDK/PSP GU.

The game’s logical resolution is:

480x272

Desktop resolutions should scale the logical coordinate system.

⸻

9. Input

Use a shared input abstraction.

Do not put PSP controller calls directly into game logic.

Platform backends translate platform input into the common input system.

⸻

10. Audio

Use a shared audio abstraction.

Game logic should request:

audio.playSound(id);

rather than directly calling platform audio APIs.

⸻

11. Save System

Use a platform-independent versioned save format.

Platform backends determine where saves are stored.

Save files must support migration when the format changes.

⸻

12. Achievements

Achievements are data-driven.

Support:

* normal achievements
* cumulative achievements
* progress achievements
* hidden achievements
* local achievements
* platform achievements where available

Platform-specific achievement services belong in platform backends.

⸻

13. Map Editor

The map editor shares the same rendering and asset systems as the game where practical.

It must support:

* paths
* waypoints
* buildable regions
* blocked regions
* spawn points
* exits
* objects
* layers
* scaling
* rotation
* grid snapping
* zoom
* pan
* undo/redo
* copy/paste
* preview
* playtest
* validation
* save/load

The editor must be designed as a proper tool, not a hacked-together debug screen.

⸻

14. Game Builder

The Game Builder is a native C++ desktop application.

It must allow:

Select SWF
Select optional IPA
Import assets
Review detected content
Select target platform
Configure build
Build
Package

Supported targets:

Windows
Linux
PSP
Xbox 360
Future platforms

The builder must detect whether the required platform toolchain is available.

Example:

PSP
✓ Toolchain detected
Xbox 360
! Toolchain unavailable

Never claim that a build succeeded unless it actually succeeded.

⸻

15. Platform Backend Interface

New platforms should implement a common interface similar to:

class PlatformBackend
{
public:
    virtual bool isAvailable() const = 0;
    virtual BuildResult configure() = 0;
    virtual BuildResult build() = 0;
    virtual BuildResult package() = 0;
};

Do not scatter platform checks throughout the builder.

⸻

16. GitHub Actions

GitHub Actions must build:

Windows
Linux
PSP

Xbox 360 CI should only run when a legitimate compatible development environment/toolchain is available.

Never commit proprietary SDKs.

Every CI workflow should:

1. checkout
2. install dependencies
3. configure
4. build
5. test
6. package
7. upload artifacts

⸻

17. Testing

Every major system should have tests.

At minimum:

* asset parsing
* asset conversion
* maps
* paths
* towers
* targeting
* projectiles
* bloons
* rounds
* economy
* upgrades
* achievements
* saves
* builder project files

Fix tests when modifying the system they cover.

Never disable tests simply because they fail after a change.

⸻

18. Multiplayer

Multiplayer is a later phase.

The simulation should eventually support deterministic multiplayer.

Start with:

LAN

Then:

online

Do not implement multiplayer networking before the single-player simulation is stable.

⸻

19. Xbox 360

Xbox 360 is a later target.

Do not attempt to implement the full Xbox backend during early development.

Reserve:

platform/xbox360/

for the future backend.

Use legitimate Xbox development tools and SDKs.

Never distribute proprietary Microsoft SDK components.

⸻

20. Agent Rules

Every agent MUST:

1. Read AGENTS.md.
2. Read TODO.md.
3. Read ARCHITECTURE.md when making architectural changes.
4. Check the repository before modifying code.
5. Search for existing implementations before creating new ones.
6. Avoid duplicating functionality.
7. Build/test after making changes.
8. Update TODO.md when completing a task.
9. Document architectural decisions.
10. Keep commits focused.

Agents MUST NOT:

* rewrite unrelated systems
* delete working functionality without justification
* introduce heavyweight dependencies without approval
* implement duplicate systems
* silently change public APIs
* ignore failing tests
* claim untested functionality works
* modify proprietary assets into the repository

⸻

21. Task Ownership

Before starting a task, an agent should mark it:

IN PROGRESS

in TODO.md.

Example:

- [~] Implement PSP renderer
  Agent: renderer-psp

When complete:

- [x] Implement PSP renderer
  Agent: renderer-psp

If blocked:

- [!] Implement PSP renderer
  Agent: renderer-psp
  Blocked by: PSPDEV toolchain configuration

Never have two agents independently implement the same system unless explicitly coordinating.

⸻

22. Shared File Safety

Agents should minimize conflicts.

Before modifying a heavily shared file:

1. Check whether another task is modifying it.
2. Prefer creating a new module over editing a giant shared file.
3. Keep commits small.
4. Avoid formatting unrelated code.

If two tasks conflict, preserve the existing architecture and coordinate through TODO.md and ARCHITECTURE.md.

⸻

23. Definition of Done

A task is not complete merely because code was written.

A task is DONE when:

Implementation
+
Compilation
+
Tests
+
Documentation
+
TODO update

are complete.

If a platform cannot currently be tested, explicitly state that in the task notes.

Never claim successful platform compatibility without testing.

⸻

24. First Milestone

The first milestone is:

Windows
✓ Game builds
✓ Game launches
✓ Builder builds
✓ Builder launches
✓ Tests pass
Linux
✓ Game builds
✓ Builder builds
✓ Tests pass
PSP
✓ EBOOT.PBP builds
GitHub Actions
✓ Windows
✓ Linux
✓ PSP

Do not begin multiplayer or Xbox implementation before the core engine is stable.
