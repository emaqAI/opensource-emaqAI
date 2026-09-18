#include <psxgpu.h>
#include <psxgte.h>
#include <inline_c.h>

#include "render.h"
#include "world.h"
#include "vehicle.h"
#include "camera.h"
#include "traffic.h"
#include "police.h"
#include "mission.h"
#include "hud.h"
#include "input.h"
#include "fixed.h"

static RenderContext ctx;
static World         world;
static Vehicle       player;
static Camera        camera;
static Traffic       traffic;
static PoliceState   police;
static Mission       mission;
static InputState    input;

static void read_player_controls(int *accel, int *steer, int *handbrake) {
	*accel = 0;
	*steer = 0;
	*handbrake = (input.held & PAD_SQUARE) ? 1 : 0;

	if (input.held & PAD_UP)
		*accel = 1;
	else if (input.held & PAD_DOWN)
		*accel = -1;

	if (input.held & PAD_LEFT)
		*steer = -1;
	else if (input.held & PAD_RIGHT)
		*steer = 1;

	if (input.analog) {
		if (input.ly < -40)
			*accel = 1;
		else if (input.ly > 40)
			*accel = -1;

		if (input.lx < -40)
			*steer = -1;
		else if (input.lx > 40)
			*steer = 1;
	}
}

/* Simple circle-circle push-apart used for player-vs-traffic/police bumps. */
static int bump(Vehicle *a, Vehicle *b) {
	int dx = vehicle_world_x(a) - vehicle_world_x(b);
	int dz = vehicle_world_z(a) - vehicle_world_z(b);
	int rr = a->radius + b->radius;
	int distsq = dx * dx + dz * dz;

	if (distsq >= (rr * rr))
		return 0;

	int dist = SquareRoot0(distsq);
	if (dist < 1)
		dist = 1;
	int push = rr - dist;

	int px = (dx * push) / dist;
	int pz = (dz * push) / dist;

	a->pos.vx += px << 12;
	a->pos.vz += pz << 12;
	a->speed  /= 3;
	a->crash_timer = 12;

	return 1;
}

static void update_gameplay(void) {
	int accel, steer, handbrake;
	read_player_controls(&accel, &steer, &handbrake);

	vehicle_update(&player, &world, accel, steer, handbrake);
	traffic_update(&traffic, &world);
	police_update(&police, &world, &player);

	for (int i = 0; i < traffic.car_count; i++) {
		if (bump(&player, &traffic.cars[i].veh))
			police_add_heat(&police, &world, 1);
	}
	for (int i = 0; i < MAX_POLICE; i++) {
		if (police.cars[i].active)
			bump(&player, &police.cars[i].veh);
	}
	if (traffic_spook_pedestrian(&traffic, vehicle_world_x(&player), vehicle_world_z(&player), player.radius + 40))
		police_add_heat(&police, &world, 1);

	mission_update(&mission, &input, &player, &world, &police);
}

int main(int argc, const char **argv) {
	(void) argc;
	(void) argv;

	render_init(&ctx, 20, 24, 30);
	FntLoad(960, 0);
	FntOpen(8, 16, 304, 216, 0, 512);

	input_init();

	world_generate(&world);

	int spawn_x, spawn_z;
	world_tile_center(&world, 4, 4, &spawn_x, &spawn_z);
	vehicle_init(&player, spawn_x, spawn_z, 0, 220, 50, 50);
	player.radius = VEH_RADIUS;

	camera_init(&camera, &player);
	traffic_init(&traffic, &world);
	police_init(&police, &world);
	mission_init(&mission, &world);

	while (1) {
		input_update(&input);

		if (mission_is_gameplay(&mission))
			update_gameplay();
		else
			mission_update(&mission, &input, &player, &world, &police);

		MATRIX cam_mtx;
		camera_update(&camera, &player, &cam_mtx);

		world_draw(&world, &ctx, &cam_mtx, &ctx.clip);
		traffic_draw(&traffic, &ctx, &cam_mtx, &ctx.clip);
		police_draw(&police, &ctx, &cam_mtx, &ctx.clip);
		vehicle_draw(&player, &ctx, &cam_mtx, &ctx.clip);

		if (mission_is_gameplay(&mission))
			hud_draw(&player, &police, &mission);
		else
			mission_draw_overlay(&mission);

		FntFlush(-1);
		render_flip(&ctx);
	}

	return 0;
}
