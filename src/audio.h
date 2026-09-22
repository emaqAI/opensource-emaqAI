#pragma once

/* Uploads all SFX to SPU RAM and starts the engine loop. Call once after
 * ResetGraph(). */
void audio_init(void);

/* Re-pitches/re-volumes the always-playing engine loop to track the
 * player's current speed (same units as Vehicle.speed). Call every frame. */
void audio_engine_update(int speed);

/* Starts/stops the looping siren. No-op if already in that state, so it's
 * safe to call every frame with the current wanted-level state. */
void audio_set_siren(int on);

/* Retriggers the one-shot crash/impact sound. */
void audio_play_crash(void);
