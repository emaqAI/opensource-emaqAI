#pragma once

#include <psxgpu.h>

typedef enum {
	TEX_ROAD = 0,
	TEX_SIDEWALK,
	TEX_WALL_CONCRETE,
	TEX_WALL_BRICK,
	TEX_WALL_GLASS,
	TEX_ROOF,
	TEX_CAR_STRIPE,
	TEX_CAR_TAXI,
	TEX_CAR_POLICE,
	TEX_COUNT
} TextureId;

typedef struct {
	uint16_t tpage;
	uint16_t clut;
} Texture;

/* Uploads all game textures to VRAM. Must be called once after ResetGraph(). */
void textures_load(void);

Texture *texture_get(TextureId id);
