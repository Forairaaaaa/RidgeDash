# RidgeDash

类似于 [登山赛车](https://en.wikipedia.org/wiki/Hill_Climb_Racing) 的复古像素风 2D 物理驾驶小游戏

A retro pixel-art 2D side-scrolling physics-based driving game inspired by [Hill Climb Racing](https://en.wikipedia.org/wiki/Hill_Climb_Racing)

- 油门和刹车双按键操作，小车物理模拟
- 无尽地图，山地、沙漠、石林、雪山等不同地形
- 金币、燃油罐、跳蚤、火箭、雪人、仙人掌等拾取物
- 里程记分，燃油管理以及空翻特技加分
- 8Bit 风格音效、BGM 和动态引擎声
- CRT 着色器，高帧率渲染插值，窗口动态调整

<img width="865" height="471" alt="SCR-20260717-ugdj" src="https://github.com/user-attachments/assets/80d97ea3-a28d-473e-9511-5156e1890cad" />

<img width="860" height="466" alt="SCR-20260717-ugku" src="https://github.com/user-attachments/assets/82233c8c-beb1-4a2f-99f6-7b32d80fa9e2" />

<img width="863" height="468" alt="SCR-20260717-ugtg" src="https://github.com/user-attachments/assets/02c58aed-02a0-4128-ba3d-0dea7a5a093e" />

<img width="865" height="465" alt="SCR-20260717-ugzf" src="https://github.com/user-attachments/assets/6ae9af8d-4027-489c-a65b-4248c1da99ed" />

## 操作

- `Right` / `Space` / `D` ：油门
- `Left` / `A` ：刹车 / 倒车
- `R`：重开
- `Esc`：暂停菜单

## 美术

### Sprites

贴图都是在 Figma 里画的（给自己上难度这一块），工程源文件在 `art.fig`，可以直接导入 Figma 打开

<img width="1261" height="533" alt="SCR-20260717-uife" src="https://github.com/user-attachments/assets/0ea1d06a-950f-47e4-b519-cb55f42b1bfb" />

游戏背景的天空、大海、云朵、星星和山体是直接 raylib API 画的

### CRT Shader

一直很喜欢动物井和小丑牌的 CRT 滤镜效果，这次刚好有机会试试～

参考视频和代码：https://www.youtube.com/watch?v=28u6RoYiCWI

## 音效

音效统一为 8Bit 风格

### SFX

所有道具以及小车 SFX 都是在 [bfxr](https://www.bfxr.net/) 在线效果器上调出来的，非常好用～

音效文件夹 `assets/audio/sfx` 里都留有音效的 `.bfxr` 源数据格式，可以方便导回去调参修改

引擎动态音效：是从 [Freesound](https://freesound.org/people/EVRetro/sounds/519073/) 里截取了一段可 loop 的引擎音效，然后程序根据 input 和小车速度等状态，动态改变音量和音调

### BGM

BGM 来自 [Not Jam](https://not-jam.itch.io/not-jam-music-pack)，有个状态机来维护平静和刺激的 BGM 切换，以及每一轮的 BGM 随机抽取

## 构建

### 依赖

首先运行依赖拉取脚本：

```bash
python3 fetch_repos.py
```

### 桌面端（Linux / macOS）

#### 编译

```bash
cmake -S . -B build/desktop -DRIDGEDASH_RAYLIB_PLATFORM=Desktop
cmake --build build/desktop -j8
```

#### 运行

```bash
./dist/RidgeDash
```

##### 窗口大小

桌面窗口默认以 4 倍显示缩放，可以环境变量指定默认：

```bash
RIDGEDASH_WINDOW_SCALE=fullscreen ./dist/RidgeDash
```

`RIDGEDASH_RENDER_SCALE` 控制 raylib 目标的超采样倍率，Desktop 与 DRM 默认 2x、FBDEV 保持 1x

桌面端渲染目标至少取窗口缩放倍率，保证放大窗口后依然清晰

##### 渲染帧率

`RIDGEDASH_TARGET_FPS` 设置桌面渲染帧率，默认 `120`，取值钳制在 `30`–`240`。设为 `0` 或 `unlimited` 则不限帧率：

```bash
RIDGEDASH_TARGET_FPS=90 ./dist/RidgeDash
RIDGEDASH_TARGET_FPS=unlimited ./dist/RidgeDash
```

无论渲染帧率多少，物理帧率始终固定 60Hz，渲染会在物理步之间插值

##### 地形/道具调试

调试地形时，可在运行时强制指定某个地形剖面：

```bash
RIDGEDASH_TEST_TERRAIN=desert ./dist/RidgeDash
```

支持的值包括 `mountain`、`stone`、`desert`、`snow`，以及精确的剖面名如 `rolling`、`ridges`、`steps`、`valley`

留空则为正常随机地形

调试道具时，可在开局强制生成一个目标道具：

```bash
RIDGEDASH_TEST_PICKUP=helmet ./dist/RidgeDash
```

支持的值：`fuel` `coin` `flea` `rocket` `cactus` `snowman` `giantflea` `helmet`

可与 `RIDGEDASH_TEST_TERRAIN` 组合使用

## 打包

### macOS（.app）

在 macOS（Apple Silicon）上构建一个 ad-hoc 的 `RidgeDash.app` 并打包成 zip：

```bash
./packaging/macos/package_macos.sh
```

打包产物：

```text
dist/artifacts/RidgeDash-macos-arm64.zip
```

该 app 仅 arm64、**未签名**，所以首次打开需要右键 → 打开（或执行`xattr -dr com.apple.quarantine RidgeDash.app`）以绕过 Gatekeeper

### Linux（.tar.gz）

构建 Linux 桌面版打包：

```bash
./packaging/linux/package_linux.sh
```

打包产物：

```text
dist/artifacts/RidgeDash-linux-x86_64.tar.gz
```

这是可移动的目录包，但仍依赖目标系统提供 glibc、图形和音频运行库。为获得更好的发行版兼容性，
请在计划支持的最旧 Linux 发行版上构建。脚本会用 `ldd` 检查是否存在缺失依赖，并报告产物要求的
最高 glibc 符号版本。

解压后可直接运行 `./RidgeDash`。如需添加当前用户的应用启动器：

```bash
./install_desktop.sh
# 移除启动器：
./install_desktop.sh --uninstall
```

移动解压目录后需要重新运行安装脚本，以刷新启动器中的绝对路径。

### Windows（.zip）

在 Linux 上用 MinGW-w64 交叉编译 Windows 版打包：

```bash
# 先安装 MinGW（如未安装）
sudo apt install mingw-w64

./packaging/windows/package_windows.sh
```

打包产物：

```text
dist/artifacts/RidgeDash-windows-x86_64.zip
```

打包脚本会检查生成程序的 MinGW 运行库依赖，并在需要时自动附带对应 DLL。

### CardputerZero（Deb）

构建 CardputerZero 的 `.deb`：

```bash
./packaging/cardputer/package_cardputer.sh
```

脚本默认 `RIDGEDASH_PACKAGE_PLATFORM=AUTO`，会构建一个小的启动器包装 + 两种渲染后端：

- 优先尝试 `DRM`（GPU/KMS 加速）。
- `DRM` 初始化失败时自动回退 `FBDEV`。

需要强制某一后端时可覆盖：

```bash
RIDGEDASH_PACKAGE_PLATFORM=FBDEV ./packaging/cardputer/package_cardputer.sh
RIDGEDASH_PACKAGE_PLATFORM=Desktop ./packaging/cardputer/package_cardputer.sh
```

运行时强制使用回退渲染器：

```bash
RIDGEDASH_RENDER=fbdev RidgeDash
```

打包产物：

```text
dist/artifacts/m5cardputerzero-ridgedash_x.x.x_m5stack1_arm64.deb
```
