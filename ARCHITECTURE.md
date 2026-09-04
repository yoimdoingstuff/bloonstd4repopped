# ARCHITECTURE.md

## 1. Project Overview

This project is a native C++ cross-platform tower-defense game and game-building toolkit inspired by classic tower-defense games.

Primary goals:

- High performance
- Cross-platform support
- PSP homebrew support
- Windows support
- Linux support
- Future Xbox 360 support
- Native Game Builder
- Custom maps and content
- Optional importing of user-provided SWF/IPA assets
- Future multiplayer
- Data-driven game systems
- Automated builds and testing

The project must not depend on proprietary game source code or proprietary binaries.

---

# 2. Core Architecture

The project is divided into several major layers:

    ┌─────────────────────────────────────┐
    │             Game Builder            │
    │   Editor / Importer / Build Tools   │
    └──────────────────┬──────────────────┘
                       │
                       ▼
    ┌─────────────────────────────────────┐
    │          Shared Game Engine          │
    │                                     │
    │ Game Logic / Maps / Towers / Bloons │
    │ Economy / Rounds / UI / Saves       │
    └──────────────────┬──────────────────┘
                       │
              Platform Abstraction
                       │
        ┌──────────────┼──────────────┐
        ▼              ▼              ▼
     Windows         Linux           PSP
        │              │              │
     Native/SDL     Native/SDL      PSP GU
