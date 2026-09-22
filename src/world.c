#include <stdlib.h>
#include <inline_c.h>
#include "world.h"
#include "fixed.h"

#define quad(ctx, clip, z_hint, x0,y0,z0, x1,y1,z1, x2,y2,z2, x3,y3,z3, r,g,b) \
	render_quad_f4((ctx), (clip), (x0),(y0),(z0), (x1),(y1),(z1), (x2),(y2),(z2), (x3),(y3),(z3), (r),(g),(b))

#define tquad(ctx, clip, x0,y0,z0, x1,y1,z1, x2,y2,z2, x3,y3,z3, tex, r,g,b) \
	render_quad_ft4((ctx), (clip), (x0),(y0),(z0), (x1),(y1),(z1), (x2),(y2),(z2), (x3),(y3),(z3), (tex), (r),(g),(b))

#define WALL_TEX_COUNT 3
static const TextureId wall_textures[WALL_TEX_COUNT] = { TEX_WALL_CONCRETE, TEX_WALL_BRICK, TEX_WALL_GLASS };

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
			b->tex_id = rand() % WALL_TEX_COUNT;
			b->tint = (uint8_t) (112 + (rand() % 32));
		}
	}
}

void world_tile_center(World *w, int row, int col, int *x, int *z) {
	*x = (w->tile_offset[col] + w->tile_offset[col + 1]) >> 1;
	*z = (w->tile_offset[row] + w->tile_offset[row + 1]) >> 1;
}

static void shade_tint(uint8_t base, int pct, uint8_t *out) {
	*out = (uint8_t) iclamp((base * pct) / 100, 0, 255);
}

void world_draw(World *w, RenderContext *ctx, MATRIX *cam_mtx, RECT *clip) {
	gte_SetRotMatrix(cam_mtx);
	gte_SetTransMatrix(cam_mtx);

	Texture *road_tex = texture_get(TEX_ROAD);
	Texture *roof_tex = texture_get(TEX_ROOF);

	/* Road tiles + lane markings. */
	for (int row = 0; row < TILE_GRID; row++) {
		for (int col = 0; col < TILE_GRID; col++) {
			int is_lot = (row & 1) && (col & 1);
			if (is_lot)
				continue;

			int x0 = w->tile_offset[col],     x1 = w->tile_offset[col + 1];
			int z0 = w->tile_offset[row],     z1 = w->tile_offset[row + 1];

			tquad(ctx, clip,
				x0, 0, z0,  x1, 0, z0,
				x0, 0, z1,  x1, 0, z1,
				road_tex, 128, 128, 128);

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
	Texture *sidewalk_tex = texture_get(TEX_SIDEWALK);

	for (int i = 0; i < w->building_count; i++) {
		Building *b = &w->buildings[i];
		int h = -b->height;
		Texture *wall_tex = texture_get(wall_textures[b->tex_id]);
		uint8_t r;

		/* Sidewalk border, sitting just above the road plane so it never
		 * z-fights with the road tiles underneath it. */
		int sw = 44;
		tquad(ctx, clip,
			b->minx - sw, -2, b->minz - sw,  b->maxx + sw, -2, b->minz - sw,
			b->minx - sw, -2, b->minz,       b->maxx + sw, -2, b->minz,
			sidewalk_tex, 128, 128, 128);
		tquad(ctx, clip,
			b->maxx + sw, -2, b->maxz + sw,  b->minx - sw, -2, b->maxz + sw,
			b->maxx + sw, -2, b->maxz,       b->minx - sw, -2, b->maxz,
			sidewalk_tex, 128, 128, 128);
		tquad(ctx, clip,
			b->minx - sw, -2, b->maxz,  b->minx - sw, -2, b->minz,
			b->minx,      -2, b->maxz,  b->minx,      -2, b->minz,
			sidewalk_tex, 128, 128, 128);
		tquad(ctx, clip,
			b->maxx,      -2, b->minz,  b->maxx,      -2, b->maxz,
			b->maxx + sw, -2, b->minz,  b->maxx + sw, -2, b->maxz,
			sidewalk_tex, 128, 128, 128);

		/* North wall (z = minz). */
		shade_tint(b->tint, 100, &r);
		tquad(ctx, clip,
			b->minx, h, b->minz,  b->maxx, h, b->minz,
			b->minx, 0, b->minz,  b->maxx, 0, b->minz,
			wall_tex, r, r, r);

		/* South wall (z = maxz). */
		shade_tint(b->tint, 90, &r);
		tquad(ctx, clip,
			b->maxx, h, b->maxz,  b->minx, h, b->maxz,
			b->maxx, 0, b->maxz,  b->minx, 0, b->maxz,
			wall_tex, r, r, r);

		/* West wall (x = minx). */
		shade_tint(b->tint, 78, &r);
		tquad(ctx, clip,
			b->minx, h, b->maxz,  b->minx, h, b->minz,
			b->minx, 0, b->maxz,  b->minx, 0, b->minz,
			wall_tex, r, r, r);

		/* East wall (x = maxx). */
		shade_tint(b->tint, 68, &r);
		tquad(ctx, clip,
			b->maxx, h, b->minz,  b->maxx, h, b->maxz,
			b->maxx, 0, b->minz,  b->maxx, 0, b->maxz,
			wall_tex, r, r, r);

		/* Roof. */
		shade_tint(b->tint, 100, &r);
		tquad(ctx, clip,
			b->minx, h, b->minz,  b->maxx, h, b->minz,
			b->minx, h, b->maxz,  b->maxx, h, b->maxz,
			roof_tex, r, r, r);
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
