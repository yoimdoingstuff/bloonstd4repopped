TODO.md

Legend:

* [ ]	Not started
* [~] In progress
* [x]	Complete
* [!]	Blocked

Every blocked task must include a Blocked by: entry and a Reasoning: entry.
The reasoning must state the concrete dependency or condition that prevented
completion, the checks performed, and what is needed to unblock the task.

Agents should claim a task before working on it.

⸻

MILESTONE 1: PROJECT INFRASTRUCTURE

Build System

* [x]	Create root CMake project
    Agent: project-infrastructure
* [x]	Create CMake presets
    Agent: project-infrastructure
* [x]	Create shared engine target
    Agent: project-infrastructure
* [x]	Create desktop executable
    Agent: project-infrastructure
* [x]	Create Game Builder executable
    Agent: project-infrastructure
* [x]	Create PSP toolchain configuration
    Agent: project-infrastructure
* [x]	Create Xbox 360 placeholder backend
    Agent: project-infrastructure

GitHub Actions

* [x]	Windows CI
    Agent: project-infrastructure
* [x]	Linux CI
    Agent: project-infrastructure
* [x]	PSP CI
    Agent: project-infrastructure
* [x]	Combined build workflow
    Agent: project-infrastructure
* [x]	Artifact packaging
    Agent: project-infrastructure
* [x]	Release workflow
    Agent: project-infrastructure

Testing

* [x]	Create test framework
    Agent: project-infrastructure
* [x]	Add core engine tests
    Agent: project-infrastructure
* [x]	Add CI test execution
    Agent: project-infrastructure

⸻

MILESTONE 2: PLATFORM ABSTRACTION

Core

* [x]	Platform abstraction
    Agent: Codex / Antigravity
* [x]	Filesystem abstraction
    Agent: Codex / Antigravity
* [x]	Timing abstraction
    Agent: platform-abstraction / Antigravity
* [x]	Logging abstraction
    Agent: platform-abstraction / Antigravity
* [x]	Input abstraction
    Agent: Codex / Antigravity
* [x]	Audio abstraction
    Agent: Codex / Antigravity
* [x]	Renderer abstraction
    Agent: Codex / Antigravity

Windows

* [x]	Windows renderer
    Agent: Codex / Antigravity
* [x]	Windows input
    Agent: Codex / Antigravity
* [x]	Windows audio
    Agent: platform-abstraction / Antigravity
* [x]	Windows filesystem
    Agent: Codex / Antigravity

Linux

* [x]	Linux renderer
    Agent: Codex / Antigravity
* [x]	Linux input
    Agent: Codex / Antigravity
* [x]	Linux audio
    Agent: platform-abstraction / Antigravity
* [x]	Linux filesystem
    Agent: Codex / Antigravity

PSP

* [ ]	PSP renderer
    Notes: Local PSP cross-toolchain verified; implementation and runtime validation remain.
* [ ]	PSP input
    Notes: Local PSP cross-toolchain verified; implementation and runtime validation remain.
* [ ]	PSP audio
    Notes: Local PSP cross-toolchain verified; implementation and runtime validation remain.
* [ ]	PSP filesystem
    Notes: Local PSP cross-toolchain verified; implementation and runtime validation remain.
* [ ]	PSP memory utilities
    Notes: Local PSP cross-toolchain verified; implementation and runtime validation remain.
* [x]	PSP packaging
    Agent: Codex - minimal native PSP bootstrap and verified EBOOT packaging
    Validation: PSP GCC 15.2.0 cross-build and EBOOT structure test passed on Windows; corrupt/missing package checks passed.
    Notes: Bootstrap only; PPSSPP/hardware and GitHub CI not run. See docs/psp-build.md.

Xbox 360

* [x]	Backend skeleton only
    Agent: Codex / Antigravity

⸻

MILESTONE 3: RENDERING

* [x]	Texture system
	Agent: rendering / Antigravity
* [x]	Sprite system
	Agent: rendering / Antigravity
* [x]	Sprite batching
	Agent: rendering / Antigravity
* [x]	Texture atlas support
	Agent: rendering / Antigravity
* [x]	Font rendering
	Agent: rendering / Antigravity
