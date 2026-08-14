# omp-cef - Tests

This directory contains small demo gamemode used to test and demonstrate the main `omp-cef` features.

The samples are designed to work with both:

* open.mp
* SA-MP

The Pawn demo logic is shared to avoid duplicating code between both platforms.

---

## Directory layout

```txt
tests/
  shared/
    cef_demo.inc

  omp/
    gamemodes/
      cef_demo.pwn
    scriptfiles/
      cef/
        cef_demo/
          index.html
          overlay.html
          world2d.html
          world3d.html
          events.html
          input.html
          camera.html
          red.html
          blue.html
          empty.html
          css/
            app.css
          js/
            app.js
            cef-bridge.js

  samp/
    gamemodes/
      cef_demo.pwn
    scriptfiles/
      cef/
        cef_demo/
          index.html
          overlay.html
          world2d.html
          world3d.html
          events.html
          input.html
          camera.html
          red.html
          blue.html
          empty.html
          css/
            app.css
          js/
            app.js
            cef-bridge.js
```

---

## Resource URL convention

CEF resources are loaded through the plugin HTTP scheme:

```txt
http://cef/<resource_name>/<file>
```

For these samples:

```txt
http://cef/cef_demo/index.html
http://cef/cef_demo/overlay.html
http://cef/cef_demo/world2d.html
http://cef/cef_demo/world3d.html
http://cef/cef_demo/events.html
http://cef/cef_demo/input.html
```

---

## open.mp sample

Use:

```txt
tests/omp/gamemodes/cef_demo.pwn
```

The CEF demo resources are already located in:

```txt
tests/omp/scriptfiles/cef/cef_demo/
```

Compile the gamemode and run the open.mp test server from:

```txt
tests/omp/
```

---

## SA-MP sample

Use:

```txt
tests/samp/gamemodes/cef_demo.pwn
```

The CEF demo resources are already located in:

```txt
tests/samp/scriptfiles/cef/cef_demo/
```

Compile the gamemode and run the SA-MP test server from:

```txt
tests/samp/
```

---

## Shared Pawn code

The real demo logic is located in:

```txt
tests/shared/cef_demo.inc
```

This keeps the sample logic consistent across both platforms.

---

## Available commands

### General

```txt
/cefhelp
/cefstatus
/cefdestroyall
```

### Overlay2D

```txt
/overlay
/overlayhide
/overlayshow
/overlayreload
/overlaydestroy
/overlayrecreate
/overlayred
/overlayblue
/overlayempty
/overlayanim
/overlaydevtools
```

### World2D

```txt
/world2d
/world2dmove
/world2dhide
/world2dshow
/world2dreload
/world2ddestroy
/world2drecreate
/world2dred
/world2dblue
/world2dempty
/world2danim
/world2ddevtools
```

### WorldObject3D

```txt
/world3d
/world3dmove
/world3dattach
/world3ddetach
/world3dhide
/world3dshow
/world3dreload
/world3ddestroy
/world3drecreate
/world3dred
/world3dblue
/world3dempty
/world3danim
/world3ddevtools
```

### JavaScript events

```txt
/events
/eventsnotify
/eventsdestroy
```

### Input / focus

```txt
/input
/inputfocus
/inputblur
/inputdestroy
```

### Game screen capture

```txt
/camera
/cameradestroy
```

### Stress test

```txt
/cefstress
/cefstresshide
/cefstressshow
/cefstressred
/cefstressblue
/cefstressempty
/cefstressanim
/cefstressdestroy
/cefstressrecreate
```

---

## What these samples demonstrate

### Overlay2D

Creates a regular 2D browser overlay.

Useful for:

* HUDs
* menus
* login screens
* inventory UI
* notification panels

### World2D

Creates a browser projected into the game world at a fixed 3D position.

Useful for:

* floating labels
* interaction prompts
* world panels
* map markers

### WorldObject3D

Creates a browser rendered onto an object texture.

Useful for:

* TVs
* billboards
* monitors
* signs
* interactive screens

### JavaScript events

Demonstrates communication between:

```txt
Pawn -> CEF browser -> JavaScript
JavaScript -> CEF browser -> Pawn
```

### Input / focus

Demonstrates browser focus and keyboard input handling.

### Game screen capture

Demonstrates a phone-like CEF interface showing the live GTA frame through
`cef.screen.start()` without capturing the CEF overlay itself.

### Stress test

Creates multiple browser modes at once and tests:

* hide/show
* navigation
* destroy/recreate
* transparent pages
* stale texture behavior
* general runtime stability

---

## Basic test flow

Recommended manual test sequence:

```txt
/overlay
/overlayred
/overlayblue
/overlayempty
/overlayanim
/overlayhide
/overlayshow
/overlaydestroy
```

Then:

```txt
/world2d
/world2dred
/world2dblue
/world2dempty
/world2danim
/world2dhide
/world2dshow
/world2ddestroy
```

Then:

```txt
/world3d
/world3dred
/world3dblue
/world3dempty
/world3danim
/world3dhide
/world3dshow
/world3ddestroy
```

Finally:

```txt
/cefstress
/cefstressred
/cefstressblue
/cefstressempty
/cefstressanim
/cefstresshide
/cefstressshow
/cefstressdestroy
```

---

## Expected result

The tests should not produce:

* black textures
* stale textures
* browser freezes
* browser crashes
* visible browser content after hide
* old page flashes after hidden navigation
* duplicate browser ID errors after destroy/recreate

---

## Notes

These samples are intentionally simple and use plain HTML, CSS and JavaScript.

No React, Vue, Vite or build step is required.

The goal is to provide clear reference examples for plugin features and manual regression testing.
