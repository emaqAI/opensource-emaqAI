#include <psxgpu.h>
#include <psxgte.h>
#include "hud.h"
#include "fixed.h"

void hud_draw(Vehicle *player, PoliceState *police, Mission *m) {
	int speed_kmh = (iabs(player->speed) >> 12) * 20;

	FntPrint(-1, "PREDKOSC: %3d km/h\n", speed_kmh);

	FntPrint(-1, "POSZUKIWANY: [");
	for (int i = 0; i < WANTED_MAX; i++)
		FntPrint(-1, (i < police->wanted) ? "*" : ".");
	FntPrint(-1, "]\n");

	switch (m->stage) {
		case STAGE_M1_PLAY:
		case STAGE_M3_PLAY: {
			int dx = m->target_x - vehicle_world_x(player);
			int dz = m->target_z - vehicle_world_z(player);
			int dist = SquareRoot0(dx * dx + dz * dz) / 10;

			if (m->stage == STAGE_M3_PLAY) {
				int secs = m->mission_timer / 60;
				if (m->mission_timer > 0)
					FntPrint(-1, "DOSTAWA: %d m   CZAS: %ds\n", dist, secs);
				else
					FntPrint(-1, "DOSTAWA: %d m   CZAS: --\n", dist);
			} else {
				FntPrint(-1, "CEL: %d m\n", dist);
			}
			break;
		}

		case STAGE_M2_PLAY: {
			int secs = m->mission_timer / 60;
			if (secs < 0)
				secs = 0;
			FntPrint(-1, "UCIEKAJ POLICJI!  CZAS: %ds\n", secs);
			break;
		}

		default:
			break;
	}

	if (player->crash_timer > 6)
		FntPrint(-1, "!! KOLIZJA !!\n");
}
