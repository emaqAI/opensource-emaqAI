#include <inline_c.h>
#include "police.h"
#include "fixed.h"

static int precinct_x, precinct_z;

static void spawn_car(PoliceState *p, World *w, int slot) {
	PoliceCar *pc = &p->cars[slot];
	vehicle_init(&pc->veh, precinct_x, precinct_z, 0, 20, 20, 210);
	pc->veh.tex = texture_get(TEX_CAR_POLICE);
	pc->active = 1;
	(void) w;
}

void police_init(PoliceState *p, World *w) {
	world_tile_center(w, 0, 0, &precinct_x, &precinct_z);

	p->wanted = 0;
	p->evade_timer = 0;
	p->heat_cooldown = 0;
	for (int i = 0; i < MAX_POLICE; i++)
		p->cars[i].active = 0;
}

static void sync_active_cars(PoliceState *p, World *w) {
	int want = p->wanted;
	if (want > MAX_POLICE)
		want = MAX_POLICE;

	int active_count = 0;
	for (int i = 0; i < MAX_POLICE; i++)
		if (p->cars[i].active)
			active_count++;

	for (int i = 0; (i < MAX_POLICE) && (active_count < want); i++) {
		if (!p->cars[i].active) {
			spawn_car(p, w, i);
			active_count++;
		}
	}

	if (active_count > want) {
		for (int i = MAX_POLICE - 1; (i >= 0) && (active_count > want); i--) {
			if (p->cars[i].active) {
				p->cars[i].active = 0;
				active_count--;
			}
		}
	}
}

void police_add_heat(PoliceState *p, World *w, int amount) {
	if (p->heat_cooldown > 0)
		return;

	p->wanted = iclamp(p->wanted + amount, 0, WANTED_MAX);
	p->evade_timer = 0;
	p->heat_cooldown = HEAT_COOLDOWN;
	sync_active_cars(p, w);
}

void police_set_level(PoliceState *p, World *w, int level) {
	p->wanted = iclamp(level, 0, WANTED_MAX);
	p->evade_timer = 0;
	sync_active_cars(p, w);
}

void police_update(PoliceState *p, World *w, Vehicle *player) {
	if (p->heat_cooldown > 0)
		p->heat_cooldown--;

	if (p->wanted > 0) {
		if (police_is_near(p, vehicle_world_x(player), vehicle_world_z(player), EVADE_RADIUS)) {
			p->evade_timer = 0;
		} else {
			p->evade_timer++;
			if (p->evade_timer >= EVADE_TIME) {
				p->wanted--;
				p->evade_timer = 0;
				sync_active_cars(p, w);
			}
		}
	}

	for (int i = 0; i < MAX_POLICE; i++) {
		PoliceCar *pc = &p->cars[i];
		if (!pc->active)
			continue;

		int dx = vehicle_world_x(player) - vehicle_world_x(&pc->veh);
		int dz = vehicle_world_z(player) - vehicle_world_z(&pc->veh);
		int steer = ai_steer_toward(pc->veh.heading, dx, dz);

		vehicle_update(&pc->veh, w, 1, steer, 0);
	}
}

void police_draw(PoliceState *p, RenderContext *ctx, MATRIX *cam_mtx, RECT *clip) {
	for (int i = 0; i < MAX_POLICE; i++)
		if (p->cars[i].active)
			vehicle_draw(&p->cars[i].veh, ctx, cam_mtx, clip);
}

int police_is_near(PoliceState *p, int x, int z, int radius) {
	for (int i = 0; i < MAX_POLICE; i++) {
		PoliceCar *pc = &p->cars[i];
		if (!pc->active)
			continue;

		int dx = x - vehicle_world_x(&pc->veh);
		int dz = z - vehicle_world_z(&pc->veh);
		if ((dx * dx + dz * dz) < (radius * radius))
			return 1;
	}
	return 0;
}
