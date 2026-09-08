TODO.md

Legend:

* [ ]	Not started
* [~] In progress
* [x]	Complete
* [!] Blocked

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

* [ ]	Texture system
* [ ]	Sprite system
* [ ]	Sprite batching
* [ ]	Texture atlas support
* [ ]	Font rendering
* [ ]	Basic shapes
* [ ]	Camera

Display & Resolution

* [ ]	Logical resolution system
* [ ]	Multiple selectable logical resolutions
* [ ]	Native display resolution detection
* [ ]	Resolution configuration
* [ ]	Aspect-ratio handling
* [ ]	Letterboxing/pillarboxing
* [ ]	Integer scaling mode
* [ ]	Fractional scaling mode
* [ ]	Fullscreen scaling
* [ ]	Windowed scaling
* [ ]	Per-platform display profiles
* [ ]	PSP 480x272 display profile
* [ ]	Desktop 16:9 profiles
* [ ]	Desktop 4:3 profiles
* [ ]	HD resolution profiles
* [ ]	iPad/HD source resolution profiles
* [ ]	Custom resolution support
* [ ]	Resolution-safe UI layout
* [ ]	Resolution testing/debug overlay
* [ ]	Debug renderer
* [ ]	FPS counter

⸻

MILESTONE 4: INPUT

* [ ]	Keyboard input
* [ ]	Mouse input
* [ ]	PSP controls
* [ ]	PSP analog input
* [ ]	Xbox controller abstraction
* [ ]	Input mapping
* [ ]	Rebindable controls

Controller System

* [ ]	Controller enumeration
* [ ]	Controller connection/disconnection detection
* [ ]	Controller assignment
* [ ]	Player-to-controller mapping
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
* [ ]	Upgrade costs
* [x]	Sell values
    Agent: Antigravity
* [x]	Round rewards
    Agent: Antigravity
* [x]	Economy tests
    Agent: Antigravity

⸻

MILESTONE 7: UPGRADES

* [ ]	Upgrade data format
* [ ]	Upgrade loading
* [ ]	Upgrade UI
* [ ]	Stat modifications
* [ ]	Multiple upgrade paths
* [ ]	Upgrade validation

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
* [ ]	Freeplay framework

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
* [ ]	Map selection
* [ ]	Tower selection (Mobile-port style sidebar)
* [ ]	Tower information
* [ ]	Upgrade panel
* [ ]	HUD (Cash, lives, round)
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

* [ ]	Input detection
* [ ]	Source version detection
* [ ]	Internal asset format
* [ ]	Asset manifest
* [ ]	Placeholder asset loading (fallback)
* [ ]	Placeholder JSON data authoring (1:1 accuracy)
* [ ]	Validation
* [ ]	Conversion pipeline
* [ ]	Error reporting
* [ ]	Duplicate asset detection
* [ ]	Asset dependency tracking
* [ ]	Asset version tracking
* [ ]	Source compatibility report
* [ ]	Imported content manifest

Flash / SWF

* [ ]	SWF parser
* [ ]	Bitmap extraction
* [ ]	Vector extraction
* [ ]	Sprite extraction
* [ ]	Animation extraction
* [ ]	Audio extraction
* [ ]	Data extraction
* [ ]	Font extraction
* [ ]	Metadata extraction
* [ ]	Version detection
* [ ]	Flash game data identification

Bloons TD 4 Flash

* [ ]	BTD4 Flash version detection
* [ ]	BTD4 Flash asset identification
* [ ]	BTD4 Flash tower data identification
* [ ]	BTD4 Flash bloon data identification
* [ ]	BTD4 Flash map identification
* [ ]	BTD4 Flash round data identification
* [ ]	BTD4 Flash upgrade data identification
* [ ]	BTD4 Flash UI asset identification
* [ ]	BTD4 Flash audio identification
* [ ]	BTD4 Flash animation identification
* [ ]	BTD4 Flash content conversion
* [ ]	BTD4 Flash compatibility validation

Bloons TD 4 Expansion

* [ ]	BTD4 Expansion version detection
* [ ]	BTD4 Expansion content identification
* [ ]	BTD4 Expansion maps
* [ ]	BTD4 Expansion tower/content differences
* [ ]	BTD4 Expansion bloon/content differences
* [ ]	BTD4 Expansion rounds
* [ ]	BTD4 Expansion upgrades
* [ ]	BTD4 Expansion UI/content
* [ ]	BTD4 Expansion audio/content
* [ ]	BTD4 Expansion asset conversion
* [ ]	BTD4 Expansion compatibility validation
* [ ]	Cross-version content comparison
* [ ]	Content conflict resolution between Flash and Expansion

APK

* [ ]	APK container extraction
* [ ]	Android app structure detection
* [ ]	Resource discovery
* [ ]	Texture discovery
* [ ]	Audio discovery
* [ ]	Map discovery
* [ ]	Data discovery
* [ ]	Mobile feature detection
* [ ]	Version detection
* [ ]	Android-specific content comparison
* [ ]	APK compatibility validation

IPA

