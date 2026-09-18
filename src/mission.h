#pragma once

#include "render.h"
#include "world.h"
#include "vehicle.h"
#include "police.h"
#include "input.h"

typedef enum {
	STAGE_TITLE = 0,
	STAGE_INTRO_1,
	STAGE_INTRO_2,
	STAGE_M1_BRIEF,
	STAGE_M1_PLAY,
	STAGE_M1_WIN,
	STAGE_M2_BRIEF,
	STAGE_M2_PLAY,
	STAGE_M2_WIN,
	STAGE_M3_BRIEF,
	STAGE_M3_PLAY,
	STAGE_M3_WIN,
	STAGE_ENDING,
	STAGE_FREEROAM
} Stage;

typedef struct {
	Stage stage;
	int   timer;          /* Frames since the current stage began.       */
	int   target_x, target_z;
	int   objective_radius;
	int   mission_timer;  /* Countdown in frames, -1 if the stage has none. */
	int   m3_on_time;     /* Set if mission 3 was completed before time ran out. */
} Mission;

void mission_init(Mission *m, World *w);

/* Advances the mission/story state machine by one frame. May start or stop
 * gameplay systems (e.g. spawn police for mission 2) as a side effect. */
void mission_update(Mission *m, InputState *in, Vehicle *player, World *w, PoliceState *police);

/* Non-zero while the current stage is interactive driving gameplay (as
 * opposed to a full-screen text/briefing screen). */
int mission_is_gameplay(Mission *m);

/* Draws the current stage's full-screen text, if any (title, story beats,
 * mission briefings/results, ending). Does not call FntFlush(). */
void mission_draw_overlay(Mission *m);
