
## [1.2.0](https://github.com/aurora-mp/omp-cef/compare/v1.1.0..v1.2.0) (2026-02-19)

### 🧹 Chore

- Bump next version - ([b662b38](https://github.com/aurora-mp/omp-cef/commit/b662b385cf5195c6eb588523ba85d88636165b22))

### ✨ Features

- *(client)* Add poll player stats #14 - ([4d2318f](https://github.com/aurora-mp/omp-cef/commit/4d2318f5f557a2d69df2e8279d79d98eb0f2f73e))
- *(client)* Add cef.isChatInputOpen #13 - ([37e2fc0](https://github.com/aurora-mp/omp-cef/commit/37e2fc05e348d8381f57739c6068808ba15d0747))
- *(server)* Add CEF_IsChatInputOpen & OnCefChatInputState - ([d1153bd](https://github.com/aurora-mp/omp-cef/commit/d1153bd739889ee32ecf5f217281d99209a587f4))

### 🐛 Bug Fixes

- *(client)* Fix issues with ALT + TAB & cursor restoration - ([3013b74](https://github.com/aurora-mp/omp-cef/commit/3013b7495afeba7239709c4a5449f44cfb0d444c))
- *(omp)* Fix IsPlayerNpcBot - ([eade1e4](https://github.com/aurora-mp/omp-cef/commit/eade1e47c428b367e6ae5c1534eff4e60017c610))
- Fix GLIBC compatibility - ([e68988c](https://github.com/aurora-mp/omp-cef/commit/e68988c15255100a7852b22d18101a37d3973e55))




## [1.1.0](https://github.com/aurora-mp/omp-cef/compare/v1.0.5..v1.1.0) (2026-02-12)

### 🧹 Chore

- *(client)* Improve render_manager - ([e99fbb6](https://github.com/aurora-mp/omp-cef/commit/e99fbb6b8d80b910c143fd0dadc79558e22de7c2))
- *(client)* Improve/fix plugin runtime initialization - ([0cfdf78](https://github.com/aurora-mp/omp-cef/commit/0cfdf7899d7b2bcce473055c501fc97837d09e9b))
- Update README - ([7bee73f](https://github.com/aurora-mp/omp-cef/commit/7bee73ffad33c18afb6f6e62274da04143b7d0ba))
- Bump next version - ([97ac8a7](https://github.com/aurora-mp/omp-cef/commit/97ac8a7d1f3972d90094ad64ba2702f7cfb491e3))

### ✨ Features

- *(client)* Add `cef.exitGame` - ([c684e21](https://github.com/aurora-mp/omp-cef/commit/c684e21eefd4528d4bc1c2c764c2b302cc81eac6))
- *(client)* Add `cef.set_focus` - ([bba9a73](https://github.com/aurora-mp/omp-cef/commit/bba9a7335461d0de08eb925a5e357394252a02c6))
- Add `CEF_ToggleChatInput` - ([5bab951](https://github.com/aurora-mp/omp-cef/commit/5bab951a2a47ff92f7ba7dbebd4cbdb09d18677b))
- Add `CEF_ExitGame` #8 - ([9f510a4](https://github.com/aurora-mp/omp-cef/commit/9f510a4926a61dfa20e6145ab95262cae66bbe24))
- New features added - ([00027fe](https://github.com/aurora-mp/omp-cef/commit/00027fedaeec860139003c79038d15759f0bef4a))
- New features added - ([00027fe](https://github.com/aurora-mp/omp-cef/commit/00027fedaeec860139003c79038d15759f0bef4a))
- New features added - ([00027fe](https://github.com/aurora-mp/omp-cef/commit/00027fedaeec860139003c79038d15759f0bef4a))
- New features added - ([00027fe](https://github.com/aurora-mp/omp-cef/commit/00027fedaeec860139003c79038d15759f0bef4a))
- New features added - ([00027fe](https://github.com/aurora-mp/omp-cef/commit/00027fedaeec860139003c79038d15759f0bef4a))
- New features added - ([00027fe](https://github.com/aurora-mp/omp-cef/commit/00027fedaeec860139003c79038d15759f0bef4a))
- New features added - ([00027fe](https://github.com/aurora-mp/omp-cef/commit/00027fedaeec860139003c79038d15759f0bef4a))
- New features added - ([00027fe](https://github.com/aurora-mp/omp-cef/commit/00027fedaeec860139003c79038d15759f0bef4a))
- New features added - ([00027fe](https://github.com/aurora-mp/omp-cef/commit/00027fedaeec860139003c79038d15759f0bef4a))
- New features added - ([00027fe](https://github.com/aurora-mp/omp-cef/commit/00027fedaeec860139003c79038d15759f0bef4a))

### 🐛 Bug Fixes

- *(client)* Fix #11 - ([0707307](https://github.com/aurora-mp/omp-cef/commit/070730766faf81d9c1f0fb2566ae7f417d5a97c3))
- *(client)* Improve runtime, possible crash fixes (SAMP Addons) - ([429de24](https://github.com/aurora-mp/omp-cef/commit/429de24eb3bc76cd09771b5896d4eb7b2f1ac210))
- *(server)* Fix #10 - ([99870f0](https://github.com/aurora-mp/omp-cef/commit/99870f0a9b9c8c4d8ed97eb2fbc7a8ef785e6523))




## [1.0.5](https://github.com/aurora-mp/omp-cef/compare/v1.0.4..v1.0.5) (2026-02-05)

### ✨ Features

- *(server)* Extend OnCefInitialize with reason + message - ([bcf0c78](https://github.com/aurora-mp/omp-cef/commit/bcf0c78fdbd76d2139b4f55db6862f0455ff8fcf))

### 🐛 Bug Fixes

- *(client)* Make WndProc hook resilient to SA-MP replacing window proc - ([e437c69](https://github.com/aurora-mp/omp-cef/commit/e437c690e6908880e2eb06ab5df024efd51787f9))
- *(client)* Make WndProc hook resilient to SA-MP replacing window proc - ([e437c69](https://github.com/aurora-mp/omp-cef/commit/e437c690e6908880e2eb06ab5df024efd51787f9))
- *(client)* Make WndProc hook resilient to SA-MP replacing window proc - ([e437c69](https://github.com/aurora-mp/omp-cef/commit/e437c690e6908880e2eb06ab5df024efd51787f9))
- *(client)* Make WndProc hook resilient to SA-MP replacing window proc - ([e437c69](https://github.com/aurora-mp/omp-cef/commit/e437c690e6908880e2eb06ab5df024efd51787f9))
- *(client)* Make WndProc hook resilient to SA-MP replacing window proc - ([e437c69](https://github.com/aurora-mp/omp-cef/commit/e437c690e6908880e2eb06ab5df024efd51787f9))
- *(server)* Fix NotifyCefInitialize default params. - ([a414d14](https://github.com/aurora-mp/omp-cef/commit/a414d148a59481025f10c0832a74fa37d224dff6))
- Handle NPC playerid offset + add strict client/server version check + fix encoding - ([06ee8bf](https://github.com/aurora-mp/omp-cef/commit/06ee8bf185c196fc5ae59742d92c46964a88efce))
- Handle NPC playerid offset + add strict client/server version check + fix encoding - ([06ee8bf](https://github.com/aurora-mp/omp-cef/commit/06ee8bf185c196fc5ae59742d92c46964a88efce))
- Handle NPC playerid offset + add strict client/server version check + fix encoding - ([06ee8bf](https://github.com/aurora-mp/omp-cef/commit/06ee8bf185c196fc5ae59742d92c46964a88efce))
- Handle NPC playerid offset + add strict client/server version check + fix encoding - ([06ee8bf](https://github.com/aurora-mp/omp-cef/commit/06ee8bf185c196fc5ae59742d92c46964a88efce))
- Handle NPC playerid offset + add strict client/server version check + fix encoding - ([06ee8bf](https://github.com/aurora-mp/omp-cef/commit/06ee8bf185c196fc5ae59742d92c46964a88efce))





## [1.0.4](https://github.com/aurora-mp/omp-cef/compare/v1.0.3..v1.0.4) (2026-02-03)

### 🧹 Chore

- *(client)* Add version to log - ([cbe70a0](https://github.com/aurora-mp/omp-cef/commit/cbe70a0af79550094f7102cab9639a747ef4e25a))
- *(tests)* Add very basic login example for omp & samp - ([51d7e69](https://github.com/aurora-mp/omp-cef/commit/51d7e69e9f267c325c275663018ace003d015f30))
- *(tests)* Add react webview for tests - ([cac89fa](https://github.com/aurora-mp/omp-cef/commit/cac89fa77fc10d871dad4a91ab2a08783509d14e))

### 🐛 Bug Fixes

- *(client)* Fix CEF OSR lag: queue OnPaint frames in CPU and upload textures on D3D thread during RenderAll - ([e0ddcfe](https://github.com/aurora-mp/omp-cef/commit/e0ddcfe421e6f9c4b48484d5c95c38f3842dfd7e))
- *(client)* Fix CEF OSR lag: queue OnPaint frames in CPU and upload textures on D3D thread during RenderAll - ([e0ddcfe](https://github.com/aurora-mp/omp-cef/commit/e0ddcfe421e6f9c4b48484d5c95c38f3842dfd7e))
- *(client)* Fix CEF OSR lag: queue OnPaint frames in CPU and upload textures on D3D thread during RenderAll - ([e0ddcfe](https://github.com/aurora-mp/omp-cef/commit/e0ddcfe421e6f9c4b48484d5c95c38f3842dfd7e))
- *(client)* Fix CEF OSR lag: queue OnPaint frames in CPU and upload textures on D3D thread during RenderAll - ([e0ddcfe](https://github.com/aurora-mp/omp-cef/commit/e0ddcfe421e6f9c4b48484d5c95c38f3842dfd7e))
- *(client)* Fix CEF OSR lag: queue OnPaint frames in CPU and upload textures on D3D thread during RenderAll - ([e0ddcfe](https://github.com/aurora-mp/omp-cef/commit/e0ddcfe421e6f9c4b48484d5c95c38f3842dfd7e))





## [1.0.3](https://github.com/aurora-mp/omp-cef/compare/v1.0.2..v1.0.3) (2026-02-03)

### 🧹 Chore

- *(client)* Normalize CefApp switches - ([cd813c7](https://github.com/aurora-mp/omp-cef/commit/cd813c7775efa77af9694618b473b246ce97fc8d))
- *(client)* Some clean up - ([04b47e1](https://github.com/aurora-mp/omp-cef/commit/04b47e1d6f5176998d6fdc023d7143533568a75f))
- Add issue template - ([777ed37](https://github.com/aurora-mp/omp-cef/commit/777ed37bca43af6363dc966c8897e29161cbf92d))
- Update README - ([c4e61b2](https://github.com/aurora-mp/omp-cef/commit/c4e61b24e58cc0e2f06fc1a25a3d521dc3e8dc14))
- Update cliff (duplications) - ([d3229d4](https://github.com/aurora-mp/omp-cef/commit/d3229d40a7f7c62dfb49d935b410850730ca4db0))

### 🐛 Bug Fixes

- Fix OnCefReady callback not called when no resources need to be downloaded - ([93adc9a](https://github.com/aurora-mp/omp-cef/commit/93adc9ae9899e8613c3883eb994c30b337f28db7))

### ♻️ Refactoring

- *(client)* Replace D3D9 device proxy with vtable hooks + fallback poll - ([30c9a66](https://github.com/aurora-mp/omp-cef/commit/30c9a66ec3ede94d3b6678cf2f86b97fba0353a1))
- *(client)* Replace D3D9 device proxy with vtable hooks + fallback poll - ([30c9a66](https://github.com/aurora-mp/omp-cef/commit/30c9a66ec3ede94d3b6678cf2f86b97fba0353a1))
- *(client)* Replace D3D9 device proxy with vtable hooks + fallback poll - ([30c9a66](https://github.com/aurora-mp/omp-cef/commit/30c9a66ec3ede94d3b6678cf2f86b97fba0353a1))
- *(client)* Replace D3D9 device proxy with vtable hooks + fallback poll - ([30c9a66](https://github.com/aurora-mp/omp-cef/commit/30c9a66ec3ede94d3b6678cf2f86b97fba0353a1))
- *(client)* Replace D3D9 device proxy with vtable hooks + fallback poll - ([30c9a66](https://github.com/aurora-mp/omp-cef/commit/30c9a66ec3ede94d3b6678cf2f86b97fba0353a1))
- *(client)* Replace D3D9 device proxy with vtable hooks + fallback poll - ([30c9a66](https://github.com/aurora-mp/omp-cef/commit/30c9a66ec3ede94d3b6678cf2f86b97fba0353a1))
- *(client)* Replace D3D9 device proxy with vtable hooks + fallback poll - ([30c9a66](https://github.com/aurora-mp/omp-cef/commit/30c9a66ec3ede94d3b6678cf2f86b97fba0353a1))
- *(client)* Replace D3D9 device proxy with vtable hooks + fallback poll - ([30c9a66](https://github.com/aurora-mp/omp-cef/commit/30c9a66ec3ede94d3b6678cf2f86b97fba0353a1))
- Replace d3ddevice9 cursor forcing with Win32 SetCursor hook - ([69218f3](https://github.com/aurora-mp/omp-cef/commit/69218f3377411dbdcd6b48176e3dcefdab260b2e))
- Replace d3ddevice9 cursor forcing with Win32 SetCursor hook - ([69218f3](https://github.com/aurora-mp/omp-cef/commit/69218f3377411dbdcd6b48176e3dcefdab260b2e))
- Replace d3ddevice9 cursor forcing with Win32 SetCursor hook - ([69218f3](https://github.com/aurora-mp/omp-cef/commit/69218f3377411dbdcd6b48176e3dcefdab260b2e))
- This should not negatively impact compatibility with other mods, since it reduces the intervention surface - ([30c9a66](https://github.com/aurora-mp/omp-cef/commit/30c9a66ec3ede94d3b6678cf2f86b97fba0353a1))





## [1.0.2](https://github.com/aurora-mp/omp-cef/compare/v1.0.1..v1.0.2) (2026-02-01)

### 🧹 Chore

- *(client)* Clean verbose logs - ([28a1d71](https://github.com/aurora-mp/omp-cef/commit/28a1d71a5b9f72d0ffd208a2762141d3725aace4))
- Bump next version - ([e520280](https://github.com/aurora-mp/omp-cef/commit/e5202807efb514067e96724c40ade9b9aa0f0a8a))

### ✨ Features

- *(client)* Improve DevTools, removed the DevTools shortcut (it was for easy debugging) #3 - ([a488d0d](https://github.com/aurora-mp/omp-cef/commit/a488d0ddfbbefbbc52442e5c440289c6ea2e5dc7))
- *(server)* Add new native CEF_EnableDevTools - ([6010b2d](https://github.com/aurora-mp/omp-cef/commit/6010b2daef467c3c0abd081c5453aa42e4f6879f))
- Update for #4 - ([4d4a6dd](https://github.com/aurora-mp/omp-cef/commit/4d4a6ddafc33030a1e1ccd6a48f4de6159615c91))

### 🐛 Bug Fixes

- *(client)* UI browsers not rendering (OnPresent executed after Present) - ([0e9f262](https://github.com/aurora-mp/omp-cef/commit/0e9f262df8dc481ba3c09810bd2d01cee5377adb))
- *(client)* Fix some crashes, handle device lost/reset and avoid rendering during reset , improvements #4, #6 - ([6734002](https://github.com/aurora-mp/omp-cef/commit/6734002bd5bbb9f14870221ca5876c3904bfec4f))
- *(client)* Improvements & make D3D9 hooking robust across setups (Direct3DCreate9 + CreateDevice) #6 - ([c05ccdf](https://github.com/aurora-mp/omp-cef/commit/c05ccdf713879601f10253d450b593348a214a33))

### 🔄 Others

- *(client)* Improve DevTools, removed the DevTools shortcut (it was for easy debugging) #3 - ([a488d0d](https://github.com/aurora-mp/omp-cef/commit/a488d0ddfbbefbbc52442e5c440289c6ea2e5dc7))
- *(client)* Improve DevTools, removed the DevTools shortcut (it was for easy debugging) #3 - ([a488d0d](https://github.com/aurora-mp/omp-cef/commit/a488d0ddfbbefbbc52442e5c440289c6ea2e5dc7))
- *(client)* Improve DevTools, removed the DevTools shortcut (it was for easy debugging) #3 - ([a488d0d](https://github.com/aurora-mp/omp-cef/commit/a488d0ddfbbefbbc52442e5c440289c6ea2e5dc7))
- *(client)* Fix some crashes, handle device lost/reset and avoid rendering during reset , improvements #4, #6 - ([6734002](https://github.com/aurora-mp/omp-cef/commit/6734002bd5bbb9f14870221ca5876c3904bfec4f))
- *(client)* Fix some crashes, handle device lost/reset and avoid rendering during reset , improvements #4, #6 - ([6734002](https://github.com/aurora-mp/omp-cef/commit/6734002bd5bbb9f14870221ca5876c3904bfec4f))
- *(client)* Fix some crashes, handle device lost/reset and avoid rendering during reset , improvements #4, #6 - ([6734002](https://github.com/aurora-mp/omp-cef/commit/6734002bd5bbb9f14870221ca5876c3904bfec4f))
- *(client)* Fix some crashes, handle device lost/reset and avoid rendering during reset , improvements #4, #6 - ([6734002](https://github.com/aurora-mp/omp-cef/commit/6734002bd5bbb9f14870221ca5876c3904bfec4f))
- *(client)* Fix some crashes, handle device lost/reset and avoid rendering during reset , improvements #4, #6 - ([6734002](https://github.com/aurora-mp/omp-cef/commit/6734002bd5bbb9f14870221ca5876c3904bfec4f))
- *(client)* Fix some crashes, handle device lost/reset and avoid rendering during reset , improvements #4, #6 - ([6734002](https://github.com/aurora-mp/omp-cef/commit/6734002bd5bbb9f14870221ca5876c3904bfec4f))
- *(client)* Fix some crashes, handle device lost/reset and avoid rendering during reset , improvements #4, #6 - ([6734002](https://github.com/aurora-mp/omp-cef/commit/6734002bd5bbb9f14870221ca5876c3904bfec4f))
- *(client)* Fix some crashes, handle device lost/reset and avoid rendering during reset , improvements #4, #6 - ([6734002](https://github.com/aurora-mp/omp-cef/commit/6734002bd5bbb9f14870221ca5876c3904bfec4f))
- *(client)* Fix some crashes, handle device lost/reset and avoid rendering during reset , improvements #4, #6 - ([6734002](https://github.com/aurora-mp/omp-cef/commit/6734002bd5bbb9f14870221ca5876c3904bfec4f))
- *(client)* Improvements & make D3D9 hooking robust across setups (Direct3DCreate9 + CreateDevice) #6 - ([c05ccdf](https://github.com/aurora-mp/omp-cef/commit/c05ccdf713879601f10253d450b593348a214a33))
- *(client)* Improvements & make D3D9 hooking robust across setups (Direct3DCreate9 + CreateDevice) #6 - ([c05ccdf](https://github.com/aurora-mp/omp-cef/commit/c05ccdf713879601f10253d450b593348a214a33))
- *(client)* Improvements & make D3D9 hooking robust across setups (Direct3DCreate9 + CreateDevice) #6 - ([c05ccdf](https://github.com/aurora-mp/omp-cef/commit/c05ccdf713879601f10253d450b593348a214a33))
- *(client)* Improvements & make D3D9 hooking robust across setups (Direct3DCreate9 + CreateDevice) #6 - ([c05ccdf](https://github.com/aurora-mp/omp-cef/commit/c05ccdf713879601f10253d450b593348a214a33))
- *(client)* Improvements & make D3D9 hooking robust across setups (Direct3DCreate9 + CreateDevice) #6 - ([c05ccdf](https://github.com/aurora-mp/omp-cef/commit/c05ccdf713879601f10253d450b593348a214a33))
- *(client)* Improvements & make D3D9 hooking robust across setups (Direct3DCreate9 + CreateDevice) #6 - ([c05ccdf](https://github.com/aurora-mp/omp-cef/commit/c05ccdf713879601f10253d450b593348a214a33))
- *(client)* Improvements & make D3D9 hooking robust across setups (Direct3DCreate9 + CreateDevice) #6 - ([c05ccdf](https://github.com/aurora-mp/omp-cef/commit/c05ccdf713879601f10253d450b593348a214a33))





## [1.0.1](https://github.com/aurora-mp/omp-cef/compare/v1.0.0..v1.0.1) (2026-01-30)

### 🧹 Chore

- Bump version - ([7ee6569](https://github.com/aurora-mp/omp-cef/commit/7ee65699174583053ce099d72bd4e55c82aec6d1))
- Update README - ([f504958](https://github.com/aurora-mp/omp-cef/commit/f5049585c91a3473a3699d136b7ade0690445440))

### ✨ Features

- Added missing natives ​​(forgotten...) - ([1e53aec](https://github.com/aurora-mp/omp-cef/commit/1e53aec6667db09a2428aaca8b421db9fd24b4a9))

### 🐛 Bug Fixes

- Fix github workflow for binaries in zip - ([f22e35e](https://github.com/aurora-mp/omp-cef/commit/f22e35e167727d6b87d1acd460343e02b5cc7eb2))

### 🔄 Others

- Merge branch 'main' of https://github.com/aurora-mp/omp-cef - ([de1beb2](https://github.com/aurora-mp/omp-cef/commit/de1beb244d9d0f0a1cc2d8a3139092872dc17d15))





## [1.0.0] (2026-01-30)

### 🧹 Chore

- *(client)* Clean up - ([a79ef89](https://github.com/aurora-mp/omp-cef/commit/a79ef89ed73f40239c10d207055a910f7435f44b))
- *(client)* Init from my own repository (refactoring wip) - ([3917bdb](https://github.com/aurora-mp/omp-cef/commit/3917bdbd0aec4178a8eda620cd9999aacac219d9))
- *(omp)* Clean up CMakeLists - ([4e81a83](https://github.com/aurora-mp/omp-cef/commit/4e81a83a5272381c0d7221911303deace6eab508))
- *(server)* Init from my own repository (refactoring wip) - ([b1e2fd5](https://github.com/aurora-mp/omp-cef/commit/b1e2fd51b6a79870a1a21a98826e5d4ef8d1da40))
- *(shared)* Init from my own repository - ([304f513](https://github.com/aurora-mp/omp-cef/commit/304f513dff91774ba132408d52b517f87e9a6e4f))
- Update github workflow - ([3f3612d](https://github.com/aurora-mp/omp-cef/commit/3f3612d374973fad7e3169cdabdee01b7b3c2f12))
- Update github workflows - ([4d725a2](https://github.com/aurora-mp/omp-cef/commit/4d725a2850cedad64bfc2d50b1bf1b81c89a2b59))
- Update README - ([4eee777](https://github.com/aurora-mp/omp-cef/commit/4eee777b3201ba81565f98fe496075e3d5795df5))
- Update github workflows - ([871f77a](https://github.com/aurora-mp/omp-cef/commit/871f77aa1e93bbab29d015316cb5388e19627fb2))
- Add github workflows - ([cc07512](https://github.com/aurora-mp/omp-cef/commit/cc07512dcaf7d601b41142b9453978b4617cfbd7))
- Update github actions - ([523ebb1](https://github.com/aurora-mp/omp-cef/commit/523ebb1aceac6d55f4c849968a0d7a744922cf53))
- Add github actions - ([cd3881c](https://github.com/aurora-mp/omp-cef/commit/cd3881c143872f71dde2782fabb5d698e3d8c7a4))
- Update README - ([a582246](https://github.com/aurora-mp/omp-cef/commit/a582246576f64d80851c5ac2c1355d56f750b57d))
- Update README - ([2f14c94](https://github.com/aurora-mp/omp-cef/commit/2f14c94ffc56a6bbfabe1f24511b4352d14363b4))
- Add LICENSE - ([45bc88a](https://github.com/aurora-mp/omp-cef/commit/45bc88ae4ae78f6dd9415e92968962b376ce7d61))
- Create README - ([ef40070](https://github.com/aurora-mp/omp-cef/commit/ef4007031d6a1773fd2dbb027cef1990aef1c7c1))
- Clean up - ([bfa2a2b](https://github.com/aurora-mp/omp-cef/commit/bfa2a2beb6158c8605f12afece57c95924e40510))
- Pin deps/asio to 231cb29b - ([1981601](https://github.com/aurora-mp/omp-cef/commit/19816014c76d222f7620175fcac5c5cb262ad314))
- Add submodules - ([210ba4c](https://github.com/aurora-mp/omp-cef/commit/210ba4c4e732252e9bd3899fc7dab33390f95672))
- Re init omp-sdk deps - ([95dca18](https://github.com/aurora-mp/omp-cef/commit/95dca18ad4709b1d60a9a385b095d4cbc0ca8806))
- Init repository (refactoring in progress) - ([3f46834](https://github.com/aurora-mp/omp-cef/commit/3f46834b191d6e67cabe4e3f5207acbdfd61860a))

### ✨ Features

- *(client)* Update loader progress bar color - ([ab7ec89](https://github.com/aurora-mp/omp-cef/commit/ab7ec89189d6b214a0592ffc20ffd351dedf620b))
- *(renderer)* Add cef.off - ([c22787c](https://github.com/aurora-mp/omp-cef/commit/c22787c66c7540efd9934a83c1cb9a7c3c50452f))
- *(server)* Add back samp bridge - ([f7e5ef0](https://github.com/aurora-mp/omp-cef/commit/f7e5ef0ed5a88fa56b7bf6f1c6d7f3d628eb556a))
- Create internal browser wrapper for youtube/twitch videos - ([bacb69c](https://github.com/aurora-mp/omp-cef/commit/bacb69ceaed05985c6f52daf5ab60cb037251888))
- Add server version in log - ([60666ea](https://github.com/aurora-mp/omp-cef/commit/60666ea2f4f77caa3b98d6759e66869f47cb7380))
- Add OnCefReady callback + improvements. - ([5a93622](https://github.com/aurora-mp/omp-cef/commit/5a93622b01aff08332815c952ed66ed2ee00e547))
- Improvements + cleanup - ([2ccd3f5](https://github.com/aurora-mp/omp-cef/commit/2ccd3f5fe35e98dd534b567d61e19f3317801ba1))
- Improvements + smooth resource loader + gate browser creation until resources ready - ([158c551](https://github.com/aurora-mp/omp-cef/commit/158c55132dd656c7b66c0e99551225b5e6eb58d1))
- Add profile_dir (persist) - ([5add4e5](https://github.com/aurora-mp/omp-cef/commit/5add4e5aecd7d2569528f165608b73e080bb5a12))
- Add back RegisterEvent - ([c2c3337](https://github.com/aurora-mp/omp-cef/commit/c2c33372a1dbc171771c565bd298ae156c3e0284))
- Add back EmitBrowserEvent - ([56d2d8f](https://github.com/aurora-mp/omp-cef/commit/56d2d8ffd2042dad5b77b1d66a53ebe28436e3c6))

### 🐛 Bug Fixes

- *(renderer)* Fixes race between server emit and cef.on - ([7d4f106](https://github.com/aurora-mp/omp-cef/commit/7d4f1067bafbabd474c6202d5d508991e849f295))
- Fix warnings & build - ([1f94c99](https://github.com/aurora-mp/omp-cef/commit/1f94c9959f6d62b3fc19a1dd576a9b98ae4c42b8))
- Github workflow - ([9a696db](https://github.com/aurora-mp/omp-cef/commit/9a696db8207d59a18db74403e31043a39fe8e9d3))
- Fix rc win version - ([66f61b4](https://github.com/aurora-mp/omp-cef/commit/66f61b4fff6127cb24e85f8c016bbd37d5159e3a))
- Avoid deadlock when emitting events during KCP processing - ([d97ea0d](https://github.com/aurora-mp/omp-cef/commit/d97ea0d19179859c4c91167deb5f0c2b2e474227))

### 🔄 Others

- Merge branch 'main' of https://github.com/aurora-mp/omp-cef - ([2f370c1](https://github.com/aurora-mp/omp-cef/commit/2f370c18ec7fb3776d4ab2e719cbd0890d39631f))
- Merge branch 'main' of https://github.com/aurora-mp/omp-cef - ([39691d4](https://github.com/aurora-mp/omp-cef/commit/39691d49db1c21214d8c997f2eb0fe105b10e8f4))
- Merge branch 'main' of https://github.com/aurora-mp/omp-cef - ([809a444](https://github.com/aurora-mp/omp-cef/commit/809a444d1c72b05efd8f36ec14feb79dff7f99ce))
- Create README - ([ef40070](https://github.com/aurora-mp/omp-cef/commit/ef4007031d6a1773fd2dbb027cef1990aef1c7c1))
- Avoid deadlock when emitting events during KCP processing - ([d97ea0d](https://github.com/aurora-mp/omp-cef/commit/d97ea0d19179859c4c91167deb5f0c2b2e474227))
- Avoid deadlock when emitting events during KCP processing - ([d97ea0d](https://github.com/aurora-mp/omp-cef/commit/d97ea0d19179859c4c91167deb5f0c2b2e474227))

## New Contributors ❤️

* @NeekoGta made their first contribution



