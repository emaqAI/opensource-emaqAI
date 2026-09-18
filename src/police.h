#pragma once

#include "render.h"
#include "world.h"
#include "vehicle.h"

#define MAX_POLICE       3
#define WANTED_MAX       3
#define EVADE_RADIUS     1400
#define EVADE_TIME       (60 * 8) /* ~8 seconds clear of every unit -> heat drops. */

typedef struct {
	Vehicle veh;
	int     active;
} PoliceCar;

typedef struct {
	int       wanted;
	int       evade_timer;
	PoliceCar cars[MAX_POLICE];
} PoliceState;

void police_init(PoliceState *p, World *w);
void police_add_heat(PoliceState *p, World *w, int amount);
void police_set_level(PoliceState *p, World *w, int level);
void police_update(PoliceState *p, World *w, Vehicle *player);
void police_draw(PoliceState *p, RenderContext *ctx, MATRIX *cam_mtx, RECT *clip);

/* Non-zero if any active police car is within 'radius' of (x,z). */
int police_is_near(PoliceState *p, int x, int z, int radius);
