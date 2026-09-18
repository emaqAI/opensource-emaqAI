/* Screen-space offscreen test for triangles/quads.
 *
 * Adapted from PSn00bSDK's examples/graphics/fpscam/clip.{h,c}
 * (C) 2020-2023 Lameguy64, spicyjpeg - MPL-2.0 licensed.
 */

#ifndef _CLIP_H
#define _CLIP_H

#include <sys/types.h>
#include <psxgte.h>
#include <psxgpu.h>

/* Returns non-zero if triangle (v0,v1,v2) is entirely outside 'clip'. */
int tri_clip(RECT *clip, DVECTOR *v0, DVECTOR *v1, DVECTOR *v2);

/* Returns non-zero if quad (v0,v1,v2,v3) is entirely outside 'clip'. */
int quad_clip(RECT *clip, DVECTOR *v0, DVECTOR *v1, DVECTOR *v2, DVECTOR *v3);

#endif // _CLIP_H