* [x]	Basic shapes
	Agent: rendering / Antigravity
* [x]	Camera
	Agent: rendering / Antigravity

Display & Resolution

* [x]	Logical resolution system
	Agent: rendering / Antigravity
* [x]	Multiple selectable logical resolutions
	Agent: rendering / Antigravity
* [x]	Native display resolution detection
	Agent: rendering / Antigravity
* [x]	Resolution configuration
	Agent: rendering / Antigravity
* [x]	Aspect-ratio handling
	Agent: rendering / Antigravity
* [x]	Letterboxing/pillarboxing
	Agent: rendering / Antigravity
* [x]	Integer scaling mode
	Agent: rendering / Antigravity
* [x]	Fractional scaling mode
	Agent: rendering / Antigravity
* [x]	Fullscreen scaling
	Agent: rendering / Antigravity
* [x]	Windowed scaling
	Agent: rendering / Antigravity
* [x]	Per-platform display profiles
	Agent: rendering / Antigravity
* [x]	PSP 480x272 display profile
	Agent: rendering / Antigravity
* [x]	Desktop 16:9 profiles
	Agent: rendering / Antigravity
* [x]	Desktop 4:3 profiles
	Agent: rendering / Antigravity
* [x]	HD resolution profiles
	Agent: rendering / Antigravity
* [x]	iPad/HD source resolution profiles
	Agent: rendering / Antigravity
* [x]	Custom resolution support
	Agent: rendering / Antigravity
* [x]	Resolution-safe UI layout
	Agent: rendering / Antigravity
* [x]	Resolution testing/debug overlay
	Agent: rendering / Antigravity
* [x]	Debug renderer
	Agent: rendering / Antigravity
* [x]	FPS counter
	Agent: rendering / Antigravity

⸻

MILESTONE 4: INPUT

* [x]	Keyboard input
    Agent: platform-abstraction / builder
* [x]	Mouse input
    Agent: platform-abstraction / builder
* [ ]	PSP controls
* [ ]	PSP analog input
* [ ]	Xbox controller abstraction
* [ ]	Input mapping
* [ ]	Rebindable controls

Controller System

* [ ]	Controller enumeration
* [ ]	Controller connection/disconnection detection
* [ ]	Controller assignment
* [x]	Player-to-controller mapping
    Agent: multiplayer
    Notes: MultiplayerSession assigns SDL-compatible controller instance IDs to active player slots with duplicate-assignment checks.
* [ ]	Per-player input state
* [ ]	Multiple simultaneous controllers
* [ ]	Controller configuration
* [ ]	Controller hot-plug support
* [ ]	Controller vibration abstraction
* [ ]	Local multiplayer input testing

PSP Controller Support

* [ ]	PSP controller backend
* [ ]	PSP multi-controller support
* [ ]	PSP controller adapter support
* [ ]	PSP player assignment
* [ ]	PSP controller compatibility testing

⸻

MILESTONE 5: CORE GAME

Game State

* [x]	Game state machine
    Agent: Antigravity
* [x]	Main menu state
    Agent: Antigravity
* [x]	Gameplay state
    Agent: Antigravity
* [x]	Pause state
    Agent: Antigravity
* [x]	Game over state
    Agent: Antigravity
* [x]	Victory state
    Agent: Antigravity

Maps

* [x]	Map data structure
    Agent: Antigravity
* [x]	Path data structure
    Agent: Antigravity
* [x]	Waypoints
    Agent: Antigravity
* [x]	Spawn points
    Agent: Antigravity
* [x]	Exit points
    Agent: Antigravity
* [x]	Buildable regions
    Agent: Antigravity
* [x]	Blocked regions
    Agent: Antigravity
* [x]	Map loader
    Agent: Codex - versioned internal map loading
    Validation: Windows portable Zig C++17 build; 5 new map suites and 7 existing simulation suites passed.
    Notes: See docs/map-format.md. Full SDL/CMake application build, Linux and PSP not tested in this task.
* [x]	Map validator
    Agent: Antigravity

Bloons

* [x]	Base bloon entity
    Agent: Antigravity
* [x]	Movement
    Agent: Antigravity
