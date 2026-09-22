---
name: run-chromecity
description: Build Chrome City (PS1 homebrew) and drive it headless in PCSX-Redux. Use when asked to run the game, build it, take a screenshot of it, test a gameplay change, or verify a control/camera/rendering fix in-emulator.
---

Chrome City is a real PS1 (PSX) homebrew game (PSn00bSDK, C, no PC/desktop
build target) - there's no way to "run" it except inside a PS1 emulator.
It's driven headless via `.claude/skills/run-chromecity/driver.sh`, which
wraps Xvfb (a virtual X server) + `xdotool` (synthetic keyboard input) +
ImageMagick's `import` (screen capture) around a PCSX-Redux instance
running inside Docker. There is no in-app scripting surface (no web UI, no
CLI) - screenshots are the only way to observe the running game.

All paths below are relative to the repo root.

## Prerequisites

```bash
apt-get update
apt-get install -y xvfb xdotool imagemagick docker.io git
```

PSn00bSDK's prebuilt toolchain must be at `/opt/psn00bsdk` (this is how
this container came configured; if it's missing, see PSn00bSDK's own docs
for the prebuilt-toolchain download).

## Setup (one-time)

```bash
.claude/skills/run-chromecity/driver.sh setup
```

This starts the Docker daemon if it isn't running, pulls
`ghcr.io/grumpycoders/pcsx-redux-build:latest` (the image PCSX-Redux is
built/run inside), and - if not already cached at `/tmp/pcsx-redux-cache`
(override with `PCSX_CACHE`) - clones and builds PCSX-Redux from source
plus OpenBIOS (`grumpycoders/pcsx-redux`, MIT-licensed from-scratch BIOS
reimplementation - **no copyrighted Sony BIOS needed or used**; PCSX-Redux
automatically falls back to OpenBIOS when no retail BIOS is configured).
This build step is slow (several minutes) but only needs to happen once -
subsequent `setup` calls detect the cache and skip straight to "already
cached."

## Build

```bash
.claude/skills/run-chromecity/driver.sh build
```

Runs the real `cmake --preset default . && cmake --build ./build` (same
as a human would), producing `build/chromecity.exe` - a raw PS-EXE loaded
directly by `-loadexe`, no CD image needed for iteration.

## Run (agent path)

```bash
.claude/skills/run-chromecity/driver.sh launch                     # boots the game
.claude/skills/run-chromecity/driver.sh key x 3                    # advance past title/story screens
.claude/skills/run-chromecity/driver.sh hold x 2                   # accelerate for 2s
.claude/skills/run-chromecity/driver.sh combo x Left 1.5           # accelerate + steer left for 1.5s
.claude/skills/run-chromecity/driver.sh screenshot /tmp/shot.png   # capture the frame
.claude/skills/run-chromecity/driver.sh stop                       # tear down container + Xvfb
```

Each subcommand is a stateless call against the already-running Xvfb/
Docker container (no REPL/tmux session to manage) - state lives in those
processes, not in the script. `launch` is the slow one (~15-20s: starts
Xvfb if not already up, starts the container, waits for the PCSX-Redux
window). Everything after that is near-instant.

| command | what it does |
|---|---|
| `setup` | one-time: docker image + PCSX-Redux + OpenBIOS build/cache |
| `build` | `cmake --build` the game -> `build/chromecity.exe` |
| `launch` | start Xvfb + container + PCSX-Redux with `-loadexe build/chromecity.exe` |
| `key <name> [count]` | tap a key N times (default 1) |
| `hold <name> <secs>` | hold a key down for N seconds |
| `combo <k1> <k2> <secs>` | hold two keys together for N seconds (e.g. accelerate + steer) |
| `screenshot <path>` | save a PNG of the whole virtual screen |
| `stop` | kill the emulator, remove the container, kill Xvfb |

### Keyboard mapping

PCSX-Redux's default keyboard bindings map to **SDL scancodes**, which
land on different physical *letter* keys than you'd guess - confirmed by
reading `pcsx.json` inside the running container
(`docker exec <container> grep -A1 Keyboard_Pad /project/pcsx.json`) and
cross-checking against SDL2's scancode table:

| Game input | `xdotool`/driver key name |
|---|---|
| D-Pad Up/Down/Left/Right | `Up` / `Down` / `Left` / `Right` |
| Cross (gas, and menu/story-advance) | `x` |
| Square (brake/reverse) | `z` |
| Circle (handbrake) | `d` |
| Start | usually not needed - Cross also advances menus |

The intro/story screens need `key x 3` (three taps) to reach free-roam
gameplay from a cold boot.

## Run (human path)

If you have a real display (not headless), `PCSX_CACHE/pcsx-redux -run
-loadexe build/chromecity.exe` runs directly with a visible window and
real keyboard/gamepad input - no Xvfb/xdotool needed. See the main
`README.md`'s "Testing it yourself with PCSX-Redux + OpenBIOS" section.

## Gotchas

- **The Docker daemon is not running by default in this container** and
  can leave a stale `/var/run/docker.pid` after being killed mid-session
  (across agent turns, the daemon process itself doesn't survive) -
  `setup`/`launch` don't handle the stale-pidfile case automatically if
  `dockerd` was killed uncleanly; if `docker info` still fails after
  `driver.sh setup`, run `rm -f /var/run/docker.pid` once yourself, then
  retry.
- **`SDL_VIDEO_FORCE_EGL=1` is required** inside the container or
  PCSX-Redux crashes immediately with "Couldn't find matching GLX
  visual" - the driver sets this for you; if you launch PCSX-Redux
  manually for debugging, don't drop it.
- **Xvfb needs `+32` depth** (`-screen 0 1280x720x24+32`) for the same
  GLX-visual-matching reason - a plain `-screen 0 1280x720x24` isn't
  enough.
- **`XDG_RUNTIME_DIR` must exist and be `chmod 700`** or SDL fails to
  init inside the container; the driver creates `/tmp/xdgrt` for this.
- **The container is `--rm`,** so if you kill PCSX-Redux by any means
  other than `driver.sh stop` (e.g. `docker rm -f` from another shell),
  it's gone - `launch` again to get a fresh one, no cleanup needed first.
- **Window title is literally `PCSX-Redux`** (`xdotool search --name`
  matches on that) - if a future PCSX-Redux version renames its window,
  update `_win()` in `driver.sh`.

## Troubleshooting

- **`launch` fails at "Window never appeared"**: check
  `docker exec <container-name> cat /tmp/pcsx.log` first. If the log is
  empty/short and the process is still alive (`docker exec <container>
  ps aux`), it just needs more time - PCSX-Redux's own startup (loading
  OpenBIOS, then `-loadexe`) can take 10-15s on a cold container; the
  driver already waits up to 40s, but a heavily loaded host can exceed
  that - just retry `launch`.
- **`Cannot connect to the Docker daemon`**: see the stale-pidfile
  Gotcha above.
- **pcsx-redux binary errors with `libcapstone.so.5: cannot open shared
  object file`**: you're running the native binary outside the build
  container - always run it via `docker exec ... ghcr.io/grumpycoders/
  pcsx-redux-build:latest`, never directly on the host (that image has
  the matching shared libs; the host doesn't).
