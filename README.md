# RidgeDash

A fast-paced, arcade-style 2D physics driving game controlled with just two
buttons, inspired by [Hill Climb Racing](https://en.wikipedia.org/wiki/Hill_Climb_Racing).

## Dependencies

Fetch the third-party sources once after cloning this repository:

```bash
python3 fetch_repos.py
```

It clones the dependencies listed in `repos.json` into `dependencies/`. Only
`git` and Python 3 (standard library) are required. CMake will also refuse to
configure until the dependencies are present.

System packages expected by the build:

- CMake and a C/C++ compiler
- Desktop build on Linux: OpenGL and X11 development packages
- Desktop build on macOS: no extra packages (uses the system frameworks)
- Device build: framebuffer device (`/dev/fb0`) and evdev input access
- Optional DRM raylib build: DRM/GBM/EGL/GLES development packages
- `aarch64-linux-gnu-gcc/g++` for cross-building the CardputerZero package

Project dependencies pulled from `repos.json`:

- `raylib`
- `box2d`

## Build

### Desktop (Linux / macOS)

For desktop development and testing:

```bash
cmake -S . -B build/desktop -DRIDGEDASH_RAYLIB_PLATFORM=Desktop
cmake --build build/desktop -j8
./dist/RidgeDash
```

raylib selects the appropriate desktop backend automatically (X11/OpenGL on
Linux, Cocoa/OpenGL on macOS).

The desktop window defaults to a 3x display scale while keeping the game logic
at 320x170. Override it with:

```bash
RIDGEDASH_WINDOW_SCALE=4 ./dist/RidgeDash
```

`RIDGEDASH_RENDER_SCALE` controls supersampling for raylib targets. It defaults
to 2x for Desktop and DRM, and remains 1x for FBDEV. On desktop, the render
target uses at least the window scale so enlarged windows stay crisp.

`RIDGEDASH_TARGET_FPS` sets the desktop render frame rate. It defaults to `60`
and is clamped to the `30`–`240` range. Set it to `0` or `unlimited` to uncap
the frame rate:

```bash
RIDGEDASH_TARGET_FPS=120 ./dist/RidgeDash
RIDGEDASH_TARGET_FPS=unlimited ./dist/RidgeDash
```

Physics always runs at a fixed 60Hz regardless of the render rate; rendering
interpolates between physics steps so motion stays smooth at any frame rate.

For terrain/pickup tuning, force a specific terrain profile at runtime:

```bash
RIDGEDASH_TEST_TERRAIN=desert ./dist/RidgeDash
```

Supported values include `mountain`, `stone`, `desert`, `snow`, and exact
profile names such as `rolling`, `ridges`, `steps`, or `valley`. Leave it unset
for normal random terrain.

### CardputerZero

For the CardputerZero framebuffer build:

```bash
cmake -S . -B build/cp0 -DRIDGEDASH_RAYLIB_PLATFORM=FBDEV
cmake --build build/cp0 -j8
```

For a cross build from x86 Linux with the GNU aarch64 toolchain:

```bash
cmake -S . -B build/cp0 \
  -DRIDGEDASH_RAYLIB_PLATFORM=DRM \
  -DCMAKE_TOOLCHAIN_FILE=cmake/aarch64-linux-gnu.cmake
cmake --build build/cp0 -j8
```

Raylib's optional DRM backend links against target `GLESv2`, `EGL`, `DRM`, and `GBM`
libraries. For cross builds, the matching `arm64` development packages must be
available to the cross linker. If your host apt sources do not provide them,
build the package directly on the CardputerZero.

The output binary is `dist/RidgeDash`.

## Controls

- `Right` / `Space` / `D` / `C` / `7`: throttle
- `Left` / `A` / `Z` / `5`: brake/reverse
- `R`: restart
- `Esc`: exit

## Package

Distributable packages are built per target platform. More targets are planned;
each gets its own section below.

### CardputerZero (Debian package)

Build the CardputerZero `.deb`:

```bash
./packaging/deb/package_deb.sh
```

The package script defaults to `RIDGEDASH_PACKAGE_PLATFORM=AUTO`. This builds a
small launcher wrapper plus both render backends:

- `DRM` is tried first for GPU/KMS acceleration.
- `FBDEV` is used automatically if DRM initialization fails.

Override it if you need to force one backend:

```bash
RIDGEDASH_PACKAGE_PLATFORM=FBDEV ./packaging/deb/package_deb.sh
RIDGEDASH_PACKAGE_PLATFORM=Desktop ./packaging/deb/package_deb.sh
```

At runtime, force the fallback renderer with:

```bash
RIDGEDASH_RENDER=fbdev RidgeDash
```

The generated package is written to `dist/`:

```text
dist/m5cardputerzero-ridgedash_0.1.0_m5stack1_arm64.deb
```
