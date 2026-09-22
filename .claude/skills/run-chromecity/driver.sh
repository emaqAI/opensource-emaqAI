#!/usr/bin/env bash
# Driver for building, launching and driving Chrome City (PS1 homebrew) in
# PCSX-Redux, headless, inside this container. See SKILL.md for the guided
# walkthrough - this file is the actual mechanism.
#
# PCSX-Redux is a native GUI app with no CLI-scriptable interface, so unlike
# a web app (chromium-cli) or a server (curl) the "driver" here is: a real
# X server (Xvfb) + synthetic keyboard input (xdotool) + screen capture
# (ImageMagick's `import`). Each subcommand below is a thin, stateless
# wrapper around those three tools - state lives in the running Xvfb/docker
# processes, not in this script, so there's no REPL/tmux session to manage.
#
# Usage:
#   driver.sh setup                    # one-time: docker image + pcsx-redux + OpenBIOS
#   driver.sh build                    # cmake build the game -> build/chromecity.exe
#   driver.sh launch                   # start Xvfb + docker container + pcsx-redux
#   driver.sh key <keyname> [count]    # tap a key N times (default 1), e.g. `key x 3`
#   driver.sh hold <keyname> <seconds> # hold a key down for N seconds, e.g. `hold Up 2`
#   driver.sh combo <k1> <k2> <seconds> # hold two keys together, e.g. `combo x Left 1.5`
#   driver.sh screenshot <out.png>     # save a screenshot
#   driver.sh stop                     # kill the emulator + Xvfb + container
#
# All paths below are relative to the repo root; run this from there, or it
# resolves its own location and cd's for you.

set -euo pipefail

SKILL_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "$SKILL_DIR/../../.." && pwd)"
PCSX_CACHE="${PCSX_CACHE:-/tmp/pcsx-redux-cache}"
CONTAINER_NAME="${CONTAINER_NAME:-chromecity-pcsx}"
DISPLAY_NUM="${DISPLAY_NUM:-:97}"

log() { echo "[driver] $*" >&2; }

cmd_setup() {
	log "Checking docker daemon..."
	if ! docker info >/dev/null 2>&1; then
		log "Starting dockerd..."
		rm -f /var/run/docker.pid
		dockerd >/tmp/dockerd.log 2>&1 &
		for _ in $(seq 1 30); do docker info >/dev/null 2>&1 && break; sleep 1; done
		docker info >/dev/null 2>&1 || { log "dockerd failed to start, see /tmp/dockerd.log"; exit 1; }
	fi

	log "Ensuring pcsx-redux-build image is present..."
	docker image inspect ghcr.io/grumpycoders/pcsx-redux-build:latest >/dev/null 2>&1 \
		|| docker pull ghcr.io/grumpycoders/pcsx-redux-build:latest

	if [ ! -x "$PCSX_CACHE/pcsx-redux" ] || [ ! -f "$PCSX_CACHE/src/mips/openbios/openbios.bin" ]; then
		log "Building pcsx-redux + OpenBIOS into $PCSX_CACHE (one-time, several minutes)..."
		rm -rf "$PCSX_CACHE"
		git clone --recurse-submodules https://github.com/grumpycoders/pcsx-redux.git "$PCSX_CACHE"
		( cd "$PCSX_CACHE" && ./dockermake.sh )
		( cd "$PCSX_CACHE" && PATH="/opt/psn00bsdk/bin:$PATH" make -C src/mips/openbios -j"$(nproc)" )
	else
		log "pcsx-redux + OpenBIOS already cached at $PCSX_CACHE."
	fi

	log "Setup complete."
}

cmd_build() {
	log "Building the game..."
	cd "$REPO_ROOT"
	export PATH="/opt/psn00bsdk/bin:$PATH"
	export PSN00BSDK_LIBS="/opt/psn00bsdk/lib/libpsn00b"
	if [ ! -d build ]; then
		cmake --preset default .
	fi
	cmake --build ./build
	log "Built build/chromecity.exe"
}

