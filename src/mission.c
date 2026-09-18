#include "mission.h"

void mission_init(Mission *m, World *w) {
	m->stage = STAGE_TITLE;
	m->timer = 0;
	m->objective_radius = 0;
	m->mission_timer = -1;
	m->m3_on_time = 0;
	(void) w;
}

int mission_is_gameplay(Mission *m) {
	switch (m->stage) {
		case STAGE_M1_PLAY:
		case STAGE_M2_PLAY:
		case STAGE_M3_PLAY:
		case STAGE_FREEROAM:
			return 1;
		default:
			return 0;
	}
}

void mission_update(Mission *m, InputState *in, Vehicle *player, World *w, PoliceState *police) {
	m->timer++;

	int advance = (in->pressed & (PAD_CROSS | PAD_START)) ? 1 : 0;

	switch (m->stage) {
		case STAGE_TITLE:
			if (advance) { m->stage = STAGE_INTRO_1; m->timer = 0; }
			break;

		case STAGE_INTRO_1:
			if (advance) { m->stage = STAGE_INTRO_2; m->timer = 0; }
			break;

		case STAGE_INTRO_2:
			if (advance) { m->stage = STAGE_M1_BRIEF; m->timer = 0; }
			break;

		case STAGE_M1_BRIEF:
			if (advance) {
				m->stage = STAGE_M1_PLAY;
				m->timer = 0;
				world_tile_center(w, 8, 8, &m->target_x, &m->target_z);
				m->objective_radius = 260;
				m->mission_timer = -1;
			}
			break;

		case STAGE_M1_PLAY: {
			int dx = m->target_x - vehicle_world_x(player);
			int dz = m->target_z - vehicle_world_z(player);
			if ((dx * dx + dz * dz) < (m->objective_radius * m->objective_radius)) {
				m->stage = STAGE_M1_WIN;
				m->timer = 0;
			}
			break;
		}

		case STAGE_M1_WIN:
			if (advance) { m->stage = STAGE_M2_BRIEF; m->timer = 0; }
			break;

		case STAGE_M2_BRIEF:
			if (advance) {
				m->stage = STAGE_M2_PLAY;
				m->timer = 0;
				m->objective_radius = 0;
				m->mission_timer = 60 * 45;
				police_set_level(police, w, 2);
			}
			break;

		case STAGE_M2_PLAY:
			if (m->mission_timer > 0)
				m->mission_timer--;

			if ((m->mission_timer <= 0) || (police->wanted == 0)) {
				m->stage = STAGE_M2_WIN;
				m->timer = 0;
				police_set_level(police, w, 0);
			}
			break;

		case STAGE_M2_WIN:
			if (advance) { m->stage = STAGE_M3_BRIEF; m->timer = 0; }
			break;

		case STAGE_M3_BRIEF:
			if (advance) {
				m->stage = STAGE_M3_PLAY;
				m->timer = 0;
				world_tile_center(w, 0, 8, &m->target_x, &m->target_z);
				m->objective_radius = 260;
				m->mission_timer = 60 * 50;
				m->m3_on_time = 0;
			}
			break;

		case STAGE_M3_PLAY: {
			if (m->mission_timer > 0)
				m->mission_timer--;

			int dx = m->target_x - vehicle_world_x(player);
			int dz = m->target_z - vehicle_world_z(player);
			if ((dx * dx + dz * dz) < (m->objective_radius * m->objective_radius)) {
				m->m3_on_time = (m->mission_timer > 0);
				m->stage = STAGE_M3_WIN;
				m->timer = 0;
			}
			break;
		}

		case STAGE_M3_WIN:
			if (advance) { m->stage = STAGE_ENDING; m->timer = 0; }
			break;

		case STAGE_ENDING:
			if (advance) { m->stage = STAGE_FREEROAM; m->timer = 0; }
			break;

		case STAGE_FREEROAM:
		default:
			break;
	}
}

