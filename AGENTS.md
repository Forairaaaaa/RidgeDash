# AGENTS.md

This file provides guidance to Codex (Codex.ai/code) when working with code in this repository.

## Project

RidgeDash — a retro pixel-art 2D side-scrolling physics-based driving game inspired by Hill Climb Racing. Built in C++17 on raylib + Box2D.

## Build & Run

First, fetch dependencies (raylib, box2d, smooth_ui_toolkit):

```bash
python3 fetch_repos.py
```

### Desktop (Linux/macOS)

```bash
cmake -S . -B build/desktop -DRIDGEDASH_RAYLIB_PLATFORM=Desktop
cmake --build build/desktop -j8
./dist/RidgeDash
```

### Web (Emscripten)

```bash
cmake -S . -B build/web -DRIDGEDASH_RAYLIB_PLATFORM=Web
cmake --build build/web -j8
# Serves dist/ on localhost (requires emrun or any HTTP server)
```

### Embedded (DRM / FBDEV)

```bash
# Native build
cmake -S . -B build/cp0 -DRIDGEDASH_RAYLIB_PLATFORM=DRM
cmake --build build/cp0 -j8

# Cross-compile for aarch64
cmake -S . -B build/cp0 \
  -DRIDGEDASH_RAYLIB_PLATFORM=DRM \
  -DCMAKE_TOOLCHAIN_FILE=cmake/aarch64-linux-gnu.cmake
cmake --build build/cp0 -j8
```

Replace `DRM` with `FBDEV` for framebuffer-only builds.

### Packaging

**macOS (.app bundle, ad-hoc signed, arm64 only):**
```bash
./packaging/macos/package_macos.sh
# Produces: dist/artifacts/RidgeDash-macos-arm64.zip
```

**CardputerZero (.deb):**
```bash
./packaging/cardputer/package_cardputer.sh
# Default: launcher wrapper + both DRM and FBDEV backends (auto-fallback at runtime).
# Force a single backend:
#   RIDGEDASH_PACKAGE_PLATFORM=FBDEV ./packaging/cardputer/package_cardputer.sh
# Produces: dist/artifacts/m5cardputerzero-ridgedash_x.x.x_m5stack1_arm64.deb
```

### Environment variables (Desktop)

| Variable | Effect |
|---|---|
| `RIDGEDASH_WINDOW_SCALE` | `1`–`4` (default 4), or `fullscreen` |
| `RIDGEDASH_RENDER_SCALE` | FBO supersample factor (default 2 for Desktop/DRM, 1 for FBDEV) |
| `RIDGEDASH_TARGET_FPS` | Render FPS (default 120, clamped 30–240; `0`/`unlimited` uncaps) |
| `RIDGEDASH_CRT` | `0`/`off` disables the CRT shader (default on) |
| `RIDGEDASH_TEST_TERRAIN` | Force a specific terrain profile: `mountain`, `stone`, `desert`, `snow`, or a profile name like `rolling`, `ridges`, `steps`, `valley` |

## Architecture

### Entry points

- **`src/main.cpp`** — Desktop/DRM/FBDEV entry. Sets up the window, render target (FBO supersampling), CRT shader, and runs the blocking game loop calling `game.update()` / `game.draw()` each frame. Handles letterboxing, fullscreen toggling, and display-scale changes at runtime.
- **`src/main_web.cpp`** — Emscripten entry. Uses `emscripten_set_main_loop` so the browser thread is never blocked. Owns a heap-allocated `WebState` struct; otherwise mirrors the desktop setup (FBO supersampling, CRT shader, render interpolation).

### Core game class (`src/game/ridge_dash_game.hpp/.cpp`)

`RidgeDashGame` owns every subsystem and orchestrates the per-frame update. Its `update(float dt)` method:
1. Polls input → `GameInput`
2. Steps physics at a fixed 60Hz (accumulating leftover `dt`), calling `stepPhysics()` → Box2D world step + vehicle forces + pickup step-forces + sensor events
3. Updates world streaming (terrain/pickup generation ahead, trim behind)
4. Updates higher-level state: ground contact, tricks, run state, camera, environment biome, driver expression, engine/BGM audio
5. Updates UI animations and pause menu

`draw()` lives in **`src/game/render.cpp`** (separated from the update logic). It delegates to each subsystem: environment background → terrain → pickups → vehicle → pickup effects → HUD / start tips / game-over / pause overlays. Shared rendering helpers (camera math, coordinate transforms, common drawing routines) are in **`src/game/render_helpers.hpp/.cpp`**.

### Physics (Box2D v3.1.1)