* [x]	Path following
    Agent: Antigravity
* [x]	Health
    Agent: Antigravity
* [x]	Popping
    Agent: Antigravity
* [x]	Leaking
    Agent: Antigravity
* [x]	Child bloons
    Agent: Antigravity
* [x]	Special behaviour
    Agent: Antigravity
* [x]	Object pooling
    Agent: Antigravity

Towers

* [x]	Tower entity
    Agent: Antigravity
* [x]	Placement
    Agent: Antigravity
* [x]	Range
    Agent: Antigravity
* [x]	Targeting
    Agent: Antigravity
* [x]	First targeting
    Agent: Antigravity
* [x]	Last targeting
    Agent: Antigravity
* [x]	Close targeting
    Agent: Antigravity
* [x]	Strong targeting
    Agent: Antigravity
* [x]	Attack cooldown
    Agent: Antigravity

Projectiles

* [x]	Projectile entity
    Agent: Antigravity
* [x]	Movement
    Agent: Antigravity
* [x]	Collision
    Agent: Antigravity
* [x]	Damage
    Agent: Antigravity
* [x]	Pierce
    Agent: Antigravity
* [x]	Area damage
    Agent: Antigravity
* [x]	Projectile pooling
    Agent: Antigravity

⸻

MILESTONE 6: ECONOMY

* [x]	Cash
    Agent: Antigravity
* [x]	Lives
    Agent: Antigravity
* [x]	Tower costs
    Agent: Antigravity
* [x]	Upgrade costs
    Agent: builder
    Notes: Upgrade purchase costs are read from UpgradeDefinition data and deducted from the game economy.
* [x]	Sell values
    Agent: Antigravity
* [x]	Round rewards
    Agent: Antigravity
* [x]	Economy tests
    Agent: Antigravity

⸻

MILESTONE 7: UPGRADES

* [x]	Upgrade data format
    Agent: builder
    Notes: JSON upgrade definitions are parsed into validated UpgradeDefinition records.
* [x]	Upgrade loading
    Agent: builder
    Notes: Runtime loads imported upgrade data when present, otherwise the bundled placeholder set.
* [x]	Upgrade UI
    Agent: builder
    Notes: Selected towers expose both upgrade paths with mouse-click and desktop Q/E controls.
* [x]	Stat modifications
    Agent: builder
    Notes: UpgradeEffect values are applied to range, cooldown, damage, pierce, projectile speed, and explosion radius.
* [x]	Multiple upgrade paths
    Agent: builder
    Notes: Two independent upgrade paths are supported and tier progression is validated.
* [x]	Upgrade validation
    Agent: builder
    Notes: Duplicate IDs and tower/path/tier entries plus invalid effect values are rejected.

⸻

MILESTONE 8: ROUNDS

* [x]	Round data format
    Agent: Codex - data-driven rounds
* [x]	Round loader
    Agent: Codex - data-driven rounds
* [x]	Bloon groups
    Agent: Codex - data-driven rounds
* [x]	Spawn timing
    Agent: Codex - data-driven rounds
* [x]	Round completion
    Agent: Codex - data-driven rounds
* [x]	Round rewards
    Agent: Codex - data-driven rounds
* [x]	Freeplay framework
    Agent: Codex - deterministic freeplay generator
    Validation: Generator tests cover invalid input, campaign cycling, and progressive spawn-pressure scaling.

Round validation: Windows portable Zig C++17 build; all 19 map, simulation and round suites passed.
See docs/round-format.md. Full SDL/CMake builds, Linux and PSP were not tested.

⸻

MILESTONE 9: UI

Core & Abstraction

* [ ]	UIAdapter interface
* [ ]	UILayout configuration
* [ ]	Graphics Style loading
* [ ]	Resolution-independent UI coordinates
* [ ]	UI scaling
* [ ]	Aspect-ratio-safe UI
* [ ]	Controller navigation abstraction
* [ ]	Per-player UI support

Platform Backends

* [ ]	Desktop PlatformUI (Mouse)
* [ ]	Desktop PlatformUI (Keyboard/Gamepad)
* [ ]	PSP PlatformUI (D-pad/Buttons)
* [ ]	Xbox 360 PlatformUI (Gamepad)

