#include <inline_c.h>
#include "texture.h"

extern const uint32_t tim_road[];
extern const uint32_t tim_sidewalk[];
extern const uint32_t tim_wall_concrete[];
extern const uint32_t tim_wall_brick[];
extern const uint32_t tim_wall_glass[];
extern const uint32_t tim_roof[];
extern const uint32_t tim_car_stripe[];
extern const uint32_t tim_car_taxi[];
extern const uint32_t tim_car_police[];
extern const uint32_t tim_car_fire[];

static Texture textures[TEX_COUNT];

static void load_one(TextureId id, const uint32_t *tim_data) {
	TIM_IMAGE info;

	GetTimInfo(tim_data, &info);

	LoadImage(info.prect, info.paddr);
	DrawSync(0);
	LoadImage(info.crect, info.caddr);
	DrawSync(0);

	textures[id].tpage = getTPage(info.mode, 0, info.prect->x, info.prect->y);
	textures[id].clut = getClut(info.crect->x, info.crect->y);
}

void textures_load(void) {
	load_one(TEX_ROAD, tim_road);
	load_one(TEX_SIDEWALK, tim_sidewalk);
	load_one(TEX_WALL_CONCRETE, tim_wall_concrete);
	load_one(TEX_WALL_BRICK, tim_wall_brick);
	load_one(TEX_WALL_GLASS, tim_wall_glass);
	load_one(TEX_ROOF, tim_roof);
	load_one(TEX_CAR_STRIPE, tim_car_stripe);
	load_one(TEX_CAR_TAXI, tim_car_taxi);
	load_one(TEX_CAR_POLICE, tim_car_police);
	load_one(TEX_CAR_FIRE, tim_car_fire);
}

Texture *texture_get(TextureId id) {
	return &textures[id];
}
