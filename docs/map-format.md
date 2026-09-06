# Internal map format, version 1

`engine/map/MapLoader.hpp` exposes `parseMap` for memory and `loadMap` for
`IFileSystem`. Both return false with a diagnostic on invalid input and leave
the destination map unchanged. This loader does not read SWF, APK or IPA data.

```json
{
  "version": 1,
  "name": "Original example",
  "paths": [[[0, 136], [240, 136], [480, 200]]],
  "buildable_regions": [[0, 0, 480, 272]],
  "blocked_regions": [[200, 180, 40, 40]]
}
```

Version, name and paths are required. Regions are optional. Object field order
is arbitrary; unknown and duplicate fields are rejected. Points are `[x,y]`,
rectangles are `[x,y,width,height]`. Each path's first and last points are its
spawn and exit. Coordinates use logical game units and may extend off-screen.
The existing Map placement rules remain authoritative.

Limits: 1 MiB of JSON, 64 paths, 4096 points per path, 1024 regions per region
list, 1024 decoded bytes per string, and coordinate magnitudes at most 1,000,000.
Paths need at least two points and positive length; rectangle dimensions must
be positive. Nonfinite numbers, invalid escapes, unsupported versions and
trailing data fail validation. No general JSON DOM or new dependency is used.

The filesystem API currently reads a whole file before the parser checks its
size. Bounded file reads are a separate future filesystem improvement.
This schema covers the existing Map model; backgrounds, objects and layers
will require explicit format evolution. It does not add runtime map selection.
