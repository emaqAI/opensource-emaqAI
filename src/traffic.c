#include <stdlib.h>
#include <inline_c.h>
#include "traffic.h"
#include "fixed.h"

#define PED_RESPAWN_TIME 240
#define TRAFFIC_COLOR_COUNT 4

static const uint8_t traffic_colors[TRAFFIC_COLOR_COUNT][3] = {
	{ 210, 200,  40 }, /* taxi yellow  */
	{  60, 120, 200 }, /* blue         */
	{ 200,  60,  60 }, /* red          */
	{  60, 180,  90 }, /* green        */
};

static void add_loop(Traffic *t, World *w, int r0, int c0, int r1, int c1, int seed_offset) {
	TrafficCar *car = &t->cars[t->car_count];
	int x, z;

	world_tile_center(w, r0, c0, &x, &z);
	car->wp_x[0] = x; car->wp_z[0] = z;
	world_tile_center(w, r0, c1, &x, &z);
	car->wp_x[1] = x; car->wp_z[1] = z;
	world_tile_center(w, r1, c1, &x, &z);
	car->wp_x[2] = x; car->wp_z[2] = z;
	world_tile_center(w, r1, c0, &x, &z);
	car->wp_x[3] = x; car->wp_z[3] = z;
	car->wp_count = 4;
	car->wp_index = 1;

	const uint8_t *col = traffic_colors[t->car_count % TRAFFIC_COLOR_COUNT];
	vehicle_init(&car->veh, car->wp_x[0], car->wp_z[0], 0, col[0], col[1], col[2]);
	(void) seed_offset;

	t->car_count++;
}

void traffic_init(Traffic *t, World *w) {
	t->car_count = 0;

	add_loop(t, w, 0, 0, 4, 4, 0);
	add_loop(t, w, 0, 6, 4, 10, 1);
	add_loop(t, w, 6, 0, 10, 4, 2);
	add_loop(t, w, 6, 6, 10, 10, 3);

	static const int ped_building[6][2] = {
		{ 0, 0 }, { 0, 2 }, { 0, 4 }, { 2, 0 }, { 4, 4 }, { 3, 2 }
	};

	t->ped_count = 0;
	for (int i = 0; i < 6; i++) {
		int br = ped_building[i][0], bc = ped_building[i][1];
		if ((br >= CITY_BLOCKS) || (bc >= CITY_BLOCKS))
			continue;

		Building *b = &w->buildings[br * CITY_BLOCKS + bc];
		Pedestrian *p = &t->peds[t->ped_count++];

		p->home_x = b->minx - 60;
		p->home_z = b->minz - 60;
		p->x = p->home_x;
		p->z = p->home_z;
		p->dir = 0;
		p->radius = 50;
		p->state = PED_WALKING;
		p->timer = 0;
	}
}

void traffic_update(Traffic *t, World *w) {
	for (int i = 0; i < t->car_count; i++) {
		TrafficCar *car = &t->cars[i];
		int tx = car->wp_x[car->wp_index];
		int tz = car->wp_z[car->wp_index];
		int dx = tx - vehicle_world_x(&car->veh);
		int dz = tz - vehicle_world_z(&car->veh);

		int steer = ai_steer_toward(car->veh.heading, dx, dz);
		vehicle_update(&car->veh, w, 1, steer, 0);

		int distsq = dx * dx + dz * dz;
		if (distsq < (300 * 300))
			car->wp_index = (car->wp_index + 1) % car->wp_count;
	}

	for (int i = 0; i < t->ped_count; i++) {
		Pedestrian *p = &t->peds[i];

		if (p->state == PED_SCATTERED) {
			if (p->timer > 0) {
				p->timer--;
			} else {
				p->x = p->home_x;
				p->z = p->home_z;
				p->state = PED_WALKING;
			}
		}
	}
}

void traffic_draw(Traffic *t, RenderContext *ctx, MATRIX *cam_mtx, RECT *clip) {
	for (int i = 0; i < t->car_count; i++)
		vehicle_draw(&t->cars[i].veh, ctx, cam_mtx, clip);

	gte_SetRotMatrix(cam_mtx);
	gte_SetTransMatrix(cam_mtx);

	for (int i = 0; i < t->ped_count; i++) {
		Pedestrian *p = &t->peds[i];
		if (p->state == PED_SCATTERED)
			continue;

		int hw = 30;
		int top = -160, base = 0;
		uint8_t r = 220, g = 190, b = 150;

		render_quad_f4(ctx, clip,
			p->x - hw, top, p->z,       p->x + hw, top, p->z,
			p->x - hw, base, p->z,      p->x + hw, base, p->z,
			r, g, b);
		render_quad_f4(ctx, clip,
			p->x, top, p->z - hw,       p->x, top, p->z + hw,
			p->x, base, p->z - hw,      p->x, base, p->z + hw,
			r, g, b);
	}
}

int traffic_spook_pedestrian(Traffic *t, int x, int z, int radius) {
	for (int i = 0; i < t->ped_count; i++) {
		Pedestrian *p = &t->peds[i];
		if (p->state == PED_SCATTERED)
			continue;

		int dx = x - p->x, dz = z - p->z;
		int rr = radius + p->radius;
		if ((dx * dx + dz * dz) < (rr * rr)) {
			p->state = PED_SCATTERED;
			p->timer = PED_RESPAWN_TIME;
			return 1;
		}
	}
	return 0;
}
