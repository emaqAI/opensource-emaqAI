#include <inline_c.h>
#include "vehicle.h"
#include "fixed.h"

void vehicle_init(Vehicle *v, int x, int z, int heading, uint8_t r, uint8_t g, uint8_t b) {
	v->pos.vx = x << 12;
	v->pos.vy = 0;
	v->pos.vz = z << 12;
	v->heading = angle_wrap(heading);
	v->speed = 0;
	v->radius = VEH_RADIUS;
	v->crash_timer = 0;
	v->r = r;
	v->g = g;
	v->b = b;
}

int vehicle_update(Vehicle *v, World *w, int accel, int steer, int handbrake) {
	int turn_rate = handbrake ? (VEH_TURN_RATE + VEH_TURN_RATE / 2) : VEH_TURN_RATE;

	if (steer && (v->speed != 0))
		v->heading = angle_wrap(v->heading + steer * turn_rate);

	if (accel > 0) {
		v->speed += VEH_ACCEL;
	} else if (accel < 0) {
		if (v->speed > 0)
			v->speed -= VEH_BRAKE;
		else
			v->speed -= VEH_ACCEL;
	} else {
		if (v->speed > 0)
			v->speed = (v->speed > VEH_FRICTION) ? v->speed - VEH_FRICTION : 0;
		else if (v->speed < 0)
			v->speed = (v->speed < -VEH_FRICTION) ? v->speed + VEH_FRICTION : 0;
	}

	if (handbrake && (v->speed > 0))
		v->speed = (v->speed > VEH_BRAKE) ? v->speed - VEH_BRAKE : 0;

	v->speed = iclamp(v->speed, -VEH_MAX_REVERSE, VEH_MAX_SPEED);

	int fwd_x = isin(v->heading);
	int fwd_z = icos(v->heading);

	v->pos.vx += fx_mul(fwd_x, v->speed);
	v->pos.vz += fx_mul(fwd_z, v->speed);

	int wx = vehicle_world_x(v);
	int wz = vehicle_world_z(v);
	world_clamp_bounds(w, &wx, &wz, v->radius);

	int dx, dz;
	int hit = world_collide_circle(w, wx, wz, v->radius, &dx, &dz);

	if (hit) {
		wx += dx;
		wz += dz;
		v->speed = v->speed / 4;
		v->crash_timer = 12;
	}

	v->pos.vx = wx << 12;
	v->pos.vz = wz << 12;

	if (v->crash_timer > 0)
		v->crash_timer--;

	return hit;
}

void vehicle_draw(Vehicle *v, RenderContext *ctx, MATRIX *cam_mtx, RECT *clip) {
	MATRIX omtx;
	SVECTOR rot = { 0, (short) v->heading, 0, 0 };
	VECTOR  wpos = { vehicle_world_x(v), 0, vehicle_world_z(v) };

	RotMatrix(&rot, &omtx);
	TransMatrix(&omtx, &wpos);
	CompMatrixLV(cam_mtx, &omtx, &omtx);

	gte_SetRotMatrix(&omtx);
	gte_SetTransMatrix(&omtx);

	int hw = 90, hl = 190, roof = -130, body = -30;
	uint8_t r = v->r, g = v->g, b = v->b;

	/* Body: front (nose, +Z), back (tail, -Z), left, right, roof. */
	render_quad_f4(ctx, clip,
		-hw, roof,  hl,   hw, roof,  hl,
		-hw, body,  hl,   hw, body,  hl,
		r, g, b);

	render_quad_f4(ctx, clip,
		 hw, roof, -hl,  -hw, roof, -hl,
		 hw, body, -hl,  -hw, body, -hl,
		(uint8_t)(r / 2), 20, 20); /* dim tail end, red-ish taillights */

	render_quad_f4(ctx, clip,
		-hw, roof, -hl,  -hw, roof,  hl,
		-hw, body, -hl,  -hw, body,  hl,
		r, g, b);

	render_quad_f4(ctx, clip,
		 hw, roof,  hl,   hw, roof, -hl,
		 hw, body,  hl,   hw, body, -hl,
		r, g, b);

	render_quad_f4(ctx, clip,
		-hw, roof, -hl,   hw, roof, -hl,
		-hw, roof,  hl,   hw, roof,  hl,
		(uint8_t)(r * 2 / 3), (uint8_t)(g * 2 / 3), (uint8_t)(b * 2 / 3));

	/* Lower skirt down to the ground so the car doesn't look like it's
	 * floating when the camera looks down at it. */
	render_quad_f4(ctx, clip,
		-hw, body,  hl,   hw, body,  hl,
		-hw,    0,  hl,   hw,    0,  hl,
		20, 20, 20);
	render_quad_f4(ctx, clip,
		 hw, body, -hl,  -hw, body, -hl,
		 hw,    0, -hl,  -hw,    0, -hl,
		20, 20, 20);
	render_quad_f4(ctx, clip,
		-hw, body, -hl,  -hw, body,  hl,
		-hw,    0, -hl,  -hw,    0,  hl,
		20, 20, 20);
	render_quad_f4(ctx, clip,
		 hw, body,  hl,   hw, body, -hl,
		 hw,    0,  hl,   hw,    0, -hl,
		20, 20, 20);
}
