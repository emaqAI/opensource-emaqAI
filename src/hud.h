#pragma once

#include "vehicle.h"
#include "police.h"
#include "mission.h"

/* Draws the in-gameplay HUD (speed, wanted stars, mission objective/timer).
 * Does not call FntFlush(). */
void hud_draw(Vehicle *player, PoliceState *police, Mission *m);