- **Fixed 60Hz** physics step regardless of render frame rate. `_physicsRemainder` accumulates fractional dt each frame.
- **`Vehicle`** — 4 Box2D bodies: chassis, driver head (circle sensor), front wheel, rear wheel. Wheels connected via prismatic joints. Drive torque applied to wheels; air-control torque applied directly to chassis angular velocity.
- **Render interpolation** — `Vehicle::recordPhysicsSnapshot()` captures prev/cur positions and rotations after each physics step. When rendering at >60fps, `draw()` lerps between snapshots using `renderAlpha()` for smooth motion.
- **`TerrainSystem`** — Terrain as a chain of Box2D chain-shape bodies, generated procedurally from `TerrainProfile` parameters. Streams: points are appended ahead of the camera and trimmed behind.
- **Pickups** — 9 standard pickup types (fuel, coin, flea, rocket, cactus, snowman, giant flea, helmet, magnet) plus squid (a standalone visual effect). All standard pickups share a **CRTP base** (`PickupCollection<Derived>` in `pickup_base.hpp`) that provides `clear()`, `trim()`, `collectByShape()`, `collectOverlaps()`, `activeNear()`, `activeInRange()`, `forceSpawnAt()`, and `createSensorBody()`. Each derived class only needs to provide an `Item` struct and hooks (`items()`, `nextX()`, `itemPos()`, `itemTrimX()`, `pickupDistance()`, `doCreate()`, `doCollect()`). `PickupSystem::forEachStandard()` uses a variadic template to dispatch over all 9 members — the canonical way to add a new pickup type. Collection uses overlap tests (AABB query) against vehicle shapes plus a distance-based fallback (`collectOverlaps`). Placement is biome-aware: giant flea only spawns on stone, helmet only on snow.
- **Helmet revive** — When the helmet pickup is active, a head-hit that would normally end the run instead triggers a rescue: the helmet is consumed, the car gets brief invincibility (`_invincibleTimer`), and the run continues. The `_helmetRescuedThisFrame` flag prevents multi-segment head hits from triggering multiple rescues in one frame. The driver sprite switches to a helmeted variant while protected.
- **Magnet pickup** — When collected, attracts nearby coins toward the car for `kMagnetDuration` seconds (configurable range `kMagnetAttractRadius` and speed `kMagnetAttractSpeed`). `CoinPickups::attractCoins()` handles the attraction physics.
- **PickupEffects** — `PickupEffects` handles one-shot visual effects (puffs/particles) when pickups are collected or triggered. Its `Kind` enum covers all pickup types; each spawns a randomized burst. Effects update and draw independently from the pickup objects themselves.
- **Dust** — `RidgeDashGame` tracks wheel-ground contact dust particles (`updateDust()` / `drawDust()`). Additional dust puffs spawn on flea jumps and other events through `PickupEffects`.

### World generation

- **Terrain profiles** (`src/game/world/terrain_profiles.hpp`) — 7 profiles across 4 biomes (Mountain: rolling/ridges/steps/valley; Stone; Desert; Snow). Each profile defines length ranges, slope bounds, noise parameters, bump/kick/break chances. The terrain system selects a random profile, generates a segment, then picks the next profile — creating varied, seamless terrain.
- **Pickup streaming** — Each pickup type independently decides placement along the terrain using configurable `k*GenerateAhead` constants. Placement uses randomized gaps and terrain-height-aware positioning.

### Biome rendering (`src/game/world/biome/`)

`IBiomeRenderer` is an abstract interface with four concrete singletons: `MountainRenderer`, `StoneRenderer`, `DesertRenderer`, `SnowRenderer`. Each biome provides:
- **`friction()`** — terrain surface friction coefficient
- **`terrainColors()`** — `TerrainColors` struct (soil, soilLight, soilDark, subTop, top, marker) for drawing the layered terrain surface
- **`surfaceThickness()`** — thickness of the colored surface cap
- **`drawMarkers()`** — biome-specific surface detail marks (grass tufts, cracks, etc.)
- **`drawHorizon()`** — distant mountain/background silhouette in the biome's color palette

The `Environment` class selects the active renderer via `biomeRendererFor(biome)` and delegates background/horizon drawing to it. The `TerrainSystem` uses the renderer for surface colors and friction. Biome transitions are driven by the terrain system as it generates segments from different biomes.

### Run state machine (`RunController`)

States: `Ready` → `Running` → `Paused` / `GameOver`.

- `Ready`: start tips shown; first throttle/brake input starts the run.
- `Running`: fuel burns, stall/crash detection runs. Head hit → immediate game over. Extended inverted stall (chassis angle > ~100° for >2s while stopped) → game over. Game over triggers a 1.5s freeze-frame then shows the summary panel.
- `Paused`: entered via Esc; pause menu with resume/restart/quit, display scale, CRT toggle, BGM/SFX toggles (in sub-menus: Main / Video / Audio).
- Score = max distance traveled + coin score + trick bonus. `TrickTracker` counts front flips and back flips separately; bonus is awarded per flip landed.
- `RunRecords` persists best-ever score/coins/flips to a save file. `RunStats` tracks the current run's stats.

