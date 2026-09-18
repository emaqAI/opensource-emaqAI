/* Fixed-point / angle conventions shared by all game code.
 *
 * World positions are stored as VECTOR with each component scaled by
 * ONE (4096), matching the convention used by PSn00bSDK's GTE helpers
 * (isin()/icos() return 4096 = 1.0). This gives sub-unit precision for
 * smooth acceleration/steering while GTE transforms always receive the
 * plain (>>12) world-unit value.
 *
 * Headings/angles use PSn00bSDK's RotMatrix() convention where a full
 * turn is ANGLE_MAX (4096) units, i.e. the same domain as SVECTOR
 * rotation fields and isin()/icos() arguments as used throughout the
 * official examples (see examples/graphics/fpscam).
 */

#pragma once

#include <psxgte.h>

#define ANGLE_MAX   4096
#define ANGLE_90    (ANGLE_MAX / 4)
#define ANGLE_180   (ANGLE_MAX / 2)

static inline int angle_wrap(int a) {
	a &= (ANGLE_MAX - 1);
	return a;
}

/* Shortest signed distance from a to b, in (-ANGLE_MAX/2, ANGLE_MAX/2]. */
static inline int angle_delta(int a, int b) {
	int d = angle_wrap(b - a);
	if (d > ANGLE_180)
		d -= ANGLE_MAX;
	return d;
}

static inline int fx_mul(int a, int b) {
	return (a * b) >> 12;
}

static inline int iabs(int v) {
	return (v < 0) ? -v : v;
}

static inline int iclamp(int v, int lo, int hi) {
	if (v < lo) return lo;
	if (v > hi) return hi;
	return v;
}

/* Returns -1/0/1: which way a car facing 'heading' should steer to turn
 * toward a point offset by (dx,dz), without needing an atan2(). Works by
 * checking which side of the car's forward vector the target lies on
 * (sign of the cross product between "forward" and "to-target"). */
static inline int ai_steer_toward(int heading, int dx, int dz) {
	int fwd_x = isin(heading);
	int fwd_z = icos(heading);
	int turn_signal = dx * fwd_z - dz * fwd_x;

	if (turn_signal > 0)
		return 1;
	if (turn_signal < 0)
		return -1;
	return 0;
}
