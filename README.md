# Chrome City

An original open-world driving/crime game for the original Sony PlayStation
(PSX/PS1), in the spirit of *Grand Theft Auto III* but built from scratch as
real PS1 homebrew: procedurally generated city, drivable car with arcade
physics, AI traffic and pedestrians, a police "wanted level" system with
pursuing squad cars, and a short 3-mission story campaign with intro/ending
text screens, all rendered with the PS1 GPU/GTE (no PC/GTA code involved
anywhere).

It's written in C against [PSn00bSDK](https://github.com/Lameguy64/PSn00bSDK),
an open-source, from-scratch reimplementation of the PS1 SDK (MPL-2.0
licensed).

## Status / how this was verified

This was built and linked with the real PSn00bSDK 0.24 toolchain
(`mipsel-none-elf-gcc`, GCC 12.3.0, MIPS R3000 target) and produces a valid
PS-EXE (`chromecity.exe`, PC1=`0x800155c0`, load address `0x80010000`, the
standard PS1 user RAM base) and a bootable ISO (`chromecity.bin/.cue`) via
`mkpsxiso`. The build is warning-free at `-Wall -Wextra`. RAM usage is
~133 KB (text+data+bss), a small fraction of the console's 2 MB.

**What I could not verify in this sandbox:** actually booting it. PS1
emulators (mednafen, DuckStation, etc.) and real hardware both require
Sony's copyrighted BIOS ROM, which I don't have and deliberately did not try
to obtain — I'm not going to go looking for a Sony BIOS dump online. So
while I'm confident in the code (it's written closely against PSn00bSDK's
own official examples for the GTE/camera/controller-handling patterns, and
I traced through the fixed-point/rotation math by hand), the interactive,
in-game behavior (camera framing, drift feel, collision feel, on-screen
text layout) has **not been visually confirmed**. Please treat first boot
as the real test, and expect to tweak a few constants (see below) once you
see it move.

If you own a PS1 (or a legally-dumped BIOS from one), drop it into your
emulator's BIOS slot and load `chromecity.cue`, or burn/ship the `.bin/.cue`
to real hardware / an ODE (MODE, PicoStation, etc.).

## Building

You need [CMake](https://cmake.org/) >= 3.21 and PSn00bSDK's prebuilt
toolchain for Linux (or build it yourself — see PSn00bSDK's own docs).

```sh
# One-time setup: grab the prebuilt SDK+toolchain and put it on your PATH.
# (This project was built against PSn00bSDK 0.24.)
export PATH="/opt/psn00bsdk/bin:$PATH"
export PSN00BSDK_LIBS="/opt/psn00bsdk/lib/libpsn00b"

cmake --preset default .
cmake --build ./build
```

This produces, in `build/`:

- `chromecity.exe` — a raw PS-EXE, loadable directly by most emulators.
- `chromecity.bin` / `chromecity.cue` — a bootable CD image.

## Controls

| Input                  | Action                              |
|-------------------------|--------------------------------------|
| D-Pad Up / Left stick up      | Accelerate                     |
| D-Pad Down / Left stick down  | Brake / reverse                |
| D-Pad Left/Right / stick      | Steer                          |
| Square                        | Handbrake (tighter, harder turn) |
| Cross / Start                 | Advance story/briefing text screens |

## What's actually implemented

- **Engine**: double-buffered GTE renderer (`src/render.c`), ordering-table
  depth sorting, screen-space quad clipping adapted from PSn00bSDK's own
  `fpscam` example (`src/clip.c`).
- **World** (`src/world.c`): a procedurally generated Manhattan-style street
  grid (5x5 blocks) with varied building heights/colors and painted lane
  lines, plus AABB collision for buildings.
- **Vehicle physics** (`src/vehicle.c`): fixed-point accel/brake/steer/
  handbrake model shared by the player, traffic and police (same struct,
  different "driver").
- **Camera** (`src/camera.c`): a smoothed third-person chase camera.
- **Traffic AI** (`src/traffic.c`): cars looping fixed street routes,
  wandering-then-scattering pedestrians, both steered without a lookup
  table via a cross-product "steer toward point" trick (see `fixed.h`).
- **Police / wanted level** (`src/police.c`): 0-3 star wanted level, heat
  from hitting traffic/pedestrians, spawns pursuing squad cars, decays
  after enough time spent clear of them.
- **Story/mission state machine** (`src/mission.c`): title screen -> intro
  -> 3 missions (drive-to-contact, evade-the-cops-on-a-timer, cross-town
  delivery) each with a briefing/result text screen -> ending -> free roam.
- **HUD** (`src/hud.c`): speed, wanted stars, live objective distance /
  mission timer.

## Known simplifications (by design, not oversights)

- No textures — flat-shaded polygons only, keeps the whole thing simple
  and guaranteed to fit comfortably in VRAM/RAM.
- No true drift/slip physics — the handbrake tightens the turn radius and
  brakes hard, but the car's velocity always points along its heading.
- No memory card save. PSn00bSDK 0.24 doesn't ship a high-level card API
  and I didn't want to hand-roll unverified low-level card I/O; progress
  resets on power-off. The BIOS `card` functions in `psxapi.h` are the
  place to start if you want to add it.
- Missions can't be "failed", only completed faster or slower — this
  avoids needing a retry/game-over flow, keeping the loop always moving
  forward.

## Tuning

Most of the "feel" constants live at the top of `src/vehicle.h`
(`VEH_MAX_SPEED`, `VEH_ACCEL`, `VEH_TURN_RATE`, ...), `src/camera.h`
(`CAM_DIST`, `CAM_HEIGHT`, `CAM_PITCH`) and `src/police.h`
(`EVADE_RADIUS`, `EVADE_TIME`). Once you can actually see it running,
those are the first things worth nudging.
