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
	v->tex = NULL;
	v->shape = VSHAPE_SEDAN;
	v->lightbar = LIGHTBAR_NONE;
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

static void panel(
	RenderContext *ctx, RECT *clip, Texture *tex,
	int x0, int y0, int z0, int x1, int y1, int z1,
	int x2, int y2, int z2, int x3, int y3, int z3,
	uint8_t r, uint8_t g, uint8_t b
) {
	if (tex)
		render_quad_ft4(ctx, clip, x0, y0, z0, x1, y1, z1, x2, y2, z2, x3, y3, z3, tex, r, g, b);
	else
		render_quad_f4(ctx, clip, x0, y0, z0, x1, y1, z1, x2, y2, z2, x3, y3, z3, r, g, b);
}

static void skirt(RenderContext *ctx, RECT *clip, int hw, int hl, int body) {
	/* Dark shadow strip down to the ground, so the car doesn't look like
	 * it's floating when the camera looks down at it. */
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

/* Plain box: the original shape, used by the sedan. */
static void draw_box_body(
	RenderContext *ctx, RECT *clip, Texture *tex,
	int hw, int hl, int roof, int body,
	uint8_t br, uint8_t bg, uint8_t bb
) {
	panel(ctx, clip, tex,
		-hw, roof,  hl,   hw, roof,  hl,
		-hw, body,  hl,   hw, body,  hl,
		br, bg, bb);

	/* Tail: this is the face the chase camera is always looking at, so it
	 * must show the actual livery, not a tint - a textured car's tail uses
	 * the same neutral color as every other panel; only a flat-shaded car
	 * (no livery to protect) gets the dim red "taillight" paint. */
	panel(ctx, clip, tex,
		 hw, roof, -hl,  -hw, roof, -hl,
		 hw, body, -hl,  -hw, body, -hl,
		tex ? br : (uint8_t)(br / 2), tex ? bg : 20, tex ? bb : 20);

	panel(ctx, clip, tex,
		-hw, roof, -hl,  -hw, roof,  hl,
		-hw, body, -hl,  -hw, body,  hl,
		br, bg, bb);

	panel(ctx, clip, tex,
		 hw, roof,  hl,   hw, roof, -hl,
		 hw, body,  hl,   hw, body, -hl,
		br, bg, bb);

	panel(ctx, clip, tex,
		-hw, roof, -hl,   hw, roof, -hl,
		-hw, roof,  hl,   hw, roof,  hl,
		(uint8_t)(br * 2 / 3), (uint8_t)(bg * 2 / 3), (uint8_t)(bb * 2 / 3));

	/* Small real taillight strips at the bottom corners of the tail,
	 * drawn as flat untextured quads so they never depend on the livery. */
	render_quad_f4(ctx, clip,
		 hw, body + (roof - body) / 6, -hl,   hw - (hw / 3), body + (roof - body) / 6, -hl,
		 hw, body,                     -hl,   hw - (hw / 3), body,                     -hl,
		180, 20, 20);
	render_quad_f4(ctx, clip,
		-hw + (hw / 3), body + (roof - body) / 6, -hl,  -hw, body + (roof - body) / 6, -hl,
		-hw + (hw / 3), body,                     -hl,  -hw, body,                     -hl,
		180, 20, 20);

	skirt(ctx, clip, hw, hl, body);
}

/* Low, long cabin with a fastback taper: the roofline slopes down toward
 * the tail instead of staying flat, ending low with a small spoiler right
 * above it. The camera is always behind the car, so this (not a front
 * wedge, which would never be seen) is where a "sports car" silhouette
 * actually needs to read. taper_z is where the slope begins (how far
 * forward of the tail tip the flat roof ends). */
static void draw_sports_body(
	RenderContext *ctx, RECT *clip, Texture *tex,
	int hw, int hl, int roof, int body, int taper_z,
	uint8_t br, uint8_t bg, uint8_t bb
) {
	/* Nose: flat, like the sedan's front wall. */
	panel(ctx, clip, tex,
		-hw, roof,  hl,   hw, roof,  hl,
		-hw, body,  hl,   hw, body,  hl,
		br, bg, bb);

	/* Fastback: slopes from the flat roof's back edge (taper_z) down to a
	 * low tail lip. Replaces the sedan's flat tail wall. This is the face
	 * the chase camera is always looking at, so a textured car shows the
	 * actual livery here (neutral tint); only a flat-shaded car gets the
	 * dim red "taillight" paint. */
	panel(ctx, clip, tex,
		 hw, roof, taper_z,  -hw, roof, taper_z,
		 hw, body,     -hl,  -hw, body,     -hl,
		tex ? br : (uint8_t)(br / 2), tex ? bg : 20, tex ? bb : 20);

	/* Sides: trapezoidal, top edge pulled forward to taper_z, bottom edge
	 * running the full length to match the tail taper underneath it. */
	panel(ctx, clip, tex,
		-hw, roof,      hl,  -hw, roof, taper_z,
		-hw, body,      hl,  -hw, body,     -hl,
		br, bg, bb);

	panel(ctx, clip, tex,
		 hw, roof, taper_z,   hw, roof,      hl,
		 hw, body,     -hl,   hw, body,      hl,
		br, bg, bb);

	/* Roof: only spans from the nose back to where the taper begins. */
	panel(ctx, clip, tex,
		-hw, roof,      hl,   hw, roof,      hl,
		-hw, roof, taper_z,   hw, roof, taper_z,
		(uint8_t)(br * 2 / 3), (uint8_t)(bg * 2 / 3), (uint8_t)(bb * 2 / 3));

	/* Small rear spoiler, floating just above the low tail lip. */
	int wing_y = body - 16; /* a bit higher (more negative) than the tail */
	int wing_hw = (hw * 3) / 4;
	render_quad_f4(ctx, clip,
		-wing_hw, wing_y, -hl,       wing_hw, wing_y, -hl,
		-wing_hw, wing_y, -hl + 18,  wing_hw, wing_y, -hl + 18,
		30, 30, 34);

	/* Small real taillight strips at the bottom corners of the tail lip. */
	render_quad_f4(ctx, clip,
		 hw, body + 16, -hl,   hw - (hw / 3), body + 16, -hl,
		 hw, body,      -hl,   hw - (hw / 3), body,      -hl,
		180, 20, 20);
	render_quad_f4(ctx, clip,
		-hw + (hw / 3), body + 16, -hl,  -hw, body + 16, -hl,
		-hw + (hw / 3), body,      -hl,  -hw, body,      -hl,
		180, 20, 20);

	skirt(ctx, clip, hw, hl, body);
}

/* Roof-mounted beacon bar ("kogut"): two small lit boxes side by side,
 * each drawn as a top face + a tail-facing face (the two faces the chase
 * camera actually sees) so they read as lit domes, not flat decals. */
static void draw_lightbar(
	RenderContext *ctx, RECT *clip, int hw, int roof_y,
	uint8_t r1, uint8_t g1, uint8_t b1, uint8_t r2, uint8_t g2, uint8_t b2
) {
	int lamp_hw = hw / 3;
	int lamp_hl = 16;
	int lamp_h  = 18;
	int top_y   = roof_y - lamp_h;

	/* Left beacon. */
	render_quad_f4(ctx, clip,
		-lamp_hw * 2, top_y,  lamp_hl,   0, top_y,  lamp_hl,
		-lamp_hw * 2, top_y, -lamp_hl,   0, top_y, -lamp_hl,
		r1, g1, b1);
	render_quad_f4(ctx, clip,
		-lamp_hw * 2, top_y,  -lamp_hl,   0, top_y,  -lamp_hl,
		-lamp_hw * 2, roof_y, -lamp_hl,   0, roof_y, -lamp_hl,
		r1, g1, b1);

	/* Right beacon. */
	render_quad_f4(ctx, clip,
		0, top_y,  lamp_hl,   lamp_hw * 2, top_y,  lamp_hl,
		0, top_y, -lamp_hl,   lamp_hw * 2, top_y, -lamp_hl,
		r2, g2, b2);
	render_quad_f4(ctx, clip,
		0, top_y,   -lamp_hl,   lamp_hw * 2, top_y,  -lamp_hl,
		0, roof_y,  -lamp_hl,   lamp_hw * 2, roof_y, -lamp_hl,
		r2, g2, b2);
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

	/* A textured car is modulated from a neutral 128 base so the livery's
	 * own colors show through untinted; a flat-shaded one still uses its
	 * actual paint color as the base, same as before textures existed. */
	uint8_t base_r = v->tex ? 128 : v->r;
	uint8_t base_g = v->tex ? 128 : v->g;
	uint8_t base_b = v->tex ? 128 : v->b;

	int lightbar_hw = 90, lightbar_roof = -130;

	switch (v->shape) {
		case VSHAPE_SUV:
			/* Taller, boxier cabin riding higher off the ground. */
			draw_box_body(ctx, clip, v->tex, 98, 175, -185, -55, base_r, base_g, base_b);
			lightbar_hw = 98;
			lightbar_roof = -185;
			break;

		case VSHAPE_SPORTS:
			/* Low, long cabin with a fastback taper toward the tail. */
			draw_sports_body(ctx, clip, v->tex, 82, 195, -85, -25, -125, base_r, base_g, base_b);
			lightbar_hw = 82;
			lightbar_roof = -85;
			break;

		case VSHAPE_SEDAN:
		default:
			draw_box_body(ctx, clip, v->tex, 90, 190, -130, -30, base_r, base_g, base_b);
			break;
	}

	switch (v->lightbar) {
		case LIGHTBAR_POLICE:
			draw_lightbar(ctx, clip, lightbar_hw, lightbar_roof, 220, 20, 20, 20, 40, 230);
			break;
		case LIGHTBAR_FIRE:
		case LIGHTBAR_AMBULANCE:
			draw_lightbar(ctx, clip, lightbar_hw, lightbar_roof, 20, 40, 230, 20, 40, 230);
			break;
		case LIGHTBAR_NONE:
		default:
			break;
	}
}
