#include <psxapi.h>
#include "input.h"

static uint8_t pad_buff[2][34];

void input_init(void) {
	InitPAD(pad_buff[0], 34, pad_buff[1], 34);
	StartPAD();
	ChangeClearPAD(0);
}

void input_update(InputState *st) {
	PADTYPE *pad = (PADTYPE *) pad_buff[0];

	uint16_t prev_held = st->held;
	uint16_t new_held  = 0;

	if (pad->stat == 0) {
		st->connected = 1;

		/* pad->btn is active-low; flip so 1 == pressed. */
		new_held = (uint16_t) ~(pad->btn);

		if ((pad->type == PAD_ID_ANALOG_STICK) || (pad->type == PAD_ID_ANALOG)) {
			st->analog = 1;
			st->lx = (int) pad->ls_x - 128;
			st->ly = (int) pad->ls_y - 128;
		} else {
			st->analog = 0;
			st->lx = 0;
			st->ly = 0;
		}
	} else {
		st->connected = 0;
		st->analog    = 0;
		st->lx = 0;
		st->ly = 0;
	}

	st->held     = new_held;
	st->pressed  = new_held & (uint16_t) ~prev_held;
	st->released = prev_held & (uint16_t) ~new_held;
}
