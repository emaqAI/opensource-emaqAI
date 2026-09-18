#pragma once

#include <psxgte.h>
#include "vehicle.h"

#define CAM_DIST   420
#define CAM_HEIGHT 260
#define CAM_PITCH  300 /* out of ANGLE_MAX, looks down at the road */

typedef struct {
	int heading; /* Smoothed copy of the target's heading. */
} Camera;

void camera_init(Camera *cam, Vehicle *target);

/* Follows 'target' and writes the resulting world->camera matrix to *out. */
void camera_update(Camera *cam, Vehicle *target, MATRIX *out);
