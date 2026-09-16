# Asset importer and portable builder workflow

The intended distribution model is a native, portable Game Builder. The builder contains the project/build code and does not require the user to install the source game's runtime.

## User workflow

1. Extract the builder package.
2. Put the source BTD4 Flash `.swf` in the package's `assets/` directory.
3. Optionally put a mobile `.ipa` in the same directory when mobile-only resources are wanted.
4. Start `btd4_builder`.
5. The builder scans `assets/` automatically. **Rescan Assets** can repeat the scan.
6. Select Windows, Linux, PSP, or another available target in the builder.
7. Run **Import Assets**.
8. The importer converts the source into internal game data under `game_data/<platform>/`.
9. Build the selected target.

The source SWF/IPA remains an input to the importer. The game runtime is intended to consume only the generated internal data package.

## Source discovery

Discovery is implemented with C++17 `std::filesystem`, recursively scanning the configured asset directory. File extensions are compared case-insensitively, so `.SWF` and `.IPA` are accepted as well as lowercase extensions. Selection is deterministic by sorted path order.

The builder sets its working directory to the directory containing the executable before scanning. This makes the package portable when launched from a desktop shortcut or another working directory.

## Platform output

Imports are separated by target platform so switching from Windows to Linux does not overwrite the previous imported package:

```text
game_data/
  Windows/
  Linux/
  PSP/
  Xbox 360/
```

The exact platform-specific asset transformations will grow as the target backends and source-version importers become more complete.

## Current scope

The native SWF importer already parses SWF data, extracts supported bitmap/audio resources, writes a manifest, and reports detected source features. IPA handling currently provides archive/resource inspection and candidate detection rather than a complete IPA-to-game-data conversion.

BTD4-specific identification, mobile map extraction, expansion content, HD/iPad content, and complete data conversion remain separate importer milestones. A successful generic SWF parse must not be treated as proof that every BTD4 version is fully supported.
