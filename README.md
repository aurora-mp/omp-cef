# omp-cef — CEF Plugin (open.mp + SA:MP)

### Client/Server CEF plugin for open.mp / SA:MP
- **Server** (open.mp component & SA-MP plugin with bridge concept): Secure UDP handshake + resource distribution (packaged ressources) + events.
- **Client** (injected DLL / plugin): off-screen CEF rendering in GTA SA, overlay / world texture Browsers, JS <-> client <-> server events.

> Network convention : **CEF UDP = server_port + 2**  
> Example: server `:7777` -> CEF (Network) `:7779`

---

### Current CEF version: 143.0.14+gdd46a37+chromium-143.0.7499.193

## Features

- ✅ Browsers (HUD/Overlay ... WebView with React, Vue.js ...)
- ✅ World Texture Browsers (3D object texture)
- ✅ Secure Handshake + KCP/UDP
- ✅ VFS / packaged resources encrypted (`.pak`)
- ✅ Events: `cef.emit(...)` -> client -> UDP -> server → Pawn/C#
- ✅ Focus/cursor management

## Supported clients

- 0.3.7-R1
- 0.3.7-R3-1
- 0.3.7-R5-1
- 0.3.DL-R1

## Games tested

- Clean GTA San Andreas
- GTA San Andreas with *SA Essentials*
- GTA San Andreas with *SAMP Addons 2.7*
  
---

## Repository layout

```
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

---

## Requirements

### Windows (build)
- Visual Studio 2022 (MSVC), CMake >= 3.24
- vcpkg (manifest mode recommended)
- Windows SDK

### Client runtime
- GTA San Andreas with .ASI Loader
- SA:MP / open.mp client compatible
- `cef/` folder deployed in the GTA directory (binaries, locales, etc.)

### Server runtime
- open.mp/sa-mp server
- Write access to `scriptfiles/cef/` (resources)

---

### 1) Setup vcpkg

```powershell
git clone https://github.com/microsoft/vcpkg.git
.\vcpkg\bootstrap-vcpkg.bat
```

If you use a `vcpkg.json` (manifest):
- CMake will resolve dependencies automatically via the vcpkg toolchain.

### 2) Configure CMake

```powershell
cmake -S . -B build `
  -DCMAKE_TOOLCHAIN_FILE="E:/vcpkg/scripts/buildsystems/vcpkg.cmake" `
  -DVCPKG_TARGET_TRIPLET=x86-windows
```

> ⚠️ The triplet depends on your target (often **x86** for GTA SA / SA:MP).

### 3) Build

```powershell
cmake --build build --config Release
```

Outputs are typically located under `build/.../Release/`.

> Note (Visual Studio): If you’re using Visual Studio 2022, you don’t have to run cmake -S . -B build. Just open the repository folder in Visual Studio (File → Open → Folder). Visual Studio will pick up CMakePresets.json, configure CMake automatically, and you can build from the IDE.

---

## Server installation

### Ports
The server listens on `port` (e.g., 7777).  
The CEF plugin listens on **`port + 2`** (e.g., 7779).

✅ Open **UDP (server_port + 2)** in:
- Windows Firewall (Inbound rule)
- Provider/cloud firewall (Security Group), if applicable (VPS/cloud)

### Files
Copy:
- the compiled server plugin/component into the expected **open.mp** location (`components/`) or **SA-MP** (`plugins/`)
- Resources into:
  - `scriptfiles/cef/<resource_name>/...`
 
## WebView build (e.g React/Vite)

With Vite, you need **relative paths** so `http://cef/<resource>/...` works.

In `vite.config.ts`:

```ts
export default defineConfig({
  base: "./",
});
```

Build:

```bash
pnpm install
pnpm run build
```

Copy the contents of `dist/` to:

```
scriptfiles/cef/webview/
  index.html
  assets/...
  favicon.ico
```

Then pack:
- The server plugin will generate `your_resource.pak`
- The server distributes it to clients under `GTA San Andreas User Files/cef/cache/<ip_port>/`

---

## Client installation

Copy:
- `cef.asi` into the root of GTA San Andreas directory
- the `cef/` folder (CEF binaries + locales + resources)

Example:

```
GTA San Andreas/
  cef/
    locales/
    client.dll
    icudtl.dat
    libcef.dll
    ...
    renderer.exe
  cef.asi
  ...
```

---

## Troubleshooting

### `Resource 'assets' not found`
Your build serves `/assets/...` instead of `assets/...`  
➡️ Fix Vite: `base: "./"`

---

## Contributing

PRs welcome.
- CMake + vcpkg manifest mode

--- 

## TODO

- Some *tests* (sample gamemode, resources ... etc)
- Wiki to explain natives/callbacks

cmake -S . -B build -A Win32 -DCMAKE_TOOLCHAIN_FILE="C:/Users/evo_l/Downloads/NSYCef/vcpkg/scripts/buildsystems/vcpkg.cmake" -DVCPKG_TARGET_TRIPLET=x86-windows-static -DVCPKG_MANIFEST_FEATURES="client;server" -DBUILD_CLIENT=ON -DBUILD_SERVER_OMP=ON

cmake --build build --config Release
cmake --build build --config Release --target Client
cmake --build build --config Release --target Loader
cmake --build build --config Release --target Renderer



# Para forzar la reconstrucción total de todo el proyecto (borrando caché y forzando post-builds):
cmake --build build --config Release --clean-first

# O para forzar la reconstrucción de un solo módulo:
cmake --build build --config Release --target Client --clean-first

# Para compilar el servidor SA-MP:
cmake --build --preset="Server Samp x86 (Debug)"

# Para compilar todo el Cliente (Loader, Renderer, Client):
cmake --build --preset="Client x86 (Debug)"


$env:VCPKG_ROOT="c:\Users\evo_l\Downloads\NSYCef\vcpkg"
cmake --build build --config Release --clean-first


$env:VCPKG_ROOT="c:\Users\evo_l\Downloads\NSYCef\vcpkg"
cmake --build build --config Release --clean-first