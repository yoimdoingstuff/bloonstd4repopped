# Builder asset folder

The Game Builder uses this directory as its default source folder.

Drop the user-provided source files here:

- One `.swf` file for the base Flash game.
- Optionally one `.ipa` file for mobile-only resources such as mobile maps or Beekeeper candidates.

The builder recursively scans this directory when it starts and when **Rescan Assets** is pressed. It selects the first SWF and first IPA found in deterministic path order.

After selecting a target platform, **Import Assets** writes converted data to:

```text
game_data/<platform>/
```

The importer is native C++ and is intended to work on Windows and Linux without requiring Flash Player, an emulator, or a web runtime. The source files are treated as import inputs only. The runtime consumes the generated internal game-data package.

For a portable release, the intended layout is:

```text
BTD4-Repopped/
  btd4_builder.exe       # Windows example
  assets/
    game.swf
    mobile.ipa            # optional
  game_data/
  projects/
```

Linux uses the same layout with the native Linux builder executable.
