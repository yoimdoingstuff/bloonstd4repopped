# Asset Pipeline & Importer Specification

## 1. Overview

The `bloonstd4repopped` asset pipeline converts untrusted, user-supplied Flash (`.swf`) and mobile (`.ipa`) packages into the project's internal, normalized asset format. 

In adherence to Section 3 and 4 of `AGENTS.md`:
* The runtime engine **never** depends on SWF or IPA formats.
* The repository does **not** contain copyrighted game assets, audio, or maps.
* The importer parses user files and produces clean, standard internal assets.
* When assets are missing, the runtime engine gracefully falls back to built-in placeholder assets to remain fully playable and testable.

```
User File (SWF / IPA)
         │
         ▼
 ┌────────────────┐
 │  SwfParser /   │ ──> Decompresses CWS (zlib DEFLATE),
 │  IpaExtractor  │     reads tags (images, audio, symbol classes)
 └────────────────┘
         │
         ▼
 ┌────────────────┐
 │ AssetConverter │ ──> Normalizes class names, converts bitmaps to BMP/PNG,
 └────────────────┘     audio to WAV/MP3, maps and rounds to JSON
         │
         ▼
 ┌────────────────┐
 │  game_data/    │ ──> Standardized internal game data package
 │  manifest.json │     consumed directly by the Engine
 └────────────────┘
```

---

## 2. Internal Game Data Layout

Extracted and converted assets are stored in a `game_data/` directory with the following structure:

```
game_data/
├── manifest.json         # Master asset catalog
├── textures/             # Converted BMP / PNG / JPEG images
│   ├── tower_dart_monkey.bmp
│   ├── bloon_red.bmp
│   └── ...
├── audio/                # Converted WAV / MP3 sound cues
│   ├── pop.wav
│   └── ...
├── maps/                 # Data-driven JSON maps
│   └── original_map.json
└── rounds/               # Data-driven JSON wave progression
    └── default_rounds.json
```

---

## 3. Manifest Format (`manifest.json`)

The manifest maps asset keys to relative file paths:

```json
{
  "version": 1,
  "package_name": "Bloons TD 4 Game Data",
  "source": "/path/to/bloonstd4.swf",
  "textures": {
    "tower_dart_monkey": "textures/tower_dart_monkey.bmp",
    "bloon_red": "textures/bloon_red.bmp"
  },
  "audio": {
    "pop": "audio/pop.wav"
  },
  "maps": [
    "maps/original_map.json"
  ],
  "rounds": "rounds/default_rounds.json"
}
```

---

## 4. Standalone CLI Tool: `btd4_importer`

A native command-line utility is provided to run asset conversion headless:

```bash
btd4_importer <source.swf> [--out <output_dir>] [--ipa <source.ipa>]
```

### Options:
* `--out <dir>`: Sets output directory for `game_data/` (default: `game_data`).
* `--ipa <file>`: Optional path to iOS IPA package to discover mobile-exclusive content (e.g. Beekeeper).
* `--help`: Displays CLI usage.

---

## 5. Game Builder Integration

The native desktop Game Builder (`btd4_builder`) integrates the asset importer directly into its UI:
1. Select source SWF file.
2. Select optional IPA package.
3. Click **Import Assets** to trigger `AssetImporter::run`.
4. Progress and logs (textures extracted, audio streams parsed, symbol mappings) are streamed live into the log console.

---

## 6. Runtime Fallbacks (`AssetManager`)

If no `manifest.json` or extracted assets are found on disk, `AssetManager` automatically enables procedural fallbacks:
* Distinct 1:1 color-coded shapes for all bloon tiers (Red, Blue, Green, Yellow, Pink, Black, White, Lead, Rainbow, Ceramic, MOAB).
* Distinct color-coded shapes for base towers (Dart Monkey, Tack Shooter, Sniper, Boomerang, Bomb Tower, Super Monkey).
* Synthesized audio cues via `NullAudio` or procedural clicks.
