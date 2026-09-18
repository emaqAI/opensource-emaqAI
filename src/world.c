#include <stdlib.h>
#include <inline_c.h>
#include "world.h"
#include "fixed.h"

#define quad(ctx, clip, z_hint, x0,y0,z0, x1,y1,z1, x2,y2,z2, x3,y3,z3, r,g,b) \
	render_quad_f4((ctx), (clip), (x0),(y0),(z0), (x1),(y1),(z1), (x2),(y2),(z2), (x3),(y3),(z3), (r),(g),(b))

static void shade(int r, int g, int b, int pct, uint8_t *outr, uint8_t *outg, uint8_t *outb) {
	*outr = (uint8_t) iclamp((r * pct) / 100, 0, 255);
	*outg = (uint8_t) iclamp((g * pct) / 100, 0, 255);
	*outb = (uint8_t) iclamp((b * pct) / 100, 0, 255);
}

void world_generate(World *w) {
	int i;

	w->tile_offset[0] = 0;
	for (i = 0; i < TILE_GRID; i++) {
		int size = (i & 1) ? LOT_W : ROAD_W;
		w->tile_offset[i + 1] = w->tile_offset[i] + size;
	}
	w->city_size = w->tile_offset[TILE_GRID];

	srand(1337);

	w->building_count = 0;
	for (int row = 1; row < TILE_GRID; row += 2) {
		for (int col = 1; col < TILE_GRID; col += 2) {
			Building *b = &w->buildings[w->building_count++];

			b->minx = w->tile_offset[col];
			b->maxx = w->tile_offset[col + 1];
			b->minz = w->tile_offset[row];
			b->maxz = w->tile_offset[row + 1];
			b->height = BUILD_MIN_H + (rand() % (BUILD_MAX_H - BUILD_MIN_H));

			int tint = rand() % 4;
			switch (tint) {
				case 0: b->r = 150; b->g = 150; b->b = 165; break; /* concrete */
				case 1: b->r = 120; b->g = 140; b->b = 160; break; /* steel blue */
				case 2: b->r = 160; b->g = 120; b->b = 100; break; /* brick */
				default: b->r = 130; b->g = 130; b->b = 130; break; /* grey */
			}
		}
	}
}

void world_tile_center(World *w, int row, int col, int *x, int *z) {
	*x = (w->tile_offset[col] + w->tile_offset[col + 1]) >> 1;
	*z = (w->tile_offset[row] + w->tile_offset[row + 1]) >> 1;
}

void world_draw(World *w, RenderContext *ctx, MATRIX *cam_mtx, RECT *clip) {
	gte_SetRotMatrix(cam_mtx);
	gte_SetTransMatrix(cam_mtx);

	/* Road tiles + lane markings. */
	for (int row = 0; row < TILE_GRID; row++) {
		for (int col = 0; col < TILE_GRID; col++) {
			int is_lot = (row & 1) && (col & 1);
			if (is_lot)
				continue;

			int x0 = w->tile_offset[col],     x1 = w->tile_offset[col + 1];
			int z0 = w->tile_offset[row],     z1 = w->tile_offset[row + 1];

			quad(ctx, clip, 0,
				x0, 0, z0,  x1, 0, z0,
				x0, 0, z1,  x1, 0, z1,
				58, 58, 62);

			/* Lane stripe down the middle of straight street segments. */
			int vertical_street   = !(row & 1) && (col & 1);
			int horizontal_street = (row & 1) && !(col & 1);
			int stripe_half = 20;

			if (vertical_street) {
				int cx = (x0 + x1) >> 1;
				quad(ctx, clip, 0,
					cx - stripe_half, -1, z0,  cx + stripe_half, -1, z0,
					cx - stripe_half, -1, z1,  cx + stripe_half, -1, z1,
					210, 190, 60);
			} else if (horizontal_street) {
				int cz = (z0 + z1) >> 1;
				quad(ctx, clip, 0,
					x0, -1, cz - stripe_half,  x1, -1, cz - stripe_half,
					x0, -1, cz + stripe_half,  x1, -1, cz + stripe_half,
					210, 190, 60);
			}
		}
	}

	/* Buildings. */
	for (int i = 0; i < w->building_count; i++) {
		Building *b = &w->buildings[i];
		int h = -b->height;
		uint8_t r, g, bl;

		/* North wall (z = minz). */
		shade(b->r, b->g, b->b, 100, &r, &g, &bl);
		quad(ctx, clip, 0,
			b->minx, h, b->minz,  b->maxx, h, b->minz,
			b->minx, 0, b->minz,  b->maxx, 0, b->minz,
			r, g, bl);

		/* South wall (z = maxz). */
		shade(b->r, b->g, b->b, 90, &r, &g, &bl);
		quad(ctx, clip, 0,
			b->maxx, h, b->maxz,  b->minx, h, b->maxz,
			b->maxx, 0, b->maxz,  b->minx, 0, b->maxz,
			r, g, bl);

		/* West wall (x = minx). */
		shade(b->r, b->g, b->b, 75, &r, &g, &bl);
		quad(ctx, clip, 0,
			b->minx, h, b->maxz,  b->minx, h, b->minz,
			b->minx, 0, b->maxz,  b->minx, 0, b->minz,
			r, g, bl);

		/* East wall (x = maxx). */
		shade(b->r, b->g, b->b, 65, &r, &g, &bl);
		quad(ctx, clip, 0,
			b->maxx, h, b->minz,  b->maxx, h, b->maxz,
			b->maxx, 0, b->minz,  b->maxx, 0, b->maxz,
			r, g, bl);

		/* Roof. */
		shade(b->r, b->g, b->b, 55, &r, &g, &bl);
		quad(ctx, clip, 0,
			b->minx, h, b->minz,  b->maxx, h, b->minz,
			b->minx, h, b->maxz,  b->maxx, h, b->maxz,
			r, g, bl);
	}
}

int world_collide_circle(World *w, int x, int z, int radius, int *dx, int *dz) {
	int hit = 0;
	*dx = 0;
	*dz = 0;

	for (int i = 0; i < w->building_count; i++) {
		Building *b = &w->buildings[i];

		int cx = iclamp(x, b->minx, b->maxx);
		int cz = iclamp(z, b->minz, b->maxz);
		int ddx = x - cx;
		int ddz = z - cz;
		int distsq = ddx * ddx + ddz * ddz;

		if (distsq >= radius * radius)
			continue;

		hit = 1;

		if (ddx == 0 && ddz == 0) {
			/* Center is inside the box: push out along the shallow axis. */
			int pen_l = x - b->minx, pen_r = b->maxx - x;
			int pen_t = z - b->minz, pen_bo = b->maxz - z;
			int minx_pen = (pen_l < pen_r) ? pen_l : pen_r;
			int minz_pen = (pen_t < pen_bo) ? pen_t : pen_bo;

			if (minx_pen < minz_pen)
				*dx += (pen_l < pen_r) ? -(pen_l + radius) : (pen_r + radius);
			else
				*dz += (pen_t < pen_bo) ? -(pen_t + radius) : (pen_bo + radius);
		} else {
			int dist = SquareRoot0(distsq);
			if (dist < 1)
				dist = 1;
			int push = radius - dist;
			*dx += (ddx * push) / dist;
			*dz += (ddz * push) / dist;
		}
	}

	return hit;
}

void world_clamp_bounds(World *w, int *x, int *z, int margin) {
	if (*x < margin) *x = margin;
	if (*z < margin) *z = margin;
	if (*x > w->city_size - margin) *x = w->city_size - margin;
	if (*z > w->city_size - margin) *z = w->city_size - margin;
}
