/* Screen-space offscreen test for triangles/quads.
 *
 * Adapted from PSn00bSDK's examples/graphics/fpscam/clip.{h,c}
 * (C) 2020-2023 Lameguy64, spicyjpeg - MPL-2.0 licensed.
 *
 * Based on the off-screen detection half of the Cohen-Sutherland
 * clipping algorithm: each vertex is classified against the four screen
 * edges and a primitive is considered offscreen only if all vertices
 * share an outside region.
 */

#include "clip.h"

#define CLIP_LEFT   1
#define CLIP_RIGHT  2
#define CLIP_TOP    4
#define CLIP_BOTTOM 8

static int test_clip(RECT *clip, short x, short y) {
	int result = 0;

	if (x < clip->x)
		result |= CLIP_LEFT;
	if (x >= (clip->x + (clip->w - 1)))
		result |= CLIP_RIGHT;
	if (y < clip->y)
		result |= CLIP_TOP;
	if (y >= (clip->y + (clip->h - 1)))
		result |= CLIP_BOTTOM;

	return result;
}

int tri_clip(RECT *clip, DVECTOR *v0, DVECTOR *v1, DVECTOR *v2) {
	short c[3];

	c[0] = test_clip(clip, v0->vx, v0->vy);
	c[1] = test_clip(clip, v1->vx, v1->vy);
	c[2] = test_clip(clip, v2->vx, v2->vy);

	if ((c[0] & c[1]) == 0) return 0;
	if ((c[1] & c[2]) == 0) return 0;
	if ((c[2] & c[0]) == 0) return 0;

	return 1;
}

int quad_clip(RECT *clip, DVECTOR *v0, DVECTOR *v1, DVECTOR *v2, DVECTOR *v3) {
	short c[4];

	c[0] = test_clip(clip, v0->vx, v0->vy);
	c[1] = test_clip(clip, v1->vx, v1->vy);
	c[2] = test_clip(clip, v2->vx, v2->vy);
	c[3] = test_clip(clip, v3->vx, v3->vy);

	if ((c[0] & c[1]) == 0) return 0;
	if ((c[1] & c[2]) == 0) return 0;
	if ((c[2] & c[3]) == 0) return 0;
	if ((c[3] & c[0]) == 0) return 0;
	if ((c[0] & c[2]) == 0) return 0;
	if ((c[1] & c[3]) == 0) return 0;

	return 1;
}
