#include <assert.h>
#include <inline_c.h>
#include "render.h"
#include "clip.h"

void render_init(RenderContext *ctx, int r, int g, int b) {
	ResetGraph(0);

	SetDefDrawEnv(&(ctx->buffers[0].draw_env), 0, 0, SCREEN_XRES, SCREEN_YRES);
	SetDefDispEnv(&(ctx->buffers[0].disp_env), 0, 0, SCREEN_XRES, SCREEN_YRES);
	SetDefDrawEnv(&(ctx->buffers[1].draw_env), 0, SCREEN_YRES, SCREEN_XRES, SCREEN_YRES);
	SetDefDispEnv(&(ctx->buffers[1].disp_env), 0, SCREEN_YRES, SCREEN_XRES, SCREEN_YRES);

	setRGB0(&(ctx->buffers[0].draw_env), r, g, b);
	setRGB0(&(ctx->buffers[1].draw_env), r, g, b);
	ctx->buffers[0].draw_env.isbg = 1;
	ctx->buffers[1].draw_env.isbg = 1;
	ctx->buffers[0].draw_env.dtd  = 1;
	ctx->buffers[1].draw_env.dtd  = 1;

	ctx->active_buffer = 0;
	ctx->next_packet    = ctx->buffers[0].buffer;
	ClearOTagR(ctx->buffers[0].ot, OT_LENGTH);
	ClearOTagR(ctx->buffers[1].ot, OT_LENGTH);

	setRECT(&ctx->clip, 0, 0, SCREEN_XRES, SCREEN_YRES);

	PutDrawEnv(&(ctx->buffers[0].draw_env));

	InitGeom();
	gte_SetGeomOffset(CENTERX, CENTERY);
	gte_SetGeomScreen(CENTERX);
	gte_SetBackColor(30, 30, 40);

	SetDispMask(1);
}

void *render_reserve(RenderContext *ctx, size_t size) {
	RenderBuffer *buf  = render_active(ctx);
	uint8_t       *prim = ctx->next_packet;

	ctx->next_packet += size;
	assert(ctx->next_packet <= &(buf->buffer[PACKET_LENGTH]));

	return (void *) prim;
}

void render_link(RenderContext *ctx, int z, void *prim) {
	RenderBuffer *buf = render_active(ctx);

	if (z < 0)
		z = 0;
	if (z >= OT_LENGTH)
		z = OT_LENGTH - 1;

	addPrim(&(buf->ot[z]), prim);
}

void render_unreserve(RenderContext *ctx, size_t size) {
	ctx->next_packet -= size;
}

void render_quad_f4(
	RenderContext *ctx, RECT *clip,
	int x0, int y0, int z0, int x1, int y1, int z1,
	int x2, int y2, int z2, int x3, int y3, int z3,
	uint8_t r, uint8_t g, uint8_t b
) {
	SVECTOR v0 = { x0, y0, z0, 0 };
	SVECTOR v1 = { x1, y1, z1, 0 };
	SVECTOR v2 = { x2, y2, z2, 0 };
	SVECTOR v3 = { x3, y3, z3, 0 };
	DVECTOR sxy[4];
	int p;

	/* Cheap reject using the 3-vertex depth estimate, before touching the
	 * packet buffer at all. */
	gte_ldv3(&v0, &v1, &v2);
	gte_rtpt();
	gte_avsz3();
	gte_stotz(&p);

	if ((p <= 0) || (p >= OT_LENGTH))
		return;

	gte_stsxy0(&sxy[0]);
	gte_stsxy1(&sxy[1]);
	gte_stsxy2(&sxy[2]);

	gte_ldv0(&v3);
	gte_rtps();
	gte_stsxy(&sxy[3]);

	if (quad_clip(clip, &sxy[0], &sxy[1], &sxy[2], &sxy[3]))
		return;

	/* Accurate 4-vertex depth: this is what actually decides draw order. */
	gte_avsz4();
	gte_stotz(&p);
	if ((p <= 0) || (p >= OT_LENGTH))
		return;

	POLY_F4 *pol = (POLY_F4 *) render_reserve(ctx, sizeof(POLY_F4));
	setPolyF4(pol);
	pol->x0 = sxy[0].vx; pol->y0 = sxy[0].vy;
	pol->x1 = sxy[1].vx; pol->y1 = sxy[1].vy;
	pol->x2 = sxy[2].vx; pol->y2 = sxy[2].vy;
	pol->x3 = sxy[3].vx; pol->y3 = sxy[3].vy;
	setRGB0(pol, r, g, b);

	render_link(ctx, p, pol);
}

void render_quad_ft4(
	RenderContext *ctx, RECT *clip,
	int x0, int y0, int z0, int x1, int y1, int z1,
	int x2, int y2, int z2, int x3, int y3, int z3,
	Texture *tex, uint8_t r, uint8_t g, uint8_t b
) {
	SVECTOR v0 = { x0, y0, z0, 0 };
	SVECTOR v1 = { x1, y1, z1, 0 };
	SVECTOR v2 = { x2, y2, z2, 0 };
	SVECTOR v3 = { x3, y3, z3, 0 };
	DVECTOR sxy[4];
	int p;

	gte_ldv3(&v0, &v1, &v2);
	gte_rtpt();
	gte_avsz3();
	gte_stotz(&p);

	if ((p <= 0) || (p >= OT_LENGTH))
		return;

	gte_stsxy0(&sxy[0]);
	gte_stsxy1(&sxy[1]);
	gte_stsxy2(&sxy[2]);

	gte_ldv0(&v3);
	gte_rtps();
	gte_stsxy(&sxy[3]);

	if (quad_clip(clip, &sxy[0], &sxy[1], &sxy[2], &sxy[3]))
		return;

	gte_avsz4();
	gte_stotz(&p);
	if ((p <= 0) || (p >= OT_LENGTH))
		return;

	POLY_FT4 *pol = (POLY_FT4 *) render_reserve(ctx, sizeof(POLY_FT4));
	setPolyFT4(pol);
	pol->x0 = sxy[0].vx; pol->y0 = sxy[0].vy;
	pol->x1 = sxy[1].vx; pol->y1 = sxy[1].vy;
	pol->x2 = sxy[2].vx; pol->y2 = sxy[2].vy;
	pol->x3 = sxy[3].vx; pol->y3 = sxy[3].vy;
	setRGB0(pol, r, g, b);
	setUVWH(pol, 0, 0, 63, 63);
	pol->tpage = tex->tpage;
	pol->clut = tex->clut;

	render_link(ctx, p, pol);
}

void render_flip(RenderContext *ctx) {
	DrawSync(0);
	VSync(0);

	RenderBuffer *draw_buffer = &(ctx->buffers[ctx->active_buffer]);
	RenderBuffer *disp_buffer = &(ctx->buffers[ctx->active_buffer ^ 1]);

	PutDispEnv(&(disp_buffer->disp_env));
	DrawOTagEnv(&(draw_buffer->ot[OT_LENGTH - 1]), &(draw_buffer->draw_env));

	ctx->active_buffer ^= 1;
	ctx->next_packet    = disp_buffer->buffer;
	ClearOTagR(disp_buffer->ot, OT_LENGTH);
}
