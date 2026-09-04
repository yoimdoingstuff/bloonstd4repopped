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
