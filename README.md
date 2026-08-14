# omp-cef

![Version](https://img.shields.io/github/v/release/aurora-mp/omp-cef?include_prereleases&label=version)
![License](https://img.shields.io/github/license/aurora-mp/omp-cef)
[![Wiki](https://img.shields.io/badge/docs-wiki-blue)](https://github.com/aurora-mp/omp-cef/wiki)
![CEF](https://img.shields.io/badge/CEF-148.0.10-blue)
![SA--MP](https://img.shields.io/badge/SA--MP-0.3.7%20%7C%200.3.DL-orange)
![open.mp](https://img.shields.io/badge/open.mp-supported-brightgreen)

Client/server CEF plugin for **open.mp** and **SA-MP**.

`omp-cef` embeds Chromium Embedded Framework into GTA San Andreas Multiplayer clients and allows servers to create browser-based UI in several rendering modes: Overlay2D, World2D and WorldObject3D.

## Features

- Off-screen CEF rendering inside GTA SA
- Overlay2D browser rendering for HUDs and menus
- World2D browser rendering at world positions
- WorldObject3D browser rendering through object texture replacement
- Packaged server resources served through `http://cef/...`
- Pawn natives and callbacks
- JavaScript <-> Pawn/C# event bridge
- Focus, cursor, input and audio controls
- Game screen capture exposed to CEF interfaces as RGBA frames
- Custom ESC menu support
- Custom TAB menu (scoreboard) support
- Secure client/server handshake and UDP transport

## Documentation

The full documentation is available in the GitHub Wiki:

https://github.com/aurora-mp/omp-cef/wiki

## Supported clients

- SA-MP 0.3.7-R1
- SA-MP 0.3.7-R3-1
- SA-MP 0.3.7-R5-1
- SA-MP 0.3.DL-R1

## Current CEF version

```txt
148.0.10+g7ee53f5+chromium-148.0.7778.218
```

## Repository layout

```txt
src/
  client/          # client plugin (CEF offscreen + DX9 + SA:MP hooks)
  server/
    common/        # network core, sessions, resources, protocol
    omp/           # open.mp component (bridge + lifecycle + natives)
    samp/          # SA-MP plugin (bridge + lifecycle + natives)
   shared          # shared protocol/types (packets, serialization, common utilities)
deps/              # deps (minhook, sdk, etc.)
vcpkg.json         # vcpkg manifest (if using manifest mode)
CMakeLists.txt
CMakePresets.json  # CMake presets (configure/build presets for VS/CMake)
```

## Building

Build requirements and detailed setup instructions are documented in the wiki:

https://github.com/aurora-mp/omp-cef/wiki

Short version:

- Visual Studio 2022
- CMake
- vcpkg
- Windows SDK
- x86 target for GTA SA / SA-MP client-side

## Samples

- Overlay2D HUD
- World2D browser
- WorldObject3D browser
- JavaScript <-> Pawn events
- Browser lifecycle
- Hidden browser navigation
- Stress testing
- In-game camera preview (`/camera` in the demo gamemode)

## Game screen capture

CEF pages can receive the current GTA frame through `window.cef.screen`. Frames
are captured before omp-cef overlays are rendered, preventing recursive capture
when a preview is displayed inside a browser.

```js
const canvas = document.querySelector('canvas');
const context = canvas.getContext('2d', { alpha: false });

cef.screen.start((frame) => {
  canvas.width = frame.width;
  canvas.height = frame.height;

  const pixels = new Uint8ClampedArray(frame.data);
  context.putImageData(
    new ImageData(pixels, frame.width, frame.height),
    0,
    0
  );
}, 640, 360, 15);

addEventListener('beforeunload', () => cef.screen.stop(), { once: true });
```

`cef.screen.start(callback, width, height, fps)` returns frames with `data`
(`ArrayBuffer`, RGBA), `width`, `height`, `sequence`, `timestamp`, and `format`.
Width and height are clamped to `16..1280` and `16..720`; FPS is clamped to
`1..30`. Defaults are `640x360` at `15 FPS`. High resolutions may reduce the
effective FPS, and capture stops automatically when the page or browser closes.

## Contributing

Pull requests are welcome <3.

## License

See the repository license.