static void line(int row, const char *s) {
	FntPrint(-1, "  %s\n", s);
	(void) row;
}

void mission_draw_overlay(Mission *m) {
	switch (m->stage) {
		case STAGE_TITLE:
			FntPrint(-1, "\n\n\n\n");
			line(0, "");
			line(0, "        C H R O M E   C I T Y");
			line(0, "");
			line(0, "     jeden kurs. jedno miasto. zero litosci.");
			line(0, "");
			line(0, "");
			line(0, "         START lub X - zaczynamy");
			break;

		case STAGE_INTRO_1:
			line(0, "");
			line(0, "ROK 1998. CHROME CITY.");
			line(0, "Miasto nigdy nie zasypia.");
			line(0, "");
			line(0, "Jestes kierowca do wynajecia -");
			line(0, "tansza opcja niz taksowka,");
			line(0, "i o wiele mniej pytan.");
			line(0, "");
			line(0, "(X - dalej)");
			break;

		case STAGE_INTRO_2:
			line(0, "");
			line(0, "Dzwoni WIKTOR, stary znajomy.");
			line(0, "");
			line(0, "\"Mam dla ciebie robote.");
			line(0, "  Prawdziwa robote.\"");
			line(0, "");
			line(0, "(X - dalej)");
			break;

		case STAGE_M1_BRIEF:
			line(0, "");
			line(0, "MISJA 1: PIERWSZY KURS");
			line(0, "");
			line(0, "Wiktor czeka po drugiej stronie");
			line(0, "miasta. Dojedz do znacznika na");
			line(0, "mapie miasta.");
			line(0, "");
			line(0, "(X - jedziemy)");
			break;

		case STAGE_M1_WIN:
			line(0, "");
			line(0, "Wiktor: \"Niezle jezdzisz.");
			line(0, "  Mam dla ciebie cos wiekszego.\"");
			line(0, "");
			line(0, "(X - dalej)");
			break;

		case STAGE_M2_BRIEF:
			line(0, "");
			line(0, "MISJA 2: GORACY TOWAR");
			line(0, "");
			line(0, "Cos poszlo nie tak - gliny juz");
			line(0, "jada! Zgub policje zanim");
			line(0, "skonczy sie czas.");
			line(0, "");
			line(0, "(X - start pogoni)");
			break;

		case STAGE_M2_WIN:
			line(0, "");
			line(0, "Udalo sie! Gliny odpuscily.");
			line(0, "Chrome City oddycha... na razie.");
			line(0, "");
			line(0, "(X - dalej)");
			break;

		case STAGE_M3_BRIEF:
			line(0, "");
			line(0, "MISJA 3: OSTATNIA DOSTAWA");
			line(0, "");
			line(0, "To finalna przesylka Wiktora.");
			line(0, "Dowiez ja na drugi koniec miasta,");
			line(0, "najlepiej zanim czas minie.");
			line(0, "");
			line(0, "(X - jedziemy)");
			break;

		case STAGE_M3_WIN:
			line(0, "");
			if (m->m3_on_time) {
				line(0, "Dojechales na czas!");
				line(0, "Wiktor: \"Robota skonczona.");
				line(0, "  Szacunek.\"");
			} else {
				line(0, "Dojechales... troche pozniej");
				line(0, "niz mial nadzieje Wiktor.");
				line(0, "Wiktor: \"Liczy sie efekt.\"");
			}
			line(0, "");
			line(0, "(X - dalej)");
			break;

		case STAGE_ENDING:
			line(0, "");
			line(0, "KONIEC... na razie.");
			line(0, "");
			line(0, "Dzieki za jazde po Chrome City.");
			line(0, "Wolna jazda zostala odblokowana -");
			line(0, "miasto jest juz cale twoje.");
			line(0, "");
			line(0, "(START - wolna jazda)");
			break;

		default:
			break;
	}
}
