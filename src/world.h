#pragma once

#include <psxgte.h>
#include "render.h"

/* The city is laid out as a grid of streets and building lots, Manhattan
 * style: CITY_BLOCKS x CITY_BLOCKS lots, each surrounded by streets. The
 * tile grid therefore has 2*CITY_BLOCKS+1 rows/columns, alternating street
 * (even index) and lot (odd index).
 *
 * Block/street widths are randomized per-tile (not a uniform repeat)
 * instead of one fixed ROAD_W/LOT_W for every tile - loosely modeled on
 * Piekary Śląskie, a Upper Silesian town that grew by several separate
 * village cores (Szarlej, Wielkie Piekary, ...) merging together rather
 * than from one planned grid, so it reads as more organic than a sterile
 * repeating grid. MAIN_ROAD_INDEX gets one wider arterial street crossing
 * both axes near the middle, and PLAZA_ROW/PLAZA_COL - the lot at their
 * intersection - is left unbuilt as a paved central square instead of a
 * building.
 */
#define CITY_BLOCKS     5
#define TILE_GRID       (2 * CITY_BLOCKS + 1)
#define ROAD_W_MIN      600
#define ROAD_W_MAX      900
#define MAIN_ROAD_INDEX 4
#define MAIN_ROAD_W     1400
#define LOT_W_MIN       1200
#define LOT_W_MAX       2000
#define BUILD_MIN_H     500
#define BUILD_MAX_H     3000
#define PLAZA_ROW       5
#define PLAZA_COL       5

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
