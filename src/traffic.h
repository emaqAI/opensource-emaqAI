#pragma once

#include "render.h"
#include "world.h"
#include "vehicle.h"

#define MAX_TRAFFIC_CARS  4
#define MAX_WAYPOINTS     4
#define MAX_PEDESTRIANS   6

typedef struct {
	Vehicle veh;
	int     wp_x[MAX_WAYPOINTS];
	int     wp_z[MAX_WAYPOINTS];
	int     wp_count;
	int     wp_index;
} TrafficCar;

typedef enum {
	PED_WALKING = 0,
	PED_SCATTERED
} PedState;

typedef struct {
	int      x, z;
	int      home_x, home_z;
	int      dir;       /* Facing angle, cosmetic only.            */
	int      radius;
	PedState state;
	int      timer;     /* Frames remaining in current state.      */
} Pedestrian;

typedef struct {
	TrafficCar cars[MAX_TRAFFIC_CARS];
	int        car_count;
	Pedestrian peds[MAX_PEDESTRIANS];
	int        ped_count;
} Traffic;

void traffic_init(Traffic *t, World *w);
void traffic_update(Traffic *t, World *w);
void traffic_draw(Traffic *t, RenderContext *ctx, MATRIX *cam_mtx, RECT *clip);

/* Marks the pedestrian nearest (x,z) within 'radius' as scattered (a near
 * miss / close call), if any. Returns non-zero if one was found. */
int traffic_spook_pedestrian(Traffic *t, int x, int z, int radius);
