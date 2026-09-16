# Presentation, Aspect Ratios, and Map Variants

The project does not treat aspect ratio as a simple stretch setting. Different source versions and platforms may contain different map layouts, UI arrangements, and presentation dimensions.

## Separation of concerns

The engine has three related but separate concepts:

```text
Source/content style
        |
        +-- Flash assets
        +-- Mobile assets

Platform frontend
        |
        +-- Flash Desktop
        +-- PSP
        +-- Xbox Console

Presentation profile
        |
        +-- logical resolution
        +-- aspect ratio
        +-- viewport scaling
        +-- letterbox/pillarbox
        +-- map/layout variant
```

Graphics style chooses the visual source assets. The frontend chooses how the player interacts with the game. The presentation profile chooses how the selected content is fitted to the display and which compatible map/layout variant is used.

## Map variants

A map identity should not require every platform to use identical artwork or dimensions. A future internal package may represent variants like:

```text
maps/
└── launch_land/
    ├── common.json
    ├── flash.json
    ├── mobile.json
    ├── psp.json
    └── xbox360.json
```

`common.json` represents shared gameplay information where the versions are equivalent. A platform/source variant may provide different background art, bounds, object placement, paths, buildable regions, or other presentation-specific data when the original source version actually differs.

The importer should preserve detected source-specific variants rather than silently scaling one map into another format.

## Viewport behavior

Rendering uses a logical game rectangle inside the physical window/display.

For example, if a 4:3 logical map is displayed inside a widescreen desktop window, the engine should calculate the largest correctly scaled 4:3 rectangle and center it. The unused area becomes letterboxing or pillarboxing.

```text
+------------------------------------------+
|              letterbox                   |
|     +----------------------------+       |
|     |                            |       |
|     |       logical map          |       |
|     |        4:3                 |       |
|     |                            |       |
|     +----------------------------+       |
|              letterbox                   |
+------------------------------------------+
```

The exact bars may be horizontal or vertical depending on the source and display aspect ratios.

Integer scaling should be preferred where the selected target benefits from pixel-accurate presentation. Fractional scaling remains available for modern desktop resolutions.

## Coordinate conversion

The same transform must be shared by rendering and input:

```text
physical display coordinates
        |
        v
viewport transform
        |
        v
logical game coordinates
        |
        v
map/UI hit testing
```

This is required for:

- Windows mouse input
- controller virtual cursors
- tower placement
- tower selection
- map editor placement
- map editor selection
- UI hit testing
- camera operations

A click outside the logical game rectangle must not accidentally become a click on the map.

## Target intent

Windows should retain the Flash-style mouse-first presentation and interaction model.

PSP should use the PSP-oriented frontend and 480x272 display profile.

Xbox 360 should use the controller-first console frontend and an appropriate widescreen presentation profile when its backend becomes available.

Mobile content remains selectable independently through the graphics/content style and source package. A mobile map should not be assumed to be geometrically identical to a Flash map merely because both represent the same named level.

## Builder integration

The Game Builder should eventually expose the selections needed to make this deterministic:

```text
Source:
  SWF
  optional IPA

Graphics Style:
  Flash / Mobile

Frontend:
  Flash Desktop / PSP / Xbox Console

Presentation:
  Auto / source profile / explicit profile

Target:
  Windows / Linux / PSP / Xbox 360
```

The builder can use `Auto` as the normal choice, selecting the compatible presentation and map variant from the source and target. Advanced users can override it when multiple valid variants are available.

The runtime package should record the selected content, frontend, presentation profile, and map variant in its manifest so the engine does not need to guess at startup.
