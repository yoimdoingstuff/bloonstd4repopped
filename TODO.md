TODO.md

Legend:

* [ ] Not started
* [~] In progress
* [x] Complete
* [!] Blocked

Every blocked task must include a `Blocked by:` entry and a `Reasoning:` entry.
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

* [!]	PSP renderer
	Agent: platform-abstraction
	Blocked by: PSPDEV toolchain configuration
	Reasoning: Cross-compilation for PSP requires official/open-source PSPDEV toolchain environment which is not configured locally.
* [!]	PSP input
	Agent: platform-abstraction
	Blocked by: PSPDEV toolchain configuration
	Reasoning: Requires PSPDEV toolchain environment.
* [!]	PSP audio
	Agent: platform-abstraction
	Blocked by: PSPDEV toolchain configuration
	Reasoning: Requires PSPDEV toolchain environment.
* [!]	PSP filesystem
	Agent: platform-abstraction
	Blocked by: PSPDEV toolchain configuration
	Reasoning: Requires PSPDEV toolchain environment.
* [!]	PSP memory utilities
	Agent: platform-abstraction
	Blocked by: PSPDEV toolchain configuration
	Reasoning: Requires PSPDEV toolchain environment.
* [!]	PSP packaging
	Agent: platform-abstraction
	Blocked by: PSPDEV toolchain configuration
	Reasoning: Requires PSPDEV toolchain environment.

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
* [ ]	Logical 480x272 resolution
* [ ]	Scaling
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
* [ ]	Map loader
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

* [ ]	Round data format
* [ ]	Round loader
* [ ]	Bloon groups
* [ ]	Spawn timing
* [ ]	Round completion
* [ ]	Round rewards
* [ ]	Freeplay framework

⸻

MILESTONE 9: UI

Core & Abstraction

* [ ]	UIAdapter interface
* [ ]	UILayout configuration
* [ ]	Graphics Style loading

Platform Backends

* [ ]	Desktop PlatformUI (Mouse)
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
* [ ]	Internal asset format
* [ ]	Asset manifest
* [ ]	Placeholder asset loading (fallback)
* [ ]	Placeholder JSON data authoring (1:1 accuracy)
* [ ]	Validation
* [ ]	Conversion pipeline
* [ ]	Error reporting

SWF

* [ ]	SWF parser
* [ ]	Bitmap extraction
* [ ]	Vector extraction
* [ ]	Sprite extraction
* [ ]	Animation extraction
* [ ]	Audio extraction
* [ ]	Data extraction
* [ ]	Version detection

IPA

* [ ]	IPA container extraction
* [ ]	App bundle detection
* [ ]	Resource discovery
* [ ]	Texture discovery
* [ ]	Audio discovery
* [ ]	Map discovery
* [ ]	Data discovery
* [ ]	Mobile feature detection
* [ ]	Version detection

Mobile Content

* [ ]	Beekeeper detection
* [ ]	Mobile map detection
* [ ]	Mobile UI detection
* [ ]	Mobile achievement detection
* [ ]	Mobile asset detection

⸻

MILESTONE 14: GAME BUILDER

UI

* [ ]	Native builder window
* [ ]	Source file selection
* [ ]	SWF selection
* [ ]	IPA selection
* [ ]	Graphics Style selection (Flash vs Mobile)
* [ ]	Platform selection
* [ ]	Build configuration
* [ ]	Import progress
* [ ]	Build progress
* [ ]	Build logs
* [ ]	Error display

Project System

* [ ]	.btd4proj format
* [ ]	Project loading
* [ ]	Project saving
* [ ]	Source path management
* [ ]	Build configuration

Build System

* [ ]	Windows builder
* [ ]	Linux builder
* [ ]	PSP builder
* [ ]	Xbox 360 builder interface
* [ ]	Toolchain detection
* [ ]	Output packaging

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

⸻

MILESTONE 16: PERFORMANCE

* [ ]	Profiling framework
* [ ]	Memory statistics
* [ ]	Entity pooling
* [ ]	Projectile pooling
* [ ]	Bloon pooling
* [ ]	Render batching
* [ ]	Texture atlas optimization
* [ ]	PSP memory optimization
* [ ]	PSP CPU optimization
* [ ]	PSP draw-call optimization
* [ ]	Desktop performance benchmarks

Targets:

100 bloons
500 bloons
1000 bloons
100 towers
500 projectiles

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

⸻

MILESTONE 18: MULTIPLAYER

Simulation

* [ ]	Deterministic simulation
* [ ]	State serialization
* [ ]	Event serialization
* [ ]	Network abstraction

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
* [ ]	Xbox packaging
* [ ]	Xbox performance optimization

⸻

MILESTONE 20: RELEASE

* [ ]	Windows release
* [ ]	Linux release
* [ ]	PSP release
* [ ]	Game Builder release
* [ ]	Map Editor release
* [ ]	Documentation
* [ ]	Installation instructions
* [ ]	Modding documentation
* [ ]	Release CI
* [ ]	Versioning
* [ ]	Changelog
* [ ]	Final performance testing