Screens & Panels

* [ ]	Main menu
* [x]	Tower selection (Mobile-port style sidebar)
	Agent: ui / Antigravity
* [ ]	Tower information
* [ ]	Upgrade panel
* [x]	HUD (Cash, lives, round)
	Agent: ui / Antigravity
* [ ]	Pause menu
* [ ]	Victory screen
* [ ]	Game over screen
* [ ]	Options menu
* [ ]	Resolution selection
* [ ]	Display settings
* [ ]	Controller settings
* [ ]	Multiplayer lobby/player selection

⸻

MILESTONE 10: AUDIO

* [ ]	Audio abstraction
* [ ]	Sound effect loader
* [ ]	Music loader
* [ ]	Sound playback
* [ ]	Music playback
* [ ]	Volume settings
* [ ]	Audio caching
* [ ]	PSP audio optimization

⸻

MILESTONE 11: ACHIEVEMENTS

* [ ]	Achievement data format
* [ ]	Achievement manager
* [ ]	Achievement progress
* [ ]	Achievement unlocking
* [ ]	Achievement notifications
* [ ]	Hidden achievements
* [ ]	Local achievement storage
* [ ]	Platform achievement abstraction

⸻

MILESTONE 12: SAVE SYSTEM

* [ ]	Save format
* [ ]	Save versioning
* [ ]	Save/load manager
* [ ]	Settings saves
* [ ]	Progress saves
* [ ]	Achievement saves
* [ ]	Custom map saves
* [ ]	Migration system

⸻

MILESTONE 13: ASSET IMPORTER

General

* [x]	Input detection
	Agent: asset-importer / Antigravity
* [x]	Source version detection
	Agent: asset-importer / Antigravity
* [x]	Internal asset format
	Agent: asset-importer / Antigravity
* [x]	Asset manifest
	Agent: asset-importer / Antigravity
* [x]	Placeholder asset loading (fallback)
	Agent: asset-importer / Antigravity
* [x]	Placeholder JSON data authoring (1:1 accuracy)
	Agent: asset-importer / Antigravity
* [x]	Validation
	Agent: asset-importer / Antigravity
* [x]	Conversion pipeline
	Agent: asset-importer / Antigravity
* [x]	Error reporting
	Agent: asset-importer / Antigravity
* [x]	Duplicate asset detection
	Agent: asset-importer / Antigravity
* [x]	Asset dependency tracking
	Agent: asset-importer / Antigravity
* [x]	Asset version tracking
	Agent: asset-importer / Antigravity
* [x]	Source compatibility report
	Agent: asset-importer / Antigravity
* [x]	Imported content manifest
	Agent: asset-importer / Antigravity

Flash / SWF

* [x]	SWF parser
	Agent: asset-importer / Antigravity
* [x]	Bitmap extraction
	Agent: asset-importer / Antigravity
* [x]	Vector extraction
	Agent: asset-importer / Antigravity
* [x]	Sprite extraction
	Agent: asset-importer / Antigravity
* [x]	Animation extraction
	Agent: asset-importer / Antigravity
* [x]	Audio extraction
	Agent: asset-importer / Antigravity
* [x]	Data extraction
	Agent: asset-importer / Antigravity
* [x]	Font extraction
	Agent: asset-importer / Antigravity
* [x]	Metadata extraction
	Agent: asset-importer / Antigravity
* [x]	Version detection
	Agent: asset-importer / Antigravity
* [x]	Flash game data identification
	Agent: asset-importer / Antigravity

Bloons TD 4 Flash

* [ ]	BTD4 Flash version detection
* [ ]	BTD4 Flash asset identification
* [ ]	BTD4 Flash tower data identification
* [ ]	BTD4 Flash bloon data identification
* [ ]	BTD4 Flash map identification
* [ ]	BTD4 Flash round data identification
* [ ]	BTD4 Flash upgrade data identification
* [ ]	BTD4 Flash UI asset identification

Bloons TD 4 Expansion

* [ ]	Expansion source identification
* [ ]	Expansion asset identification
* [ ]	Expansion map identification
* [ ]	Expansion tower/data identification
* [ ]	Expansion round/data identification

