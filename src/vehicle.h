#pragma once

#include <psxgte.h>
#include "render.h"
#include "world.h"

typedef enum {
	VSHAPE_SEDAN = 0, /* Plain box, the original shape.                    */
	VSHAPE_SUV,       /* Taller, boxier cabin + taller skirt (ride height).*/
	VSHAPE_SPORTS     /* Low cabin, sloped wedge hood, small rear spoiler. */
} VehicleShape;

typedef enum {
	LIGHTBAR_NONE = 0,
	LIGHTBAR_POLICE, /* Two beacons, red + blue ("kogut"). */
	LIGHTBAR_FIRE    /* Two beacons, red + red.            */
} LightbarType;

/* A car's world position is stored pre-scaled by ONE (4096) for smooth
 * sub-unit physics; use vehicle_world_x()/vehicle_world_z() to get plain
 * world-space coordinates for rendering/collision/gameplay logic. */
typedef struct {
	VECTOR  pos;
	int     heading;   /* 0..ANGLE_MAX-1 */
	int     speed;     /* Scaled by ONE, signed (negative = reverse).      */
	int     radius;    /* Collision radius, raw world units.               */
	int     crash_timer;
	uint8_t r, g, b;
	Texture      *tex;      /* NULL = flat-shaded fallback using r,g,b. */
	VehicleShape shape;     /* Body silhouette; defaults to VSHAPE_SEDAN. */
	LightbarType lightbar;  /* Roof beacons ("kogut"); defaults to none.  */
} Vehicle;

#define VEH_MAX_SPEED     (9 * 4096)
#define VEH_MAX_REVERSE   (4 * 4096)
#define VEH_ACCEL         410
#define VEH_BRAKE         820
#define VEH_FRICTION      160
#define VEH_TURN_RATE     34
#define VEH_RADIUS        140

void vehicle_init(Vehicle *v, int x, int z, int heading, uint8_t r, uint8_t g, uint8_t b);

static inline int vehicle_world_x(Vehicle *v) { return v->pos.vx >> 12; }
static inline int vehicle_world_z(Vehicle *v) { return v->pos.vz >> 12; }

/* accel: -1 (brake/reverse), 0, or 1 (accelerate). steer: -1, 0, or 1.
 * handbrake: non-zero engages a stronger brake with a tighter turn.
 * Returns non-zero if the car hit a building this frame. */
int vehicle_update(Vehicle *v, World *w, int accel, int steer, int handbrake);

void vehicle_draw(Vehicle *v, RenderContext *ctx, MATRIX *cam_mtx, RECT *clip);
