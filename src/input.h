#pragma once

#include <stdint.h>
#include <psxpad.h>

typedef struct {
	uint16_t held;      /* Active-high: bit set while button is held.      */
	uint16_t pressed;   /* Active-high: bit set only on the press edge.    */
	uint16_t released;  /* Active-high: bit set only on the release edge.  */
	int      connected;
	int      analog;    /* Non-zero if the pad reports analog stick data.  */
	int      lx, ly;    /* Left stick, centered on 0, roughly -128..127.   */
} InputState;

void input_init(void);
void input_update(InputState *st);