### Save data (`src/game/run/save_data.hpp/.cpp`)

Two persisted records stored under `$XDG_STATE_HOME/ridgedash/` (falls back to `~/.local/state/ridgedash/`):

- **`GameRecords`** — best-ever score, coins, flips, frontFlips, backFlips → `records.txt`
- **`GameSettings`** — displayScale, crtEnabled, bgmOn, sfxOn → `settings.txt`

Loaded at startup, saved whenever changed. The pause menu's audio/video settings persist across sessions.

### Audio (`AudioSystem`)

Compiled only with `RIDGEDASH_ENABLE_AUDIO` (Desktop/Web/DRM); no-ops on FBDEV.

- **Engine** — A looping WAV whose pitch and volume are modulated by vehicle speed and throttle/brake input.
- **BGM** — Two-track crossfade (calm/intense) based on vehicle speed with hysteresis and dwell timers. Tracks are shuffled per category with no back-to-back repeats.
- **SFX** — One-shot sounds with variant groups (multiple .wav files per event, one picked at random): Coin, Fuel, Cactus, Snowman, Rocket, RocketFinish, FleaJump, CarLanding, CarHit, CarFlip, UiSelect. Source `.bfxr` files are stored alongside the .wav files.

### UI (`GameUi`, `PauseMenuController`)

Uses `smooth_ui_toolkit` for animated panel transitions (slide + fade). The UI layer draws at the game's fixed 320×170 coordinate space. `GameInput` abstracts keyboard polling into three categories: `Drive` (throttle/brake), `Commands` (pause/reset/confirm/squid), and `Menu` (directional navigation). On DRM targets, input goes through `evdev` instead of raylib's GLFW backend.

`PauseMenuController` manages a three-level menu hierarchy (`MenuLevel::Main` → `Video` / `Audio`) with settings for display scale, CRT toggle, BGM toggle, and SFX toggle. Settings changes are tracked via a dirty flag and persisted through `saveGameSettings()`.

### Platform abstraction (`src/platform/`)

- **`raylib_compat.hpp`** — For FBDEV builds (no raylib headers available), provides stub type definitions (`Vector2`, `Rectangle`, `Color`, `Texture2D`) and declares the minimal raylib API surface the game needs. For real raylib builds, it includes `<raylib.h>` and sets up evdev input if `RIDGEDASH_USE_EVDEV_INPUT` is defined.
- **`fbdev_raylib_compat.cpp`** — FBDEV framebuffer rendering implementation (no raylib dependency). Provides the actual drawing and window-surface logic for framebuffer-only targets.
- **`native_window.hpp` + `native_window_mac.mm`** — macOS Cocoa hooks for native fullscreen (real fullscreen Space vs. raylib's borderless window). `native_window_stub.cpp` is the no-op implementation for other platforms.
- **`evdev_input_compat.cpp`** — Reads keyboard events from `/dev/input/event*` devices; used on DRM builds where GLFW is unavailable.

### Compile definitions

Set by CMake based on `RIDGEDASH_RAYLIB_PLATFORM`:
- `RIDGEDASH_DESKTOP_RENDER` — Desktop build (GLFW + raylib audio)
- `RIDGEDASH_DRM_RENDER` — DRM/KMS build (GPU rendering, audio enabled)
- `RIDGEDASH_USE_FBDEV` — Raw framebuffer build (no raylib, no audio)
- `RIDGEDASH_WEB` — Emscripten build
- `RIDGEDASH_ENABLE_AUDIO` — Audio enabled (Desktop + Web + DRM)
- `RIDGEDASH_USE_EVDEV_INPUT` — evdev keyboard input (DRM builds)
- `RIDGEDASH_SCREEN_WIDTH` / `RIDGEDASH_SCREEN_HEIGHT` — Fixed at 320×170

### CI/CD

GitHub Actions workflow (`.github/workflows/web.yml`) builds the Emscripten/Web target with Emscripten 3.1.74 and deploys to GitHub Pages. Manual-dispatch only (not triggered on push).

## Key constants

All tunable game parameters live in `src/game/game_config.hpp`: pixels-per-meter conversion, terrain step size, wheel/head/pickup radii, speeds, torques, fuel burn rate, coin cluster layout, BGM switching thresholds, etc. Physics and gameplay tuning should happen there.

## Dependencies

| Dependency | Version | Purpose |
|---|---|---|
| raylib | 5.5 | Rendering, windowing, audio |
| box2d | v3.1.1 | 2D physics |
| smooth_ui_toolkit | v2.13.0 | Animated UI transitions |

All three are cloned by `fetch_repos.py` into `dependencies/` and built via `add_subdirectory` in CMake.

## Code style

A `.clang-format` file (BasedOnStyle: LLVM, 4-space indent, 120 columns) is at the repo root. After modifying any `.cpp` / `.hpp` / `.h` / `.mm` file, run:

```bash
clang-format -i <file>
```
