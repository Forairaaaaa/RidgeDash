# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Build

### Dependencies

```bash
python3 fetch_repos.py
```

Pulls raylib 5.5, box2d v3.1.1, and smooth_ui_toolkit v2.13.0 into `dependencies/`. Defined in `repos.json`.

### Desktop (Linux/macOS)

```bash
cmake -S . -B build/desktop -DRIDGEDASH_RAYLIB_PLATFORM=Desktop
cmake --build build/desktop -j8
```

Binary lands at `dist/RidgeDash` with `dist/assets/` copied alongside.

### Web (Emscripten)

```bash
emcmake cmake -B build/web -DRIDGEDASH_RAYLIB_PLATFORM=Web -DCMAKE_BUILD_TYPE=Release
cmake --build build/web -j$(nproc)
```

Output (`index.html`, `.js`, `.wasm`, `.data`) goes to `dist/`.

### Embedded (DRM / FBDEV)

DRM (GPU-accelerated):
```bash
cmake -S . -B build/drm -DRIDGEDASH_RAYLIB_PLATFORM=DRM
cmake --build build/drm -j8
```

FBDEV (software framebuffer):
```bash
cmake -S . -B build/fbdev -DRIDGEDASH_RAYLIB_PLATFORM=FBDEV
cmake --build build/fbdev -j8
```

Cross-compile for aarch64 with `-DCMAKE_TOOLCHAIN_FILE=cmake/aarch64-linux-gnu.cmake`.

### Packaging

macOS `.app` bundle (must run on macOS):
```bash
./packaging/macos/package_macos.sh
```

CardputerZero `.deb`:
```bash
./packaging/cardputer/package_cardputer.sh
```

## Architecture

### Game loop and entry point

`src/main.cpp` — Owns the raylib window, render-to-texture FBO, CRT shader, `BloomPipeline`, and the main loop. Offloads all game logic to `RidgeDashGame::update()` / `draw()`. The loop blits the FBO to the window through the optional CRT+bloom post-process chain.

`src/main_web.cpp` — Emscripten variant; replaces the blocking main loop with `emscripten_set_main_loop`.

### Core game (`src/game/`)

**`RidgeDashGame`** (`ridge_dash_game.hpp/cpp`) is the central orchestrator. Owns every subsystem by value. The `update()` method runs them in a fixed order:

1. Poll input → check pause/reset/game-over state transitions
2. Stream terrain + pickups ahead of the car, cull behind
3. Apply drive input, step physics at fixed 60Hz (with optional render interpolation), process contacts/sensors
4. Update tricks, dust particles, camera, score, BGM state machine, driver expression

Friend classes (`PickupSystem`, `FuelPickups`, `CoinPickups`, etc.) allow direct access to game state without exposing public setters.

**`RenderHelpers`** (`render_helpers.hpp/cpp`) — Sprite loading from `assets/sprites/`, centered drawing, color utilities.

### Physics (`src/game/vehicle/`)

`Vehicle` builds a Box2D body per car part: chassis (rectangle), two wheels (circles connected via prismatic joints with motor/spring), and a driver head (circle, weld-jointed). The terrain is a chain of Box2D segment bodies.

Physics runs at fixed 60Hz regardless of render frame rate. When rendering faster than physics, `Vehicle::recordPhysicsSnapshot()` stores prev/cur body transforms so `draw()` can interpolate between them (`renderAlpha()`).

### World generation (`src/game/world/`)

`TerrainSystem` procedurally generates an endless 2D height map. A biome picker transitions between `Mountain`, `Stone`, `Desert`, and `Snow`. Each biome has 1-4 `TerrainProfile`s in `terrain_profiles.hpp` that control slope ranges, noise, bumps, kicks, etc. The system works as a state machine: pick a profile → walk segments until its length is reached → pick a new profile (possibly changing biome). Heights are sampled via a slope-blended noise function. Each segment becomes a Box2D static body for collision.

`Environment` handles the parallax background: sky gradient, sea, sun/moon cycle, stars, clouds, sailboat. The day/night cycle is driven by distance traveled (pseudo-time). Biome transitions smoothly crossfade background colors.

### Pickups (`src/game/pickups/`)

`PickupSystem` implements the composite pattern — one public interface delegating to typed sub-systems:

| Sub-system | Behavior |
|---|---|
| `FuelPickups` | Static fuel cans placed on terrain. Collected → refill. |
| `CoinPickups` | Clusters in shapes (lines, waves, arcs, polygons), grounded or airborne. Slope-aware and rocket-coupled placement. |
| `FleaPickups` | Jumping fleas with physics (gravity + bounce). Collected → upward impulse boost. |
| `RocketPickups` | Collected → horizontal rocket flight with particle trail. |
| `CactusPickups` | Static obstacles. Hit → car deceleration + chip particles. |
| `SnowmanPickups` | Static on terrain. Collected → speed boost timer. |
| `SquidPickups` | Easter egg. Triggered by secret input; spawns flying squids with rainbow trails. |

