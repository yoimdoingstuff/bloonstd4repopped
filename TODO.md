TODO.md

Legend:

* [ ] Not started
* [~] In progress
* [x] Complete
* [!] Blocked

Agents should claim a task before working on it.

⸻

MILESTONE 1: PROJECT INFRASTRUCTURE

Build System

* [ ]	Create root CMake project
* [ ]	Create CMake presets
* [ ]	Create shared engine target
* [ ]	Create desktop executable
* [ ]	Create Game Builder executable
* [ ]	Create PSP toolchain configuration
* [ ]	Create Xbox 360 placeholder backend

GitHub Actions

* [ ]	Windows CI
* [ ]	Linux CI
* [ ]	PSP CI
* [ ]	Combined build workflow
* [ ]	Artifact packaging
* [ ]	Release workflow

Testing

* [ ]	Create test framework
* [ ]	Add core engine tests
* [ ]	Add CI test execution

⸻

MILESTONE 2: PLATFORM ABSTRACTION

Core

* [ ]	Platform abstraction
* [ ]	Filesystem abstraction
* [ ]	Timing abstraction
* [ ]	Logging abstraction
* [ ]	Input abstraction
* [ ]	Audio abstraction
* [ ]	Renderer abstraction

Windows

* [ ]	Windows renderer
* [ ]	Windows input
* [ ]	Windows audio
* [ ]	Windows filesystem

Linux

* [ ]	Linux renderer
* [ ]	Linux input
* [ ]	Linux audio
* [ ]	Linux filesystem

PSP

* [ ]	PSP renderer
* [ ]	PSP input
* [ ]	PSP audio
* [ ]	PSP filesystem
* [ ]	PSP memory utilities
* [ ]	PSP packaging

Xbox 360

* [ ]	Backend skeleton only

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

* [ ]	Game state machine
* [ ]	Main menu state
* [ ]	Gameplay state
* [ ]	Pause state
* [ ]	Game over state
* [ ]	Victory state

Maps

* [ ]	Map data structure
* [ ]	Path data structure
* [ ]	Waypoints
* [ ]	Spawn points
* [ ]	Exit points
* [ ]	Buildable regions
* [ ]	Blocked regions
* [ ]	Map loader
* [ ]	Map validator

Bloons

* [ ]	Base bloon entity
* [ ]	Movement
* [ ]	Path following
* [ ]	Health
* [ ]	Popping
* [ ]	Leaking
* [ ]	Child bloons
* [ ]	Special behaviour
* [ ]	Object pooling

Towers

* [ ]	Tower entity
* [ ]	Placement
* [ ]	Range
* [ ]	Targeting
* [ ]	First targeting
* [ ]	Last targeting
* [ ]	Close targeting
* [ ]	Strong targeting
* [ ]	Attack cooldown

Projectiles

* [ ]	Projectile entity
* [ ]	Movement
* [ ]	Collision
* [ ]	Damage
* [ ]	Pierce
* [ ]	Area damage
* [ ]	Projectile pooling

⸻

MILESTONE 6: ECONOMY

* [ ]	Cash
* [ ]	Lives
* [ ]	Tower costs
* [ ]	Upgrade costs
* [ ]	Sell values
* [ ]	Round rewards
* [ ]	Economy tests

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

* [ ]	Main menu
* [ ]	Map selection
* [ ]	Tower selection
* [ ]	Tower information
* [ ]	Upgrade panel
* [ ]	Cash display
* [ ]	Lives display
* [ ]	Round display
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