* [ ]	IPA container extraction
* [ ]	iOS app bundle detection
* [ ]	Resource discovery
* [ ]	Texture discovery
* [ ]	Audio discovery
* [ ]	Map discovery
* [ ]	Data discovery
* [ ]	Mobile feature detection
* [ ]	Version detection
* [ ]	iOS-specific content comparison
* [ ]	IPA compatibility validation

Bloons TD 4 HD / iPad

* [ ]	BTD4 HD version detection
* [ ]	iPad application structure detection
* [ ]	HD texture identification
* [ ]	HD UI identification
* [ ]	Higher-resolution asset identification
* [ ]	iPad-specific map identification
* [ ]	iPad-specific content identification
* [ ]	HD audio/content identification
* [ ]	HD map data identification
* [ ]	HD tower data identification
* [ ]	HD bloon data identification
* [ ]	HD round data identification
* [ ]	HD upgrade data identification
* [ ]	HD UI/layout data identification
* [ ]	HD asset conversion
* [ ]	HD-to-standard asset fallback generation
* [ ]	Standard-to-HD asset handling
* [ ]	BTD4 HD compatibility validation

Mobile Content

* [ ]	Beekeeper detection
* [ ]	Mobile map detection
* [ ]	Mobile UI detection
* [ ]	Mobile achievement detection
* [ ]	Mobile asset detection
* [ ]	Mobile-only feature manifest
* [ ]	Mobile feature compatibility validation

Cross-Version Import

* [ ]	Detect multiple supplied source versions
* [ ]	Merge compatible content from multiple versions
* [ ]	Prefer higher-quality assets when available
* [ ]	Prefer higher-resolution assets when available
* [ ]	Detect conflicting versions of the same asset
* [ ]	User-selectable source priority
* [ ]	Automatic source priority
* [ ]	Generate missing resolutions
* [ ]	Generate platform-optimized assets
* [ ]	Generate platform compatibility report

⸻

MILESTONE 14: GAME BUILDER

UI

* [ ]	Native builder window
* [ ]	Source file selection
* [ ]	SWF selection
* [ ]	APK selection
* [ ]	IPA selection
* [ ]	Multiple source file selection
* [ ]	Graphics Style selection (Flash vs Mobile)
* [ ]	Source version selection
* [ ]	Platform selection
* [ ]	Resolution selection
* [ ]	Scaling configuration
* [ ]	Build configuration
* [ ]	Import progress
* [ ]	Build progress
* [ ]	Build logs
* [ ]	Error display
* [ ]	Compatibility warnings
* [ ]	Imported-content summary
* [ ]	Feature selection

Source Selection

* [ ]	Flash source mode
* [ ]	BTD4 Expansion source mode
* [ ]	Android source mode
* [ ]	iOS/iPad source mode
* [ ]	Combined source mode
* [ ]	Optional IPA import
* [ ]	Optional APK import
* [ ]	Source priority configuration

Platform Selection

* [ ]	PSP
* [ ]	Windows
* [ ]	Linux
* [ ]	Xbox 360
* [ ]	Future platform abstraction

Feature Configuration

* [ ]	Base game features
* [ ]	Achievements
* [ ]	Map editor
* [ ]	Custom maps
* [ ]	Local multiplayer
* [ ]	Mobile content
* [ ]	HD assets
* [ ]	Resolution scaling
* [ ]	Platform-specific features
* [ ]	Feature compatibility validation

Project System

* [ ]	.btd4proj format
* [ ]	Project loading
* [ ]	Project saving
* [ ]	Source path management
* [ ]	Multiple source path management
* [ ]	Build configuration
* [ ]	Imported asset manifest
* [ ]	Feature configuration
* [ ]	Resolution configuration
* [ ]	Platform configuration

Build System

* [ ]	Windows builder
* [ ]	Linux builder
* [ ]	PSP builder
* [ ]	Xbox 360 builder interface
* [ ]	Toolchain detection
* [ ]	Platform dependency detection
* [ ]	Output packaging
* [ ]	Build artifact validation
* [ ]	Build reproducibility

⸻

MILESTONE 15: MAP EDITOR

* [ ]	Map editor window
* [ ]	Map canvas
* [ ]	Selection tool
* [ ]	Path tool
* [ ]	Build-area tool
* [ ]	Spawn tool
* [ ]	Exit tool
* [ ]	Object tool
* [ ]	Layer system
* [ ]	Properties panel
* [ ]	Zoom
* [ ]	Pan
* [ ]	Grid snapping
* [ ]	Undo/redo
* [ ]	Copy/paste
* [ ]	Map validation
* [ ]	Save/load
* [ ]	Playtest mode
* [ ]	Resolution-independent editing
* [ ]	High-resolution map editing
* [ ]	Map preview at multiple resolutions
* [ ]	Platform compatibility preview
* [ ]	HD asset preview
* [ ]	Import existing maps
* [ ]	Export maps to internal format

⸻

MILESTONE 16: PERFORMANCE

General