cmd_launch() {
	[ -f "$REPO_ROOT/build/chromecity.exe" ] || { log "No build/chromecity.exe - run 'driver.sh build' first."; exit 1; }
	[ -x "$PCSX_CACHE/pcsx-redux" ] || { log "No cached pcsx-redux - run 'driver.sh setup' first."; exit 1; }

	if ! pgrep -f "Xvfb $DISPLAY_NUM " >/dev/null 2>&1; then
		log "Starting Xvfb on $DISPLAY_NUM..."
		Xvfb "$DISPLAY_NUM" -screen 0 1280x720x24+32 >/tmp/xvfb-chromecity.log 2>&1 &
		sleep 2
	fi

	docker rm -f "$CONTAINER_NAME" >/dev/null 2>&1 || true
	log "Starting pcsx-redux container..."
	docker run --rm -d --name "$CONTAINER_NAME" \
		-v "$PCSX_CACHE:/project" \
		-v "$REPO_ROOT:/game" \
		-v /tmp/.X11-unix:/tmp/.X11-unix \
		-w /project \
		ghcr.io/grumpycoders/pcsx-redux-build:latest sleep infinity >/dev/null

	docker exec -d -e DISPLAY="$DISPLAY_NUM" -e LIBGL_ALWAYS_SOFTWARE=1 \
		-e SDL_VIDEO_FORCE_EGL=1 -e XDG_RUNTIME_DIR=/tmp/xdgrt -w /project "$CONTAINER_NAME" \
		sh -c "mkdir -p /tmp/xdgrt && chmod 700 /tmp/xdgrt && ./pcsx-redux -run -loadexe /game/build/chromecity.exe -safe -noupdate -stdout > /tmp/pcsx.log 2>&1"

	log "Waiting for the PCSX-Redux window..."
	WIN=""
	for _ in $(seq 1 40); do
		WIN="$(DISPLAY="$DISPLAY_NUM" xdotool search --name "PCSX-Redux" 2>/dev/null | head -1 || true)"
		[ -n "$WIN" ] && break
		sleep 1
	done
	[ -n "${WIN:-}" ] || { log "Window never appeared - check: docker exec $CONTAINER_NAME cat /tmp/pcsx.log"; exit 1; }
	echo "$WIN" > /tmp/chromecity-pcsx-win
	log "Launched. Window id $WIN."
}

_win() {
	if [ -f /tmp/chromecity-pcsx-win ]; then cat /tmp/chromecity-pcsx-win; else
		DISPLAY="$DISPLAY_NUM" xdotool search --name "PCSX-Redux" 2>/dev/null | head -1 || true
	fi
}

cmd_key() {
	local key="${1:?keyname required}" count="${2:-1}"
	local win; win="$(_win)"
	for _ in $(seq 1 "$count"); do
		DISPLAY="$DISPLAY_NUM" xdotool key --window "$win" "$key"
		sleep 0.3
	done
}

cmd_hold() {
	local key="${1:?keyname required}" secs="${2:?seconds required}"
	local win; win="$(_win)"
	DISPLAY="$DISPLAY_NUM" xdotool keydown --window "$win" "$key"
	sleep "$secs"
	DISPLAY="$DISPLAY_NUM" xdotool keyup --window "$win" "$key"
}

cmd_combo() {
	local k1="${1:?key1 required}" k2="${2:?key2 required}" secs="${3:?seconds required}"
	local win; win="$(_win)"
	DISPLAY="$DISPLAY_NUM" xdotool keydown --window "$win" "$k1"
	DISPLAY="$DISPLAY_NUM" xdotool keydown --window "$win" "$k2"
	sleep "$secs"
	DISPLAY="$DISPLAY_NUM" xdotool keyup --window "$win" "$k2"
	DISPLAY="$DISPLAY_NUM" xdotool keyup --window "$win" "$k1"
}

cmd_screenshot() {
	local out="${1:?output path required}"
	import -display "$DISPLAY_NUM" -window root "$out"
	log "Saved $out"
}

cmd_stop() {
	docker exec "$CONTAINER_NAME" sh -c "pkill -f pcsx-redux" >/dev/null 2>&1 || true
	docker rm -f "$CONTAINER_NAME" >/dev/null 2>&1 || true
	pkill -f "Xvfb $DISPLAY_NUM " >/dev/null 2>&1 || true
	rm -f /tmp/chromecity-pcsx-win
	log "Stopped."
}

case "${1:-}" in
	setup)      cmd_setup ;;
	build)      cmd_build ;;
	launch)     cmd_launch ;;
	key)        shift; cmd_key "$@" ;;
	hold)       shift; cmd_hold "$@" ;;
	combo)      shift; cmd_combo "$@" ;;
	screenshot) shift; cmd_screenshot "$@" ;;
	stop)       cmd_stop ;;
	*)
		echo "Usage: $0 {setup|build|launch|key|hold|combo|screenshot|stop} [args...]" >&2
		exit 1
		;;
esac