Bloons TD 4 HD / iPad

* [ ]	HD source identification
* [ ]	HD asset identification
* [ ]	HD map identification
* [ ]	HD tower/data identification
* [ ]	HD round/data identification
* [ ]	HD UI identification

IPA

* [ ]	IPA archive detection
* [ ]	IPA extraction
* [ ]	IPA resource discovery
* [ ]	IPA metadata extraction
* [ ]	IPA source manifest
* [ ]	IPA-to-internal conversion

⸻

MILESTONE 13A: DEFINITIVE EDITION ASSET FUSION

* [x] Multi-source project configuration for Flash, Expansion, phone/mobile and HD/iPad inputs
* [x] Automatic phone/mobile and HD IPA source-layer extraction
* [x] Definitive Edition builder mode
* [x] Target-aware HD/mobile duplicate asset selection
* [x] Expansion SWF layer import
* [x] Runtime manifest target-platform metadata
* [ ] Automatic source-version identification from actual package contents
    Notes: Current source slots are explicit; deeper package fingerprinting remains.
* [ ] Resolution-aware map selection
    Notes: Maps need explicit source identity and quality metadata rather than filename-only discovery.
* [ ] Mobile asset upscaling pipeline
    Notes: Needs a real image decoder/upscaler and quality validation. Native HD assets should always take precedence.
* [ ] Full four-source data merge for towers, rounds, upgrades and maps
    Notes: Texture/resource fusion is implemented first; structured game-data merging still needs source-specific parsers and conflict rules.

⸻

⸻

MILESTONE 14: GAME BUILDER

* [x]	Project creation
    Agent: builder
    Notes: New Project resets project settings, editor state, and import fingerprints.
* [x]	Project loading
    Agent: builder
    Notes: Builder automatically loads project.btd4proj on startup and exposes a Load Project action.
* [x]	Project saving
    Agent: builder
* [x]	Asset import UI
    Agent: builder - native file picker, Explorer/file-manager drag-and-drop, SWF/IPA paths
* [x]	Source edition selector
    Agent: builder - BTD4 Flash / BTD4 Expansion / BTD4 HD (iPad), isolated output directories
* [x]	Platform selector
    Agent: builder
* [x]	Build configuration
    Agent: builder
    Notes: Platform backends configure isolated CMake build directories from imported source data.
* [x]	Build invocation
    Agent: builder
    Notes: Builder runs configure, compile, then package through the selected platform backend.
* [x]	Build output management
    Agent: builder
    Notes: Windows and Linux backends create self-contained Playable directories.
* [x]	Project validation
    Agent: builder
    Notes: Builder validates source files, edition, target platform, and build configuration before compilation.
* [x]	Map editor
    Agent: builder
    Notes: Integrated visual map editor saves to the versioned internal map schema.
* [x]	Round editor
    Agent: builder
    Notes: Builder edits bloon groups, timing, and path indexes and saves validated custom_rounds.json.
* [x]	Tower editor
    Agent: builder
    Notes: Builder edits base tower cost, range, cooldown, projectile, damage, pierce, speed, and explosion values and saves custom_towers.json.

* [x]	Upgrade editor
    Agent: builder
    Notes: Builder edits upgrade identity, path/tier, cost, and stat modifiers and saves custom_upgrades.json.

* [x]	Preview mode
    Agent: builder
    Notes: Builder can launch packaged Windows/Linux Playable builds directly.

⸻

MILESTONE 15: MAP EDITOR

* [x]	Map editor core
    Agent: builder
* [x]	Map canvas
    Agent: builder
* [x]	Waypoint editing
    Agent: builder
* [x]	Path editing
    Agent: builder
* [ ]	Spawn editing
* [ ]	Exit editing
* [x]	Buildable region editing
    Agent: builder
* [x]	Blocked region editing
    Agent: builder
* [x]	Map validation UI
    Agent: builder
* [x]	Map preview
    Agent: builder
* [x]	Map save/load
    Agent: builder
* [x]	Custom map packaging
    Agent: builder
    Notes: Builder-saved maps are copied into Playable/game_data/maps and loaded by the runtime before the built-in fallback map.

⸻