* [ ]	Profiling framework
* [ ]	Memory statistics
* [ ]	Entity pooling
* [ ]	Projectile pooling
* [ ]	Bloon pooling
* [ ]	Render batching
* [ ]	Texture atlas optimization
* [ ]	Asset streaming
* [ ]	Asset caching
* [ ]	Loading-time optimization
* [ ]	Startup-time optimization
* [ ]	CPU profiling
* [ ]	GPU/render profiling
* [ ]	Memory profiling

PSP

* [ ]	PSP memory optimization
* [ ]	PSP CPU optimization
* [ ]	PSP draw-call optimization
* [ ]	PSP texture-memory optimization
* [ ]	PSP asset-size optimization
* [ ]	PSP loading optimization
* [ ]	PSP low-resolution performance mode
* [ ]	PSP high-quality performance mode
* [ ]	PSP frame-time profiling

Desktop

* [ ]	Windows performance benchmarks
* [ ]	Linux performance benchmarks
* [ ]	Multiple resolution benchmarks
* [ ]	High-resolution rendering benchmarks

Stress Tests

Targets:

100 bloons
500 bloons
1000 bloons
100 towers
500 projectiles

Additional tests:

* [ ]	2000+ bloon stress test
* [ ]	Large map stress test
* [ ]	Maximum tower stress test
* [ ]	Maximum projectile stress test
* [ ]	Local multiplayer stress test

⸻

MILESTONE 17: MODDING

* [ ]	Mod directory
* [ ]	Mod manifest
* [ ]	Custom maps
* [ ]	Custom towers
* [ ]	Custom bloons
* [ ]	Custom textures
* [ ]	Custom sounds
* [ ]	Mod validation
* [ ]	Mod versioning
* [ ]	Mod compatibility checking
* [ ]	Custom resolution assets
* [ ]	Custom UI layouts

⸻

MILESTONE 18: MULTIPLAYER

Local Multiplayer

* [ ]	Local multiplayer game-state support
* [ ]	Player management
* [ ]	2-player local co-op
* [ ]	Multiple controller support
* [ ]	Player-specific input
* [ ]	Player-to-controller assignment
* [ ]	Player-specific tower ownership
* [ ]	Player-specific UI
* [ ]	Shared cash/economy rules
* [ ]	Shared lives rules
* [ ]	Local multiplayer pause handling
* [ ]	Local multiplayer game-start flow
* [ ]	Local multiplayer map selection
* [ ]	Local multiplayer testing

PSP Local Multiplayer

* [ ]	PSP local multiplayer architecture
* [ ]	PSP multi-controller input
* [ ]	PSP controller adapter support
* [ ]	PSP player assignment
* [ ]	PSP multiplayer performance testing
* [ ]	PSP multiplayer memory testing

Simulation

* [ ]	Deterministic simulation
* [ ]	State serialization
* [ ]	Event serialization
* [ ]	Network abstraction
* [ ]	Multiplayer-safe random number generation
* [ ]	Multiplayer desync detection

LAN

* [ ]	LAN discovery
* [ ]	Host
* [ ]	Join
* [ ]	Lobby
* [ ]	Co-op game
* [ ]	Synchronization
* [ ]	Disconnect handling

Online

* [ ]	Online transport
* [ ]	Matchmaking architecture
* [ ]	Lobby server
* [ ]	Authentication architecture
* [ ]	NAT traversal strategy
* [ ]	Reconnection

⸻

MILESTONE 19: XBOX 360

* [ ]	Xbox graphics backend
* [ ]	Xbox input
* [ ]	Xbox audio
* [ ]	Xbox filesystem
* [ ]	Xbox saves
* [ ]	Xbox achievements
* [ ]	Xbox networking
* [ ]	Xbox local multiplayer
* [ ]	Xbox multiple controller support
* [ ]	Xbox resolution support
* [ ]	Xbox HD rendering
* [ ]	Xbox packaging
* [ ]	Xbox performance optimization

⸻

MILESTONE 20: RELEASE

Platforms

* [ ]	Windows release
* [ ]	Linux release
* [ ]	PSP release
* [ ]	Xbox 360 release

Tools

* [ ]	Game Builder release
* [ ]	Map Editor release
* [ ]	Asset Importer release

Compatibility

* [ ]	Flash source compatibility testing
* [ ]	BTD4 Expansion compatibility testing
* [ ]	APK compatibility testing
* [ ]	IPA compatibility testing
* [ ]	BTD4 HD compatibility testing
* [ ]	Multiple resolution testing
* [ ]	Controller compatibility testing
* [ ]	Local multiplayer testing
* [ ]	Platform feature testing

Documentation

* [ ]	Documentation
* [ ]	Installation instructions
* [ ]	Game Builder documentation
* [ ]	Asset importing documentation
* [ ]	Modding documentation
* [ ]	Map editor documentation
* [ ]	Platform build documentation
* [ ]	PSP installation documentation

Release

* [ ]	Release CI
* [ ]	Versioning
* [ ]	Changelog
* [ ]	Final performance testing
* [ ]	Final memory testing
* [ ]	Final compatibility testing
* [ ]	Final build reproducibility testing
