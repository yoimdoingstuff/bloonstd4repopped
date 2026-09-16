# Portable Game Builder Workflow

The long-term user-facing workflow is a portable native Game Builder. The user should not need the repository source tree, CMake, or a development environment just to import their own game source and build a target.

## User workflow

1. Download the native Game Builder package.
2. Place supported source content in the builder's `assets/` directory.
3. Launch the builder.
4. The builder discovers supported source files automatically.
5. Review detected source/version/content information.
6. Select the target platform.
7. Import/convert the source into internal game data.
8. Configure the build.
9. Build and package the selected target.

A typical portable layout is:

```text
BTD4-Repopped/
├── BTD4Builder.exe       # Windows example
├── assets/
│   ├── game.swf          # user supplied
│   └── mobile.ipa        # optional user supplied
├── projects/
├── game_data/
│   ├── Windows/
│   ├── Linux/
│   ├── PSP/
│   └── Xbox 360/
└── builds/
    ├── Windows/
    ├── Linux/
    ├── PSP/
    └── Xbox 360/
```

Linux uses the equivalent native builder executable and directory layout.

## Source discovery

Source discovery is deliberately separate from conversion. It may locate SWF, IPA, and other supported input files, but discovery does not imply that the content has been successfully identified or converted.

The builder should:

- accept an explicit asset directory
- recursively discover supported source files where appropriate
- treat extensions case-insensitively
- show exactly which files were selected
- allow the user to rescan after changing the asset directory
- report missing or invalid source files clearly

## Import isolation

Imported data belongs in platform-specific generated directories and is independent of the original source package. The runtime loads only the generated internal format.

For example:

```text
assets/game.swf
        |
        v
    SWF importer
        |
        v
 game_data/Windows/
        |
        v
 Windows build
```

The same source can be imported again for another target without making the runtime read the SWF directly.

## Windows and Linux testing

The desktop builder is the first practical target for end-to-end importer testing. Tests should use synthetic or legally redistributable fixtures rather than placing proprietary BTD4 packages in the repository.

The test matrix should cover:

- Windows path separators and drive-qualified paths
- Linux paths
- relative paths next to the builder
- spaces in filenames and directories
- upper/lower-case `.SWF`, `.swf`, `.IPA`, and `.ipa` extensions
- missing assets
- multiple candidate source files
- optional IPA presence/absence
- output isolation between target platforms
- manifest generation
- importer failures without corrupting an existing output package

The existence of the portable builder does not remove the need for a real source conversion test. Source discovery, parsing, conversion, and packaging should each report their own status.

## Toolchain handling

The builder is distributed as a native application, but target toolchains remain platform-specific. Windows/Linux builds can be developed and tested on desktop hosts. PSP and Xbox 360 require their appropriate development environments and must report when the required toolchain is unavailable.

The UI should never display a successful build merely because configuration completed. A successful build means the requested build and packaging operations actually completed and produced verified output.
