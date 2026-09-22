#include <inline_c.h>
#include "camera.h"
#include "fixed.h"

void camera_init(Camera *cam, Vehicle *target) {
	cam->heading = target->heading;
}

void camera_update(Camera *cam, Vehicle *target, MATRIX *out) {
	cam->heading = angle_wrap(cam->heading + angle_delta(cam->heading, target->heading) / 4);

	int fwd_x = isin(cam->heading);
	int fwd_z = icos(cam->heading);

	int car_x = vehicle_world_x(target);
	int car_z = vehicle_world_z(target);

	int cam_x = car_x - fx_mul(fwd_x, CAM_DIST);
	int cam_z = car_z - fx_mul(fwd_z, CAM_DIST);
	int cam_y = -CAM_HEIGHT;

	/* This builds a *view* (world->camera) matrix, not an object matrix, so
	 * unlike vehicle_draw()'s RotMatrix(heading) (which places an object
	 * facing that heading), the rotation here has to be the inverse of the
	 * camera's own facing - i.e. negated - or the picture spins the wrong
	 * way and by the wrong amount every time the camera's heading changes
	 * (most visible while turning, since heading is otherwise constant). */
	SVECTOR trot = { (short) CAM_PITCH, (short) -cam->heading, 0, 0 };
	VECTOR  tpos;
	MATRIX  mtx;

	RotMatrix(&trot, &mtx);

	tpos.vx = -cam_x;
	tpos.vy = -cam_y;
	tpos.vz = -cam_z;
	ApplyMatrixLV(&mtx, &tpos, &tpos);
	TransMatrix(&mtx, &tpos);

	*out = mtx;
}
