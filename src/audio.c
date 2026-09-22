#include <stdint.h>
#include <psxspu.h>
#include <hwregs_c.h>
#include "audio.h"
#include "vehicle.h"
#include "fixed.h"

/* .VAG header, matches PSn00bSDK's own vagsample example - big-endian
 * fields, 48 bytes, followed directly by the raw SPU-ADPCM body. */
typedef struct {
	uint32_t magic;
	uint32_t version;
	uint32_t interleave;
	uint32_t size;         /* big-endian */
	uint32_t sample_rate;  /* big-endian */
	uint16_t _reserved[5];
	uint16_t channels;
	char     name[16];
} VAG_Header;

#define SWAP_ENDIAN(x) ( \
	(((uint32_t) (x) & 0x000000ff) << 24) | \
	(((uint32_t) (x) & 0x0000ff00) <<  8) | \
	(((uint32_t) (x) & 0x00ff0000) >>  8) | \
	(((uint32_t) (x) & 0xff000000) >> 24) \
)

extern const uint8_t snd_engine[];
extern const uint8_t snd_siren[];
extern const uint8_t snd_crash[];

#define CH_ENGINE 0
#define CH_SIREN  1
#define CH_CRASH  2

/* First 4KB of SPU RAM is reserved (capture buffers + a dummy sample
 * SpuInit() itself uploads at 0x1000), see the vagsample example. */
#define ALLOC_START_ADDR 0x1010

static int next_sample_addr = ALLOC_START_ADDR;

static int engine_addr, engine_sr;
static int siren_addr, siren_sr;
static int crash_addr, crash_sr;

static int siren_playing = 0;

static int upload_sample(const void *data, int size) {
	int addr = next_sample_addr;
	int rounded = (size + 63) & 0xffffffc0;

	SpuSetTransferMode(SPU_TRANSFER_BY_DMA);
	SpuSetTransferStartAddr(addr);
	SpuWrite((const uint32_t *) data, rounded);
	SpuIsTransferCompleted(SPU_TRANSFER_WAIT);

	next_sample_addr = addr + rounded;
	return addr;
}

static void start_channel(int ch, int addr, int rate, int vol_l, int vol_r) {
	SpuSetKey(0, 1 << ch);

	SPU_CH_FREQ(ch) = getSPUSampleRate(rate);
	SPU_CH_ADDR(ch) = getSPUAddr(addr);
	SPU_CH_VOL_L(ch) = vol_l;
	SPU_CH_VOL_R(ch) = vol_r;
	SPU_CH_ADSR1(ch) = 0x00ff;
	SPU_CH_ADSR2(ch) = 0x0000;

	SpuSetKey(1, 1 << ch);
}

static int load_vag(const uint8_t *raw, int *out_addr, int *out_sr) {
	const VAG_Header *hdr = (const VAG_Header *) raw;
	int size = (int) SWAP_ENDIAN(hdr->size);

	*out_addr = upload_sample(raw + sizeof(VAG_Header), size);
	*out_sr   = (int) SWAP_ENDIAN(hdr->sample_rate);
	return size;
}

void audio_init(void) {
	SpuInit();

	load_vag(snd_engine, &engine_addr, &engine_sr);
	load_vag(snd_siren, &siren_addr, &siren_sr);
	load_vag(snd_crash, &crash_addr, &crash_sr);

	/* The engine loop plays continuously from boot; only its pitch/volume
	 * change afterward (see audio_engine_update()), so it never re-clicks. */
	start_channel(CH_ENGINE, engine_addr, engine_sr, 0x2000, 0x2000);
}

void audio_engine_update(int speed) {
	int abs_speed = (speed < 0) ? -speed : speed;
	if (abs_speed > VEH_MAX_SPEED)
		abs_speed = VEH_MAX_SPEED;

	/* Idle hum around engine_sr*0.7, revving up to engine_sr*2.0 at top
	 * speed - plain integer lerp, no floats (no FPU on the R3000). */
	int rate = engine_sr - (engine_sr * 3 / 10)
		+ (abs_speed * (engine_sr * 13 / 10)) / VEH_MAX_SPEED;

	int vol = 0x1800 + (abs_speed * 0x2400) / VEH_MAX_SPEED;

	SPU_CH_FREQ(CH_ENGINE) = getSPUSampleRate(rate);
	SPU_CH_VOL_L(CH_ENGINE) = vol;
	SPU_CH_VOL_R(CH_ENGINE) = vol;
}

void audio_set_siren(int on) {
	if (on == siren_playing)
		return;
	siren_playing = on;

	if (on)
		start_channel(CH_SIREN, siren_addr, siren_sr, 0x3800, 0x3800);
	else
		SpuSetKey(0, 1 << CH_SIREN);
}

void audio_play_crash(void) {
	start_channel(CH_CRASH, crash_addr, crash_sr, 0x3fff, 0x3fff);
}
