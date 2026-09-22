#pragma once

#include <psxgte.h>
#include "render.h"

/* The city is laid out as a grid of streets and building lots, Manhattan
 * style: CITY_BLOCKS x CITY_BLOCKS lots, each surrounded by streets.
 * The tile grid therefore has 2*CITY_BLOCKS+1 rows/columns, alternating
 * street (even index) and lot (odd index).
 */
#define CITY_BLOCKS   5
#define TILE_GRID     (2 * CITY_BLOCKS + 1)
#define ROAD_W        700
#define LOT_W         1600
#define BUILD_MIN_H   500
#define BUILD_MAX_H   3000

#define MAX_BUILDINGS (CITY_BLOCKS * CITY_BLOCKS)

typedef struct {
	int     minx, minz, maxx, maxz; /* World-space footprint (raw units). */
	int     height;                 /* Extrusion height, roof at y = -height. */
	int     tex_id;                 /* TEX_WALL_* variant for all 4 walls.    */
	uint8_t tint;                   /* Per-building brightness, ~112-144.     */
} Building;

typedef struct {
	int      tile_offset[TILE_GRID + 1];
	int      city_size;
	Building buildings[MAX_BUILDINGS];
	int      building_count;
} World;

void world_generate(World *w);
void world_draw(World *w, RenderContext *ctx, MATRIX *cam_mtx, RECT *clip);

/* Tests a circle (x,z,radius) against all building footprints. Returns
 * non-zero on overlap and accumulates a separation vector into (dx,dz)
 * that would push the circle back outside the nearest building(s). */
int world_collide_circle(World *w, int x, int z, int radius, int *dx, int *dz);

/* Clamps (x,z) to stay within the city's outer boundary. */
void world_clamp_bounds(World *w, int *x, int *z, int margin);

/* Returns the world-space center of tile (row,col). */
void world_tile_center(World *w, int row, int col, int *x, int *z);
