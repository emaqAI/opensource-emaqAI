#pragma once

#include <stddef.h>
#include <stdint.h>
#include <psxgpu.h>
#include <psxgte.h>

#define SCREEN_XRES 320
#define SCREEN_YRES 240
#define CENTERX     (SCREEN_XRES >> 1)
#define CENTERY     (SCREEN_YRES >> 1)

#define OT_LENGTH     4096
#define PACKET_LENGTH 32768

typedef struct {
	DISPENV  disp_env;
	DRAWENV  draw_env;
	uint32_t ot[OT_LENGTH];
	uint8_t  buffer[PACKET_LENGTH];
} RenderBuffer;

typedef struct {
	RenderBuffer buffers[2];
	uint8_t      *next_packet;
	int          active_buffer;
	RECT         clip;
} RenderContext;

void  render_init(RenderContext *ctx, int r, int g, int b);

/* Reserves 'size' bytes for a new primitive in the active buffer without
 * linking it into the ordering table yet. The caller must follow up with
 * render_link() (once the primitive's final depth is known) or roll the
 * allocation back manually if it ends up being discarded. */
void  *render_reserve(RenderContext *ctx, size_t size);

/* Links a previously reserved primitive into the ordering table at depth z. */
void  render_link(RenderContext *ctx, int z, void *prim);

/* Rolls back the last reservation of 'size' bytes (must be the most recent
 * one, and must not have been linked yet). Used to discard primitives that
 * turn out to be offscreen/behind the camera. */
void  render_unreserve(RenderContext *ctx, size_t size);

void  render_flip(RenderContext *ctx);

/* Transforms, clips and draws a flat-shaded quad given in world space, with
 * vertices in (top-left, top-right, bottom-left, bottom-right) order, as
 * expected by POLY_F4. The GTE rotation/translation matrix must already be
 * set by the caller. Silently no-ops if the quad ends up offscreen, behind
 * the camera, or outside the ordering table's depth range. */
void render_quad_f4(
	RenderContext *ctx, RECT *clip,
	int x0, int y0, int z0, int x1, int y1, int z1,
	int x2, int y2, int z2, int x3, int y3, int z3,
	uint8_t r, uint8_t g, uint8_t b
);

static inline RenderBuffer *render_active(RenderContext *ctx) {
	return &(ctx->buffers[ctx->active_buffer]);
}