Each sub-system follows the same lifecycle: `stream()` creates items ahead of the car, `trim()` removes items far behind, `collectByShape()`/`collectOverlaps()` handle collection, `update()` animates active items, `draw()` renders them.

`PickupEffects` renders collection puff particles.

### Run state (`src/game/run/`)

`RunController` — Finite state machine: `Ready → Running → Paused / GameOver`. Manages fuel burn/refill, end-condition detection (stall timer, crash timer, head-hit), and game-over countdown.

`RunStats` — Accumulates distance, coin score, and trick score. Score = max(distance + coinScore + trickScore) to prevent post-crash drift.

`RunRecords` — Persists high scores via `save_data.cpp` (JSON file in the app directory).

`TrickTracker` — Detects mid-air flips >250° and fires per-flip scoring events.

### Audio (`src/game/audio/`)

`AudioSystem` — Wraps raylib `Music`/`Sound`. Only compiled when `RIDGEDASH_ENABLE_AUDIO` is defined (Desktop/Web). On DRM/FBDEV every method is a no-op.

- **Engine**: Looping `.wav` with real-time pitch/volume modulation based on speed, throttle, and brake input. Pitch maps to a musical scale.
- **BGM**: Two-tier shuffle system — calm and intense tracks randomly drawn from `assets/audio/music/calm/` and `assets/audio/music/intense/`. Hysteresis state machine switches based on car speed with a dwell timer to prevent rapid toggling. Tracks play through a shuffle bag (no repeats until all played).
- **SFX**: One-shot sounds loaded from `assets/audio/sfx/`. Sources in `.bfxr` format (bfxr.net) can be re-exported to `.wav`.

### UI (`src/game/ui/`)

`GameUi` uses `smooth_ui_toolkit` for animated transitions (start tips slide-in, game-over panel, pause menu). Renders HUD (fuel bar, distance, score), score popups, and the pause menu with display scale and CRT toggle options.

`PauseMenuController` — Menu state machine with scale/CRT settings that get consumed by `main.cpp` to actually change window mode or toggle the shader.

`GameInput` — Unified input abstraction. `poll()` reads keyboard (or evdev on embedded) and maps to `Drive`, `Commands`, and `Menu` structs.

### Platform abstraction (`src/platform/`)

`raylib_compat.hpp` — When `RIDGEDASH_USE_FBDEV` is defined, provides stub types (`Vector2`, `Rectangle`, `Color`, `Texture2D`) and function declarations so the game compiles without linking real raylib. Otherwise includes `<raylib.h>` directly. Also wraps evdev input via macros (`#define IsKeyDown RidgeDashIsKeyDown`).

`native_window.hpp` / `native_window_mac.mm` / `native_window_stub.cpp` — macOS native fullscreen toggle (Cocoa `NSWindow` API). Stub for other platforms.

### Rendering pipeline

1. Game draws to a supersampled `RenderTexture2D` (FBO scale driven by monitor height to stay crisp in fullscreen)
2. `BloomPipeline::runPasses()` — optional multi-pass chain: bright-pass extract → horizontal blur → vertical blur → phosphor persistence (temporal decay + stamp) → composite (persisted scene + bloom)
3. Final blit from the composited texture through `crt.fs` (scanlines, chromatic aberration, vignette) to the window, letterboxed with integer scaling

`crt_es.fs` is the GLES-friendly variant for Web builds.

Six shaders live in `assets/shaders/`: `crt.fs`, `crt_es.fs`, `bloom_brightpass.fs`, `bloom_blur.fs`, `phosphor_persistence.fs`, `composite.fs`.

### Build configuration

Screen resolution is fixed at 320×170 via CMake defines (`RIDGEDASH_SCREEN_WIDTH`, `RIDGEDASH_SCREEN_HEIGHT`). Platform selection is through `RIDGEDASH_RAYLIB_PLATFORM` which gates compile definitions (`RIDGEDASH_DESKTOP_RENDER`, `RIDGEDASH_DRM_RENDER`, `RIDGEDASH_USE_FBDEV`, `RIDGEDASH_ENABLE_AUDIO`, `RIDGEDASH_WEB`, `RIDGEDASH_USE_EVDEV_INPUT`).

Environment variables for desktop runtime tuning: `RIDGEDASH_WINDOW_SCALE`, `RIDGEDASH_RENDER_SCALE`, `RIDGEDASH_TARGET_FPS`, `RIDGEDASH_CRT`, `RIDGEDASH_TEST_TERRAIN`.

## Code conventions

- C++17, C99
- 4-space indent, 120-column limit, `{` on new line after functions (`.clang-format`)
- `snake_case` for variables/functions, `PascalCase` for classes, `kPascalCase` for constants
- RAII for resource ownership; subsystems owned by value, not heap-allocated
- Fixed-step physics with accumulator (`_physicsRemainder`), clamped to max 4 steps/frame
- No exceptions or RTTI; error handling via early returns and validity checks (`b2World_IsValid`, `IsRenderTextureValid`, `IsShaderValid`)