MILESTONE 16: MULTIPLAYER

* [x]	Local multiplayer framework
    Agent: multiplayer
    Notes: Local session supports 1-4 players, explicit session lifecycle, and shared/split economy policy.
* [x]	Player abstraction
    Agent: multiplayer
    Notes: Four player slots with stable IDs and display names are available.
* [x]	Player state
    Agent: multiplayer
    Notes: Each player has an active flag and independent Economy state; player 0 remains the single-player compatibility slot.
* [x]	Multiple tower ownership
    Agent: multiplayer
    Notes: Towers carry an owner player ID; placement and selling use the owning player's economy.
* [ ]	Shared economy mode
* [ ]	Split economy mode
* [ ]	Local multiplayer UI
* [ ]	Network abstraction
* [ ]	Online session framework
* [ ]	Host/join
* [ ]	Session discovery
* [ ]	State synchronization
* [ ]	Prediction/interpolation
* [ ]	Desync detection
* [ ]	Reconnect support

⸻

MILESTONE 17: XBOX 360

* [x]	Xbox 360 backend skeleton
	Agent: Codex / Antigravity
* [ ]	Xbox 360 renderer
* [ ]	Xbox 360 input
* [ ]	Xbox 360 audio
* [ ]	Xbox 360 filesystem
* [ ]	Xbox 360 packaging
* [ ]	Xbox 360 deployment

⸻

MILESTONE 17A: MOBILE / HANDHELD TARGETS

PlayStation Vita

* [x] Vita backend skeleton
    Notes: Registered as a future target only, matching the Xbox 360 backend stage.
* [ ] Vita renderer
* [ ] Vita input
* [ ] Vita audio
* [ ] Vita filesystem
* [ ] Vita packaging
* [ ] Vita deployment
* [ ] Vita hardware / Vita3K validation

Android

* [x] Android backend skeleton
    Notes: Registered as a future target only. The planned compatibility floor is Android 4.0-class devices.
* [ ] Android renderer
* [ ] Android input
* [ ] Android audio
* [ ] Android filesystem
* [ ] Android packaging
* [ ] Android 4.0 compatibility validation
* [ ] Modern Android compatibility validation
* [ ] APK / package signing

iOS

* [x] iOS backend skeleton
    Notes: Registered as a future target only. Planned deployment includes jailbroken devices and legacy iOS 9-or-earlier targets.
* [ ] iOS renderer
* [ ] iOS input
* [ ] iOS audio
* [ ] iOS filesystem
* [ ] iOS packaging
* [ ] Jailbroken-device deployment
* [ ] iOS 9-or-earlier compatibility validation
* [ ] Legacy signing / installation workflow

⸻

⸻

MILESTONE 18: PSP

* [ ]	PSP renderer implementation
* [ ]	PSP input implementation
* [ ]	PSP audio implementation
* [ ]	PSP filesystem implementation
* [ ]	PSP memory utilities
* [x]	PSP packaging
	Agent: Codex
* [ ]	PSP runtime validation
* [ ]	PPSSPP testing
* [ ]	Hardware testing

⸻

MILESTONE 19: QUALITY

* [ ]	Performance profiling
* [ ]	Memory profiling
* [ ]	Leak detection
* [ ]	Crash handling
* [ ]	Error recovery
* [ ]	Logging improvements
* [ ]	Debug tools
* [ ]	Automated regression testing
* [ ]	Cross-platform testing
* [ ]	Asset compatibility testing

⸻

MILESTONE 20: POLISH

* [ ]	Animation polish
* [ ]	Particle effects
* [ ]	Visual effects
* [ ]	UI polish
* [ ]	Sound polish
* [ ]	Music integration
* [ ]	Tutorial
* [ ]	Help system
* [ ]	Accessibility
* [ ]	Localization
* [ ]	Credits
* [ ]	About screen

⸻

MILESTONE 21: RELEASE

* [ ]	Versioning
* [ ]	Release builds
* [ ]	Installer
* [ ]	Portable package
* [ ]	PSP package
* [ ]	Linux package
* [ ]	Xbox package
* [ ]	Documentation
* [ ]	Licensing review
* [ ]	Final QA
